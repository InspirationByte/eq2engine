//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#include "core/IPackFileReader.h"
#include "core/ConVar.h"
#include "imaging/ImageLoader.h"
#include "utils/KeyValues.h"

#include "NVRHIRenderAPI.h"
#include "NVRHIRenderDefs.h"
#include "NVRHIStates.h"
#include "NVRHICommandRecorder.h"
#include "NVRHIRenderPassRecorder.h"

#include "../RenderWorker.h"
#include "NVRHIComputePassRecorder.h"

constexpr EqStringRef s_shaderKindVertexName = "Vertex";
constexpr EqStringRef s_shaderKindFragmentName = "Fragment";
constexpr EqStringRef s_shaderKindComputeName = "Compute";
constexpr EqStringRef s_DefaultVertexLayoutName = "Default";

DECLARE_CVAR(nvrhi_preload_shaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);

CNVRHIRenderAPI CNVRHIRenderAPI::Instance;
IShaderAPI* g_renderAPI = &CNVRHIRenderAPI::Instance;

static uint PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind, int entryPointStrHash)
{
	uint hash = queryStrHash | (static_cast<uint>(vertexLayoutIdx) << StringId24Bits) | (static_cast<uint>(kind) << (StringId24Bits + 4));
	hash *= 31;
	hash += entryPointStrHash;
	return hash;
}

ShaderInfoNVRHIImpl::~ShaderInfoNVRHIImpl()
{
}

ShaderInfoNVRHIImpl::ShaderInfoNVRHIImpl(ShaderInfoNVRHIImpl&& other) noexcept
	: shaderName(std::move(other.shaderName))
	, shaderPackFile(std::move(other.shaderPackFile))
	, vertexLayouts(std::move(other.vertexLayouts))
	, defines(std::move(other.defines))
	, modules(std::move(other.modules))
	, modulesMap(std::move(other.modulesMap))
	, shaderKinds(other.shaderKinds)

{
	other.shaderPackFile = nullptr;
}

ShaderInfoNVRHIImpl& ShaderInfoNVRHIImpl::operator=(ShaderInfoNVRHIImpl&& other) noexcept
{
	shaderName = std::move(other.shaderName);
	shaderPackFile = std::move(other.shaderPackFile);
	vertexLayouts = std::move(other.vertexLayouts);
	defines = std::move(other.defines);
	modules = std::move(other.modules);
	modulesMap = std::move(other.modulesMap);
	shaderKinds = other.shaderKinds;
	other.shaderPackFile = nullptr;
	return *this;
}

void ShaderInfoNVRHIImpl::Release()
{
	for (Module& module : modules)
		module.rhiModule = nullptr;
}

bool ShaderInfoNVRHIImpl::GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const
{
	Array<int> defineIds(PP_SL);
	for (const EqString& define : findDefines)
	{
		const int defineId = arrayFindIndex(defines, define);
		if (defineId == -1)
			return false;
		defineIds.append(defineId);
	}

	arraySort(defineIds, [](int a, int b) {
		return a - b;
	});

	EqString queryStr;
	for (int id : defineIds)
	{
		if (queryStr.Length())
			queryStr.Append("|");
		queryStr.Append(defines[id]);
	}
	outHash = StringId24(queryStr, true);
	return true;
}

//------------------------------------------

void CNVRHIRenderAPI::Init(const ShaderAPIParams& params)
{
	ShaderAPI_Base::Init(params);

	int shaderPackCount = 0;
	int shaderModCount = 0;
	EqString shaderPackPath;
	CFileSystemFind fsFind("shaders/*.shd", SP_MOD | SP_DATA);
	while (fsFind.Next())
	{
		if (fsFind.IsDirectory())
			continue;

		fnmPathCombine(shaderPackPath, "shaders", fsFind.GetPath());

		shaderModCount += LoadShaderPackage(shaderPackPath);
		++shaderPackCount;
	}

	Msg("* Found %d shader packages, %d modules loaded\n", shaderPackCount, shaderModCount);
}

void CNVRHIRenderAPI::Shutdown()
{
	ShaderAPI_Base::Shutdown();
	m_shaderCache.clear(true);
	m_rhiDevice = nullptr;
}

int CNVRHIRenderAPI::LoadShaderPackage(const char* filename)
{
	IPackFileReaderPtr shaderPackFile = g_fileSystem->OpenPackage(filename, SP_MOD | SP_DATA);
	if (!shaderPackFile)
		return 0;

	KVSection shaderInfoKvs;
	{
		IFilePtr file = shaderPackFile->Open("ShaderInfo", VS_OPEN_READ);
		if (!KV_LoadFromStream(file, &shaderInfoKvs))
		{
			Msg("No ShaderInfo in file %s\n", filename);
			return 0;
		}
	}

	defer{
		if (shaderPackFile)
			m_shaderCache.remove(StringId24(shaderInfoKvs.GetName()));
	};

	if (!CString::SubString(filename, shaderInfoKvs.GetName()))
	{
		ASSERT_FAIL("Shader package '%s' file name doesn't match it's name '%s' in desc", filename, shaderInfoKvs.GetName());
		return 0;
	}

	auto it = m_shaderCache.find(StringId24(shaderInfoKvs.GetName()));
	if (!it.atEnd())
	{
		ASSERT_FAIL("Shader '%s' has been already loaded from different package", shaderInfoKvs.GetName());
		return 0;
	}

	it = m_shaderCache.insert(StringId24(shaderInfoKvs.GetName()));

	ShaderInfoNVRHIImpl& shaderInfo = *it;
	shaderInfo.shaderPackFile = shaderPackFile;
	shaderInfo.shaderName = shaderInfoKvs.GetName();

	const KVSection* defines = shaderInfoKvs["Defines"];
	if (defines)
	{
		shaderInfo.defines.reserve(defines->ValueCount());
		for (const EqStringRef def : defines->Values<EqStringRef>())
			shaderInfo.defines.append(def);
	}

	for (const KVSection* key : shaderInfoKvs.Get("VertexLayouts").Keys())
	{
		ShaderInfoNVRHIImpl::VertLayout& layout = shaderInfo.vertexLayouts.append();
		layout.name = key->GetName();
		if (layout.name != s_DefaultVertexLayoutName)
			layout.nameHash = StringId24(layout.name);
		
		if (!CString::CompareCaseIns(KV_GetValueString(key, 0), "aliasOf"))
		{
			layout.aliasOf = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoNVRHIImpl::VertLayout& layout) {
				return layout.name == EqStringRef(KV_GetValueString(key, 1));
			});
		}
	}

	auto getKind = [](const EqStringRef& kindStr) -> int {
		if (!kindStr.CompareCaseIns(s_shaderKindVertexName))
			return SHADERKIND_VERTEX;
		else if (!kindStr.CompareCaseIns(s_shaderKindFragmentName))
			return SHADERKIND_FRAGMENT;
		else if (!kindStr.CompareCaseIns(s_shaderKindComputeName))
			return SHADERKIND_COMPUTE;
		return 0;
	};

	auto getKindExt = [](int kind) -> char* {
		if (kind == SHADERKIND_VERTEX)
			return ".vert";
		if (kind == SHADERKIND_FRAGMENT)
			return ".frag";
		if (kind == SHADERKIND_COMPUTE)
			return ".comp";
		return nullptr;
	};

	int filesFound = 0;
	const KVSection* fileListSec = shaderInfoKvs["FileList"];
	for (const KVSection* itemSec : fileListSec->Keys("spv"))
	{
		int vertLayoutIdx = -1;
		EqStringRef kindStr;
		EqStringRef entryPointName;
		EqStringRef queryStr;
		if (itemSec->GetValues(vertLayoutIdx, kindStr, entryPointName, queryStr) < 4)
		{
			ASSERT_FAIL("Shader %s 'spv' does not have 4 values");
			break;
		}

		const int kind = getKind(kindStr);
		ASSERT_MSG(kind != 0, "Shader kind is not valid");

		shaderInfo.shaderKinds |= kind;

		const int moduleIndex = shaderInfo.modules.numElem();
		{
			const EqString shaderFileName = EqString::Format("%s-%s%s", shaderInfo.vertexLayouts[vertLayoutIdx].name, queryStr, getKindExt(kind));
			
			ShaderInfoNVRHIImpl::Module& modInfo = shaderInfo.modules.append();
			modInfo.fileIndex = shaderInfo.shaderPackFile->FindFileIndex(shaderFileName);
			modInfo.type = SHADERMODULE_SPIRV;
			modInfo.kind = static_cast<EShaderKind>(kind);
			modInfo.entryPoint = entryPointName;
		}
		{
			const int queryStrHash = StringId24(queryStr, true);
			const int entryPointStrHash = StringId24(entryPointName);
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertLayoutIdx, kind, entryPointStrHash);

			auto exIt = shaderInfo.modulesMap.find(shaderModuleId);
			ASSERT_MSG(exIt.atEnd(), "%s-%s%s module already added at idx %d (check for hash collisions)", shaderInfo.shaderName.ToCString(), queryStr, kindStr.ToCString(), exIt.value());

			shaderInfo.modulesMap.insert(shaderModuleId, moduleIndex);
		}
		++filesFound;

		if (nvrhi_preload_shaders.GetBool())
			GetOrLoadShaderModule(shaderInfo, moduleIndex);
	}

	// we need to validate references so collect refs in second pass
	int refIdx = 0;
	for (const KVSection* itemSec : fileListSec->Keys("ref"))
	{
		int vertLayoutIdx = -1;
		EqStringRef kindStr;
		EqStringRef entryPointName;
		EqStringRef queryStr;
		int refSpvIndex = -1;
		if (itemSec->GetValues(vertLayoutIdx, kindStr, entryPointName, queryStr, refSpvIndex) < 5)
		{
			ASSERT_FAIL("Shader %s 'ref' does not have 5 values (old shader version?)");
			break;
		}

		const int kind = getKind(kindStr);

		ASSERT_MSG(kind != 0, "Shader kind is not valid");
		const int queryStrHash = StringId24(queryStr, true);
		const int entryPointStrHash = StringId24(entryPointName);
		const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertLayoutIdx, kind, entryPointStrHash);
		ASSERT_MSG(shaderInfo.modules[refSpvIndex].kind == static_cast<EShaderKind>(kind), "%s ref %d (%s-%s) points to invalid shader kind", shaderInfo.shaderName.ToCString(), refSpvIndex, kindStr, queryStr);

		auto exIt = shaderInfo.modulesMap.find(shaderModuleId);
		if (!exIt.atEnd())
		{
			ASSERT_FAIL("%s %s-%s module reference already added at idx %d (check for hash collisions)", shaderInfo.shaderName.ToCString(), kindStr, queryStr, exIt.value());
		}

		shaderInfo.modulesMap.insert(shaderModuleId, refSpvIndex);
		++refIdx;
	}

	shaderPackFile = nullptr;

	return filesFound;
}

void CNVRHIRenderAPI::PrintAPIInfo() const
{
	Msg("ShaderAPI: WGPURenderAPI\n");

	Msg("  Maximum texture anisotropy: %d\n", m_caps.maxTextureAnisotropicLevel);
	Msg("  Maximum drawable textures: %d\n", m_caps.maxTextureUnits);
	Msg("  Maximum vertex textures: %d\n", m_caps.maxVertexTextureUnits);
	Msg("  Maximum texture size: %d x %d\n", m_caps.maxTextureSize, m_caps.maxTextureSize);

	MsgInfo("------ Loaded textures ------\n");

	CScopedMutex scoped(g_sapi_TextureMutex);
	for (auto it = m_TextureList.begin(); !it.atEnd(); ++it)
	{
		CNVRHITexture* pTexture = static_cast<CNVRHITexture*>(*it);
		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(), pTexture->GetHeight());
	}
}

bool CNVRHIRenderAPI::IsDeviceActive() const
{
	return !m_deviceLost;
}

IVertexFormat* CNVRHIRenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CNVRHIVertexFormat(name, formatDesc);
	m_VFList.append(pVF);
	return pVF;
}

// Destroy vertex format
void CNVRHIRenderAPI::DestroyVertexFormat(IVertexFormat* pFormat)
{
	if (m_VFList.fastRemove(pFormat))
		delete pFormat;
}

//-------------------------------------------------------------
// Textures

ITexturePtr CNVRHIRenderAPI::CreateTextureResource(const char* pszName)
{
	CRefPtr<CNVRHITexture> texture = CRefPtr_new(CNVRHITexture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}

// It will add new rendertarget
ITexturePtr	CNVRHIRenderAPI::CreateRenderTarget(const TextureDesc& targetDesc)
{
	CRefPtr<CNVRHITexture> texture = CRefPtr_new(CNVRHITexture);
	texture->SetName(targetDesc.name);
	texture->SetFlags(targetDesc.flags | TEXFLAG_RENDERTARGET);
	texture->SetFormat(targetDesc.format);
	texture->SetSamplerState(targetDesc.sampler);
	texture->m_imgType = (targetDesc.flags & TEXFLAG_CUBEMAP) ? IMAGE_TYPE_CUBE : IMAGE_TYPE_2D;

	DevMsg(DEVMSG_RENDER, "Creating render target %s\n", targetDesc.name.ToCString());

	ResizeRenderTarget(texture, targetDesc.size, targetDesc.mipmapCount, targetDesc.sampleCount);

	if (!texture->m_rhiTexture) 
		return nullptr;

	if (!(targetDesc.flags & TEXFLAG_TRANSIENT))
	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(texture);
		m_TextureList.insert(texture->m_nameHash, texture);
	}

	return ITexturePtr(texture);
}

void CNVRHIRenderAPI::ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount, int sampleCount)
{
	CNVRHITexture* texture = static_cast<CNVRHITexture*>(renderTarget);
	if (!texture)
		return;

	if (texture->GetWidth() == newSize.width && 
		texture->GetHeight() == newSize.height && 
		texture->GetArraySize() == newSize.arraySize &&
		texture->GetMipCount() == mipmapCount &&
		texture->GetSampleCount() == sampleCount)
		return;

	if (!(texture->GetFlags() & TEXFLAG_RENDERTARGET))
	{
		ASSERT_FAIL("Must be a rendertarget");
		return;
	}

	DevMsg(DEVMSG_RENDER, "Resize render target %s (%dx%d -> %dx%d)\n", texture->GetName(), texture->GetWidth(), texture->GetHeight(), newSize.width, newSize.height);

	const int flags = texture->GetFlags();

	const bool isArray = newSize.arraySize > 1;
	const bool isCubeMap = (flags & TEXFLAG_CUBEMAP);

	texture->SetDimensions(newSize.width, newSize.height, newSize.arraySize);
	texture->SetMipCount(mipmapCount);
	texture->SetSampleCount(sampleCount);
	texture->Release();

	auto rhiTextureDesc = nvrhi::TextureDesc()
		.setMipLevels(mipmapCount)
		.setSampleCount(sampleCount)
		.setIsUAV((flags & TEXFLAG_STORAGE) != 0)
		.setFormat(GetNVRHITextureFormat(texture->GetFormat()));

	const int arrayLayerCount = isCubeMap ? ITexture::CubeArraySlice(0, newSize.arraySize) : newSize.arraySize;
	rhiTextureDesc
		.setWidth((uint)newSize.width)
		.setHeight((uint)newSize.height)
		.setArraySize((uint)newSize.arraySize);

	if (flags & TEXFLAG_CUBEMAP)
	{
		rhiTextureDesc.dimension = (newSize.arraySize > 1) ? nvrhi::TextureDimension::TextureCubeArray : nvrhi::TextureDimension::TextureCube;
	}
	else
	{
		rhiTextureDesc.dimension = (newSize.arraySize > 1) ? nvrhi::TextureDimension::Texture2DArray : nvrhi::TextureDimension::Texture2D;

		// TODO: depth and nvrhi::TextureDimension::Texture3D
	}

	if (rhiTextureDesc.format == nvrhi::Format::UNKNOWN)
	{
		MsgError("Invalid or unsupported texture format %d\n", texture->GetFormat());
		return;
	}

	nvrhi::TextureHandle rhiTexture = m_rhiDevice->createTexture(rhiTextureDesc);
	if (!rhiTexture)
	{
		ErrorMsg("Failed to create render target %s\n", texture->GetName());
		return;
	}

	texture->m_rhiTexture = rhiTexture;

	// add default view
	// create default texture view
	{
		auto rhiDefaultTexViewDesc = nvrhi::TextureSubresourceSet()
			.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
			.setNumArraySlices(nvrhi::TextureSubresourceSet::AllArraySlices);
		texture->m_rhiViews.append(rhiDefaultTexViewDesc);
	}

	// FIXME: need some kind of better table and only by request 
	// or it is going to be ridiculously large
	if (isCubeMap)
	{
		// add individual cubemap views
		for (int slice = 0; slice < newSize.arraySize; ++slice)
		{
			for (int i = 0; i < 6; ++i)
			{
				auto rhiTexViewDesc = nvrhi::TextureSubresourceSet()
					.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
					.setArraySlices(ITexture::CubeArraySlice(i, slice), 1);
				texture->m_rhiViews.append(rhiTexViewDesc);
			}
		}
	}
	else if(isArray)
	{
		// add array views
		for (int i = 0; i < newSize.arraySize; ++i)
		{
			auto rhiTexViewDesc = nvrhi::TextureSubresourceSet()
				.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
				.setArraySlices(i, 1);
			texture->m_rhiViews.append(rhiTexViewDesc);
		}
	}
}

//-------------------------------------------------------------
// Pipeline management

IGPUBufferPtr CNVRHIRenderAPI::CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name) const
{
	CRefPtr<CNVRHIBuffer> buffer = CRefPtr_new(CNVRHIBuffer, bufferInfo, bufferUsageFlags, name);
	//TODO: buffer->IsValid();

	return IGPUBufferPtr(buffer);
}

IGPUPipelineLayoutPtr CNVRHIRenderAPI::CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const
{
	CRefPtr<CNVRHIPipelineLayout> pipelineLayout = CRefPtr_new(CNVRHIPipelineLayout);
	pipelineLayout->m_dbgName = layoutDesc.name;

	// Pipeline layout and bind group layout
	// are also objects of IGPURenderPipeline
	// There are 3 distinctive bind groups or buffers as our MatSystem design defines:
	//		- Material Constant Properties (static buffer)
	//		- Material Proxy Properties (buffers of these group updated every frame)
	//		- Scene Properties (camera, transform, fog, clip planes)

	int bindGroupIndex = 0;
	for(const BindGroupLayoutDesc& bindGroupDesc : layoutDesc.bindGroups)
	{
		auto rhiBindingLayoutDesc = nvrhi::BindingLayoutDesc()
			.setRegisterSpace(bindGroupIndex);

		int rhiShaderTypeVisbility = 0;
		for(const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
		{
			const bool isSRV = (entry.visibility & (SHADERKIND_VERTEX | SHADERKIND_FRAGMENT));
			const bool isUAV = (entry.visibility & (SHADERKIND_COMPUTE));

			if (entry.visibility & SHADERKIND_VERTEX)	rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Vertex);
			if (entry.visibility & SHADERKIND_FRAGMENT) rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Pixel);
			if (entry.visibility & SHADERKIND_COMPUTE)	rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Compute);

			switch (entry.type)
			{
				case BINDENTRY_BUFFER:
					if (isSRV) rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(entry.binding));
					if (isUAV) rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(entry.binding));
					break;
				case BINDENTRY_SAMPLER:
					rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(entry.binding));
					break;
				case BINDENTRY_TEXTURE:
					if (isSRV) rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(entry.binding));
					if (isUAV) rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(entry.binding));
					break;
				case BINDENTRY_STORAGETEXTURE:
					if (isUAV) rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(entry.binding));
					break;
			}
		}
		rhiBindingLayoutDesc.setVisibility(static_cast<nvrhi::ShaderType>(rhiShaderTypeVisbility));

		nvrhi::BindingLayoutHandle rhiBindingLayout = m_rhiDevice->createBindingLayout(rhiBindingLayoutDesc);
		if (!rhiBindingLayout)
			return nullptr;

		pipelineLayout->m_rhiBindingLayout[pipelineLayout] = rhiBindingLayout;
		++bindGroupIndex;
	}

	return IGPUPipelineLayoutPtr(pipelineLayout);
}

static void FillWGPUBindGroupEntries(WGPUDevice rhiDevice, const BindGroupDesc& bindGroupDesc, Array<WGPUBindGroupEntry>& rhiBindGroupEntryList)
{
	for (const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		WGPUBindGroupEntry rhiBindGroupEntryDesc = {};
		rhiBindGroupEntryDesc.binding = bindGroupEntry.binding;
		switch (bindGroupEntry.type)
		{
		case BINDENTRY_BUFFER:
		{
			CWGPUBuffer* buffer = static_cast<CWGPUBuffer*>(bindGroupEntry.buffer.buffer.Ptr());
			if (buffer)
				rhiBindGroupEntryDesc.buffer = buffer->GetWGPUBuffer();
			else
				ASSERT_FAIL("NULL buffer for bindGroup %d binding %d", bindGroupDesc.groupIdx, bindGroupEntry.binding);

			rhiBindGroupEntryDesc.size = bindGroupEntry.buffer.size < 0 ? WGPU_WHOLE_SIZE : bindGroupEntry.buffer.size;
			rhiBindGroupEntryDesc.offset = bindGroupEntry.buffer.offset;
			break;
		}
		case BINDENTRY_SAMPLER:
		{
			WGPUSamplerDescriptor rhiSamplerDesc = {};
			FillWGPUSamplerDescriptor(bindGroupEntry.sampler, rhiSamplerDesc);

			ASSERT(bindGroupEntry.sampler.maxAnisotropy > 0);

			rhiBindGroupEntryDesc.sampler = wgpuDeviceCreateSampler(rhiDevice, &rhiSamplerDesc);
			wgpuSamplerAddRef(rhiBindGroupEntryDesc.sampler);
			break;
		}
		case BINDENTRY_STORAGETEXTURE:
		case BINDENTRY_TEXTURE:
			CWGPUTexture* texture = static_cast<CWGPUTexture*>(bindGroupEntry.texture.texture.Ptr());

			// NOTE: animated textures aren't that supported, so it would need array lookup through the shader
			if (texture)
			{
				ASSERT_MSG(texture->GetWGPUTextureViewCount(), "Texture '%s' has no views", texture->GetName());
				rhiBindGroupEntryDesc.textureView = texture->GetWGPUTextureView(bindGroupEntry.texture.arraySlice);
			}
			else
				ASSERT_FAIL("NULL texture for bindGroup %d binding %d", bindGroupDesc.groupIdx, bindGroupEntry.binding);
			break;
		}

		rhiBindGroupEntryList.append(rhiBindGroupEntryDesc);
	}

}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPUPipelineLayout* layoutDesc, const BindGroupDesc& bindGroupDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	const CNVRHIPipelineLayout* pipelineLayout = static_cast<const CNVRHIPipelineLayout*>(layoutDesc);

	const Array<WGPUBindGroupLayout>& rhiLayout = pipelineLayout->m_rhiBindGroupLayout;
	if (!rhiLayout.inRange(bindGroupDesc.groupIdx))
		return nullptr;

	Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	WGPUBindGroupDescriptor rhiBindGroupDesc = {};

	// samplers are created in FillWGPUBindGroupEntries
	defer{
		for (WGPUBindGroupEntry& entry : rhiBindGroupEntryList)
		{
			if (entry.sampler)
				wgpuSamplerRelease(entry.sampler);
		}
	};

	FillWGPUBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiBindGroupEntryList);
	
	rhiBindGroupDesc.label = _WSTR(bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr);
	rhiBindGroupDesc.layout = rhiLayout[bindGroupDesc.groupIdx];
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;
	
	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_rhiBindingSet = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!renderPipeline)
	{
		ASSERT_FAIL("renderPipeline is null");
		return nullptr;
	}

	Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	WGPUBindGroupDescriptor rhiBindGroupDesc = {};

	// samplers are created in FillWGPUBindGroupEntries
	defer{
		for (WGPUBindGroupEntry& entry : rhiBindGroupEntryList)
		{
			if (entry.sampler)
				wgpuSamplerRelease(entry.sampler);
		}
		wgpuBindGroupLayoutRelease(rhiBindGroupDesc.layout);
	};

	FillWGPUBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiBindGroupEntryList);

	rhiBindGroupDesc.label = _WSTR(bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr);
	rhiBindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(static_cast<const CNVRHIRenderPipeline*>(renderPipeline)->m_rhiRenderPipeline, bindGroupDesc.groupIdx);
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;

	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_rhiBindingSet = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!computePipeline)
	{
		ASSERT_FAIL("computePipeline is null");
		return nullptr;
	}

	Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	WGPUBindGroupDescriptor rhiBindGroupDesc = {};

	// samplers are created in FillWGPUBindGroupEntries
	defer{
		for (WGPUBindGroupEntry& entry : rhiBindGroupEntryList)
		{
			if (entry.sampler)
				wgpuSamplerRelease(entry.sampler);
		}
		wgpuBindGroupLayoutRelease(rhiBindGroupDesc.layout);
	};

	FillWGPUBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiBindGroupEntryList);

	rhiBindGroupDesc.label = _WSTR(bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr);
	rhiBindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(static_cast<const CWGPUComputePipeline*>(computePipeline)->m_rhiComputePipeline, bindGroupDesc.groupIdx);
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;

	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_rhiBindingSet = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

nvrhi::ShaderHandle CNVRHIRenderAPI::GetOrLoadShaderModule(const ShaderInfoNVRHIImpl& shaderInfo, int shaderModuleIdx) const
{
	ShaderInfoNVRHIImpl::Module& mod = const_cast<ShaderInfoNVRHIImpl::Module&>(shaderInfo.modules[shaderModuleIdx]);
	if (mod.rhiModule)
		return mod.rhiModule;

	CMemoryStream shaderData(PP_SL);
	{
		IFilePtr shaderFile = shaderInfo.shaderPackFile->Open(mod.fileIndex, VS_OPEN_READ);
		if (!shaderFile)
		{
			ASSERT_FAIL("Unable to open file in shader package!");
			return nullptr;
		}

		shaderData.Open(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, shaderFile->GetSize());
		shaderData.AppendStream(shaderFile);
	}

	const EqString shaderModuleName = EqString::Format("%s-%d", shaderInfo.shaderName.ToCString(), shaderModuleIdx);

	nvrhi::ShaderHandle rhiShaderModule = nullptr;
	if (mod.type == SHADERMODULE_SPIRV)
	{
		nvrhi::ShaderDesc rhiShaderDesc{};
		rhiShaderDesc.debugName = shaderModuleName;
		rhiShaderDesc.entryName = mod.entryPoint.ToCString();

		switch (mod.kind)
		{
		case SHADERKIND_VERTEX:
			rhiShaderDesc.shaderType = nvrhi::ShaderType::Vertex;
			break;
		case SHADERKIND_FRAGMENT:
			rhiShaderDesc.shaderType = nvrhi::ShaderType::Pixel;
			break;
		case SHADERKIND_COMPUTE:
			rhiShaderDesc.shaderType = nvrhi::ShaderType::Compute;
			break;
		}

		rhiShaderModule = m_rhiDevice->createShader(rhiShaderDesc, shaderData.GetBasePointer(), shaderData.GetSize());
	}
	else
	{
		ASSERT_FAIL("Shader module type %d (found in package %s) not supported", mod.type, shaderInfo.shaderName.ToCString());
		return nullptr;
	}
	
	if (!rhiShaderModule)
	{
		MsgError("Can't create shader module %s!\n", shaderModuleName.ToCString());
		return nullptr;
	}
	mod.rhiModule = rhiShaderModule;

	return rhiShaderModule;
}

void CNVRHIRenderAPI::LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines, const char* entryPointName) const
{
	const int shaderNameHash = StringId24(shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		MsgError("LoadShaderModules: unknown shader '%s' specified\n", shaderName);
		return;
	}

	const ShaderInfoNVRHIImpl& shaderInfo = *shaderIt;
	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(defines, queryStrHash))
	{
		MsgError("LoadShaderModules: unknown defines in query for shader '%s'\n", shaderName);
		return;
	}

	const int entryPointStrHash = StringId24(entryPointName);

	for (int i = 0; i < shaderInfo.vertexLayouts.numElem(); ++i)
	{
		const ShaderInfoNVRHIImpl::VertLayout& layout = shaderInfo.vertexLayouts[i];
		if (layout.aliasOf != -1)
			continue;

		if(shaderInfo.shaderKinds & SHADERKIND_FRAGMENT)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_FRAGMENT, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_VERTEX)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_VERTEX, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_COMPUTE)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_COMPUTE, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
	}
}

IGPURenderPipelinePtr CNVRHIRenderAPI::CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout) const
{
	PROF_EVENT("CWGPURenderAPI::CreateRenderPipeline");

	const int shaderNameHash = StringId24(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Render pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfoNVRHIImpl& shaderInfo = *shaderIt;
	ASSERT_MSG(shaderInfo.shaderName == pipelineDesc.shaderName, "Shader name mismatch, requested '%s' got '%s' (hash collision?)", pipelineDesc.shaderName.ToCString(), shaderInfo.shaderName.ToCString());

	if (!(shaderInfo.shaderKinds & (SHADERKIND_VERTEX | SHADERKIND_FRAGMENT)))
	{
		ASSERT_FAIL("Shader %s must have Vertex or Fragment kind", shaderInfo.shaderName.ToCString());
		return nullptr;
	}

	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(pipelineDesc.shaderQuery, queryStrHash))
	{
		ASSERT_FAIL("Render pipeline has unknown defines in query for shader '%s'", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	int vertexLayoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoNVRHIImpl::VertLayout& layout) {
		return layout.nameHash == pipelineDesc.shaderVertexLayoutId;
	});

	if (vertexLayoutIdx == -1)
	{
		ASSERT_FAIL("Render pipeline %s has unknown vertex layout specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}
	if (shaderInfo.vertexLayouts[vertexLayoutIdx].aliasOf != -1)
		vertexLayoutIdx = shaderInfo.vertexLayouts[vertexLayoutIdx].aliasOf;

	// 1. The pipeline almost fully constructed by the MatSystemShader
	//    except the primitive state, which is:
	//    - topology
	//    - cull mode
	//    - strip index format for primitive restart
	// 
	//    The rest is happily encoded by the render pass
	//    - scissor
	//    - viewport
	//
	// 2. MatSystemShader must know which vertex format (layout) it does support
	//    so for each vertex (DynMeshVertex, EGFVertex, LevelVertex)
	//    the pipeline is generated.
	//
	// 3. When building DrawCall for command buffer, MatSystem must decide
	//    which pipeline to use from shader. 
	//    For example, RenderDrawCmd settings that selecting pipeline:
	//    - vertexFormat
	//    - primitiveTopology
	//

	// pipeline-overridable constants
	Array<WGPUConstantEntry> rhiVertexPipelineConstants(PP_SL);
	Array<WGPUConstantEntry> rhiFragmentPipelineConstants(PP_SL);

	for (const PipelineConst& constant : pipelineDesc.vertex.constants)
		rhiVertexPipelineConstants.append({ nullptr, _WSTR(constant.name), constant.value});

	for (const PipelineConst& constant : pipelineDesc.fragment.constants)
		rhiFragmentPipelineConstants.append({ nullptr, _WSTR(constant.name), constant.value });

	WGPURenderPipelineDescriptor rhiRenderPipelineDesc = {};
	if (pipelineLayout)
	{
		rhiRenderPipelineDesc.layout = static_cast<const CWGPUPipelineLayout*>(pipelineLayout)->m_rhiPipelineLayout;
	}

	// Setup vertex pipeline
	// Required
	Array<WGPUVertexAttribute> rhiVertexAttribList(PP_SL);
	Array<WGPUVertexBufferLayout> rhiVertexBufferLayoutList(PP_SL);
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");

		for(const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			const int firstVertexAttrib = rhiVertexAttribList.numElem();
			for(const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				if (attrib.format == ATTRIBUTEFORMAT_NONE)
					continue;

				WGPUVertexAttribute vertAttr = {};
				vertAttr.format = g_wgpuVertexFormats[attrib.format][attrib.count-1];
				vertAttr.offset = attrib.offset;
				vertAttr.shaderLocation = attrib.location;
				rhiVertexAttribList.append(vertAttr);
			}

			WGPUVertexBufferLayout rhiVertexBufferLayout = {};
			rhiVertexBufferLayout.arrayStride = vertexLayout.stride;
			rhiVertexBufferLayout.attributeCount = rhiVertexAttribList.numElem() - firstVertexAttrib;
			rhiVertexBufferLayout.attributes = &rhiVertexAttribList[firstVertexAttrib];
			rhiVertexBufferLayout.stepMode = g_wgpuVertexStepMode[vertexLayout.stepMode];
			rhiVertexBufferLayoutList.append(rhiVertexBufferLayout);
		}

		WGPUShaderModule rhiVertexShaderModule = nullptr;
		{
			const int entryPointStrHash = StringId24(pipelineDesc.vertex.shaderEntryPoint);
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

			if (!itShaderModuleId.atEnd())
			{
				EqString queryStr;
				for (const EqString& str : pipelineDesc.shaderQuery)
				{
					if (queryStr.Length())
						queryStr.Append("|");
					queryStr.Append(str);
				}
				ASSERT_MSG(shaderInfo.modules[*itShaderModuleId].kind == SHADERKIND_VERTEX, "Incorrect shader kind for %s %s in shader package %s", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr.ToCString(), pipelineDesc.shaderName.ToCString());
				rhiVertexShaderModule = GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
			}
		}

		WGPUVertexState& rhiVertexState = rhiRenderPipelineDesc.vertex;
		rhiVertexState.module = rhiVertexShaderModule;
		rhiVertexState.entryPoint = _WSTR(pipelineDesc.vertex.shaderEntryPoint);
		rhiVertexState.bufferCount = rhiVertexBufferLayoutList.numElem();
		rhiVertexState.buffers = rhiVertexBufferLayoutList.ptr();
		rhiVertexState.constants = rhiVertexPipelineConstants.ptr();
		rhiVertexState.constantCount = rhiVertexPipelineConstants.numElem();

		if (!rhiVertexState.module)
		{
			EqString queryStr;
			for (const EqString& str : pipelineDesc.shaderQuery)
			{
				if (queryStr.Length())
					queryStr.Append("|");
				queryStr.Append(str);
			}

			ASSERT_FAIL("No vertex shader module found for %s %s in shader package %s", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr.ToCString(), pipelineDesc.shaderName.ToCString());
			return nullptr;
		}
	}
	
	// Depth state
	// Optional when depth read = false
	WGPUDepthStencilState rhiDepthStencil = {};
	if (pipelineDesc.depthStencil.format != FORMAT_NONE)
	{
		rhiDepthStencil.format = GetWGPUTextureFormat(pipelineDesc.depthStencil.format);
		rhiDepthStencil.depthWriteEnabled = (WGPUOptionalBool)pipelineDesc.depthStencil.depthWrite;
		rhiDepthStencil.depthCompare = pipelineDesc.depthStencil.depthTest ? g_wgpuCompareFunc[pipelineDesc.depthStencil.depthFunc] : WGPUCompareFunction_Always;
		rhiDepthStencil.stencilReadMask = pipelineDesc.depthStencil.stencilMask;
		rhiDepthStencil.stencilWriteMask = pipelineDesc.depthStencil.stencilWriteMask;
		rhiDepthStencil.depthBias = pipelineDesc.depthStencil.depthBias;
		rhiDepthStencil.depthBiasSlopeScale = pipelineDesc.depthStencil.depthBiasSlopeScale;
		rhiDepthStencil.depthBiasClamp = 0; // TODO

		// back
		rhiDepthStencil.stencilBack.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilBack.compareFunc];
		rhiDepthStencil.stencilBack.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.failOp];
		rhiDepthStencil.stencilBack.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.depthFailOp];
		rhiDepthStencil.stencilBack.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.passOp];

		// front
		rhiDepthStencil.stencilFront.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilFront.compareFunc];
		rhiDepthStencil.stencilFront.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.failOp];
		rhiDepthStencil.stencilFront.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.depthFailOp];
		rhiDepthStencil.stencilFront.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.passOp];
		rhiRenderPipelineDesc.depthStencil = &rhiDepthStencil;
	}

	// Setup fragment pipeline
	// Fragment state
	// When opted out, requires rhiDepthStencil state
	WGPUFragmentState rhiFragmentState = {};
	FixedArray<WGPUColorTargetState, MAX_RENDERTARGETS> rhiColorTargets;
	FixedArray<WGPUBlendState, MAX_RENDERTARGETS> rhiColorTargetBlends;
	if(pipelineDesc.fragment.shaderEntryPoint.Length())
	{
		for(const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
		{
			WGPUColorTargetState rhiColorTarget = {};

			if(target.blendEnable)
			{
				WGPUBlendState rhiBlend = {};
				FillWGPUBlendComponent(target.colorBlend, rhiBlend.color);
				FillWGPUBlendComponent(target.alphaBlend, rhiBlend.alpha);
				rhiColorTargetBlends.append(rhiBlend);

				rhiColorTarget.blend = &rhiColorTargetBlends.back();
			}

			rhiColorTarget.format = GetWGPUTextureFormat(target.format);
			rhiColorTarget.writeMask = target.writeMask;
			rhiColorTargets.append(rhiColorTarget);
		}

		WGPUShaderModule rhiFragmentShaderModule = nullptr; // TODO: fetch from cache of fragment modules?
		{
			const int entryPointStrHash = StringId24(pipelineDesc.fragment.shaderEntryPoint);
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

			if (!itShaderModuleId.atEnd())
			{
				EqString queryStr;
				for (const EqString& str : pipelineDesc.shaderQuery)
				{
					if (queryStr.Length())
						queryStr.Append("|");
					queryStr.Append(str);
				}
				ASSERT_MSG(shaderInfo.modules[*itShaderModuleId].kind == SHADERKIND_FRAGMENT, "Incorrect shader kind for %s %s in shader package %s", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr.ToCString(), pipelineDesc.shaderName.ToCString());
				rhiFragmentShaderModule = GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
			}
		}

		rhiFragmentState.module = rhiFragmentShaderModule;
		rhiFragmentState.entryPoint = _WSTR(pipelineDesc.fragment.shaderEntryPoint);
		rhiFragmentState.targetCount = rhiColorTargets.numElem();
		rhiFragmentState.targets = rhiColorTargets.ptr();
		rhiFragmentState.constants = rhiFragmentPipelineConstants.ptr();
		rhiFragmentState.constantCount = rhiFragmentPipelineConstants.numElem();

		if(!rhiFragmentState.module)
		{
			EqString queryStr;
			for (const EqString& str : pipelineDesc.shaderQuery)
			{
				if (queryStr.Length())
					queryStr.Append("|");
				queryStr.Append(str);
			}

			ASSERT_FAIL("No fragment shader module found for %s %s in shader package %s", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr.ToCString(), pipelineDesc.shaderName.ToCString());
			return nullptr;
		}

		rhiRenderPipelineDesc.fragment = &rhiFragmentState;
	}

	if (!rhiRenderPipelineDesc.depthStencil && !rhiRenderPipelineDesc.fragment)
	{
		ASSERT_FAIL("Render pipeline requires either depthStencil or fragment states (or both)");
		return nullptr;
	}

	// Multisampling
	rhiRenderPipelineDesc.multisample.count = pipelineDesc.multiSample.count;
	rhiRenderPipelineDesc.multisample.mask = pipelineDesc.multiSample.mask;
	rhiRenderPipelineDesc.multisample.alphaToCoverageEnabled = pipelineDesc.multiSample.alphaToCoverage;

	// Primitive toplogy
	rhiRenderPipelineDesc.primitive.frontFace = WGPUFrontFace_CW; // for now always, TODO
	rhiRenderPipelineDesc.primitive.cullMode = g_wgpuCullMode[pipelineDesc.primitive.cullMode];
	rhiRenderPipelineDesc.primitive.topology = g_wgpuPrimTopology[pipelineDesc.primitive.topology];
	rhiRenderPipelineDesc.primitive.stripIndexFormat = g_wgpuStripIndexFormat[pipelineDesc.primitive.stripIndex];

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString());
	rhiRenderPipelineDesc.label = _WSTR(pipelineName);

	{
		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		WGPURenderPipeline rhiRenderPipeline = wgpuDeviceCreateRenderPipeline(m_rhiDevice, &rhiRenderPipelineDesc);
		if (!rhiRenderPipeline)
		{
			ASSERT_FAIL("Render pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CNVRHIRenderPipeline> renderPipeline = CRefPtr_new(CNVRHIRenderPipeline);
		renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;

		return IGPURenderPipelinePtr(renderPipeline);
	}
}

IGPUComputePipelinePtr CNVRHIRenderAPI::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout) const
{
	const int shaderNameHash = StringId24(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Compute pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfoNVRHIImpl& shaderInfo = *shaderIt;
	ASSERT_MSG(shaderInfo.shaderName == pipelineDesc.shaderName, "Shader name mismatch, requested '%s' got '%s' (hash collision?)", pipelineDesc.shaderName.ToCString(), shaderInfo.shaderName.ToCString());

	if (!(shaderInfo.shaderKinds & SHADERKIND_COMPUTE))
	{
		ASSERT_FAIL("Shader %s must have Compute kind", shaderInfo.shaderName.ToCString());
		return nullptr;
	}

	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(pipelineDesc.shaderQuery, queryStrHash))
	{
		ASSERT_FAIL("Compute pipeline has unknown defines in query for shader '%s'", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	int layoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoNVRHIImpl::VertLayout& layout) {
		return layout.nameHash == pipelineDesc.shaderLayoutId;
	});

	if (layoutIdx == -1)
	{
		ASSERT_FAIL("Compute pipeline %s has unknown layout id %d", pipelineDesc.shaderName.ToCString(), pipelineDesc.shaderLayoutId);
		return nullptr;
	}
	if (shaderInfo.vertexLayouts[layoutIdx].aliasOf != -1)
		layoutIdx = shaderInfo.vertexLayouts[layoutIdx].aliasOf;

	nvrhi::ShaderHandle rhiComputeShaderModule = nullptr;
	{
		const int entryPointStrHash = StringId24(pipelineDesc.shaderEntryPoint);
		const uint shaderModuleId = PackShaderModuleId(queryStrHash, layoutIdx, SHADERKIND_COMPUTE, entryPointStrHash);
		auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

		if (!itShaderModuleId.atEnd())
		{
			EqString queryStr;
			for (const EqString& str : pipelineDesc.shaderQuery)
			{
				if (queryStr.Length())
					queryStr.Append("|");
				queryStr.Append(str);
			}
			ASSERT_MSG(shaderInfo.modules[*itShaderModuleId].kind == SHADERKIND_COMPUTE, "Incorrect shader kind for %s %s in shader package %s", shaderInfo.vertexLayouts[layoutIdx].name.ToCString(), queryStr.ToCString(), pipelineDesc.shaderName.ToCString());
			rhiComputeShaderModule = GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
	}

	const CNVRHIPipelineLayout* pipelineLayoutImpl = static_cast<const CNVRHIPipelineLayout*>(pipelineLayout);

	auto rhiComputePipelineDesc = nvrhi::ComputePipelineDesc()
		.setComputeShader(rhiComputeShaderModule);

	for (nvrhi::BindingLayoutHandle& rhiLayout : pipelineLayoutImpl->m_rhiBindingLayout)
		rhiComputePipelineDesc.addBindingLayout(rhiLayout);

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[layoutIdx].name.ToCString());

	{
		PROF_EVENT(EqString::Format("CreateComputePipeline for %s", pipelineName.ToCString()));
		nvrhi::ComputePipelineHandle rhiComputePipeline = m_rhiDevice->createComputePipeline(rhiComputePipelineDesc);
		if (!rhiComputePipeline)
		{
			ASSERT_FAIL("Compute pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CNVRHIComputePipeline> renderPipeline = CRefPtr_new(CNVRHIComputePipeline);
		renderPipeline->m_rhiComputePipeline = rhiComputePipeline;
		renderPipeline->m_dbgName = pipelineName;

		return IGPUComputePipelinePtr(renderPipeline);
	}
}

IGPUCommandRecorderPtr CNVRHIRenderAPI::CreateCommandRecorder(const char* name, void* userData) const
{
	nvrhi::CommandListHandle rhiCommandList = m_rhiDevice->createCommandList();
	rhiCommandList->open();

	CRefPtr<CNVRHICommandRecorder> commandRecorder = CRefPtr_new(CNVRHICommandRecorder);
	commandRecorder->m_dbgLabel = name;
	commandRecorder->m_rhiCommandList = rhiCommandList;
	commandRecorder->m_userData = userData;

	return IGPUCommandRecorderPtr(commandRecorder);
}

IGPURenderPassRecorderPtr CNVRHIRenderAPI::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	nvrhi::CommandListHandle rhiCommandList = m_rhiDevice->createCommandList();
	rhiCommandList->open();

	CRefPtr<CNVRHIRenderPassRecorder> renderPass = CRefPtr_new(CNVRHIRenderPassRecorder, rhiCommandList, userData);
	renderPass->InternalBeginRenderPass(renderPassDesc);

	return IGPURenderPassRecorderPtr(renderPass);
}

IGPUComputePassRecorderPtr CNVRHIRenderAPI::BeginComputePass(const char* name, void* userData) const
{
	nvrhi::CommandListHandle rhiCommandList = m_rhiDevice->createCommandList();
	rhiCommandList->open();

	CRefPtr<CNVRHIComputePassRecorder> renderPass = CRefPtr_new(CNVRHIComputePassRecorder, rhiCommandList, userData, name);
	return IGPUComputePassRecorderPtr(renderPass);
}

void CNVRHIRenderAPI::SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	PROF_EVENT_F();

	Array<nvrhi::ICommandList*> rhiSubmitBuffers(PP_SL);
	rhiSubmitBuffers.reserve(cmdBuffers.numElem());
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;

		const CNVRHICommandBuffer* bufferImpl = static_cast<const CNVRHICommandBuffer*>(cmdBuffer);
		ASSERT(bufferImpl->m_rhiCommandList);
		rhiSubmitBuffers.append(bufferImpl->m_rhiCommandList);
	}

	uint64_t cmdListInstance = m_rhiDevice->executeCommandLists(rhiSubmitBuffers.ptr(), rhiSubmitBuffers.numElem());
}


Future<bool> CNVRHIRenderAPI::SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	Array<nvrhi::ICommandList*> rhiSubmitBuffers(PP_SL);
	rhiSubmitBuffers.reserve(cmdBuffers.numElem());
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;

		const CNVRHICommandBuffer* bufferImpl = static_cast<const CNVRHICommandBuffer*>(cmdBuffer);
		ASSERT(bufferImpl->m_rhiCommandList);
		rhiSubmitBuffers.append(bufferImpl->m_rhiCommandList);
	}

	if (!rhiSubmitBuffers.numElem())
		return Future<bool>::Succeed(true);

	const uint64_t lastSubmitInstance = m_rhiDevice->executeCommandLists(rhiSubmitBuffers.ptr(), rhiSubmitBuffers.numElem());

	Promise<bool> promise;

	// TODO: proper wait
	m_rhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Graphics, lastSubmitInstance);
	promise.SetResult(true);

	return promise.CreateFuture();
}

void CNVRHIRenderAPI::Flush()
{
	m_rhiDevice->runGarbageCollection();
}
