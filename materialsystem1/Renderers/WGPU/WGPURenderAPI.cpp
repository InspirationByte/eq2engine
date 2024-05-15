//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#include "core/IFilePackageReader.h"
#include "core/ConVar.h"
#include "imaging/ImageLoader.h"
#include "utils/KeyValues.h"

#include "WGPURenderAPI.h"
#include "WGPURenderDefs.h"
#include "WGPUStates.h"
#include "WGPUCommandRecorder.h"
#include "WGPURenderPassRecorder.h"

#include "../RenderWorker.h"
#include "WGPUComputePassRecorder.h"


#define ASSERT_DEPRECATED() // ASSERT_FAIL("Deprecated API %s", __func__)

DECLARE_CVAR(wgpu_preload_shaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);

CWGPURenderAPI CWGPURenderAPI::Instance;
IShaderAPI* g_renderAPI = &CWGPURenderAPI::Instance;

static uint PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind)
{
	return queryStrHash | (static_cast<uint>(vertexLayoutIdx) << StringHashBits) | (static_cast<uint>(kind) << (StringHashBits + 4));
}

ShaderInfoWGPUImpl::~ShaderInfoWGPUImpl()
{
	g_fileSystem->ClosePackage(shaderPackFile);
}

ShaderInfoWGPUImpl::ShaderInfoWGPUImpl(ShaderInfoWGPUImpl&& other) noexcept
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

ShaderInfoWGPUImpl& ShaderInfoWGPUImpl::operator=(ShaderInfoWGPUImpl&& other) noexcept
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

void ShaderInfoWGPUImpl::Release()
{
	for (ShaderInfoWGPUImpl::Module module : modules)
		wgpuShaderModuleRelease(module.rhiModule);
}

bool ShaderInfoWGPUImpl::GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const
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
	outHash = StringToHash(queryStr, true);
	return true;
}

//------------------------------------------

void CWGPURenderAPI::Init(const ShaderAPIParams& params)
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

		CombinePath(shaderPackPath, "shaders", fsFind.GetPath());

		shaderModCount += LoadShaderPackage(shaderPackPath);
		++shaderPackCount;
	}

	Msg("Init shader cache from %d packages, %d shader modules loaded\n", shaderPackCount, shaderModCount);
}

void CWGPURenderAPI::Shutdown()
{
	ShaderAPI_Base::Shutdown();
	m_shaderCache.clear(true);
	m_rhiDevice = nullptr;
	m_rhiQueue = nullptr;
}

int CWGPURenderAPI::LoadShaderPackage(const char* filename)
{
	IFilePackageReader* shaderPackFile = g_fileSystem->OpenPackage(filename, SP_MOD | SP_DATA);
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
		{
			m_shaderCache.remove(StringToHash(shaderInfoKvs.GetName()));
			g_fileSystem->ClosePackage(shaderPackFile);
		}
	};

	if (!strstr(filename, shaderInfoKvs.GetName()))
	{
		ASSERT_FAIL("Shader package '%s' file name doesn't match it's name '%s' in desc", filename, shaderInfoKvs.GetName());
		return 0;
	}

	auto it = m_shaderCache.find(StringToHash(shaderInfoKvs.GetName()));
	if (!it.atEnd())
	{
		ASSERT_FAIL("Shader '%s' has been already loaded from different package", shaderInfoKvs.GetName());
		return 0;
	}

	it = m_shaderCache.insert(StringToHash(shaderInfoKvs.GetName()));

	ShaderInfoWGPUImpl& shaderInfo = *it;
	shaderInfo.shaderPackFile = shaderPackFile;
	shaderInfo.shaderName = shaderInfoKvs.GetName();

	const KVSection* defines = shaderInfoKvs.FindSection("Defines");
	if (defines)
	{
		shaderInfo.defines.reserve(defines->ValueCount());
		for (KVValueIterator<EqString> it(defines); !it.atEnd(); ++it)
			shaderInfo.defines.append(it);
	}

	int shaderKinds = 0;
	for (KVValueIterator<EqString> it(shaderInfoKvs.FindSection("ShaderKinds")); !it.atEnd(); ++it)
	{
		const EqString kindName(it);
		if (kindName == "Vertex")
			shaderKinds |= SHADERKIND_VERTEX;
		else if (kindName == "Fragment")
			shaderKinds |= SHADERKIND_FRAGMENT;
		else if (kindName == "Compute")
			shaderKinds |= SHADERKIND_COMPUTE;
	}
	shaderInfo.shaderKinds = shaderKinds;

	static constexpr const char* DefaultVertexLayoutName = "Default";

	for (KVKeyIterator it(shaderInfoKvs.FindSection("VertexLayouts")); !it.atEnd(); ++it)
	{
		ShaderInfoWGPUImpl::VertLayout& layout = shaderInfo.vertexLayouts.append();
		layout.name = EqString(it);
		if (layout.name != DefaultVertexLayoutName)
			layout.nameHash = StringToHash(layout.name);
		
		if (!stricmp(KV_GetValueString(*it, 0), "aliasOf"))
		{
			layout.aliasOf = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoWGPUImpl::VertLayout& layout) {
				return layout.name == KV_GetValueString(*it, 1);
			});
		}
	}

	const KVSection* fileListSec = shaderInfoKvs.FindSection("FileList");

	int filesFound = 0;
	for (KVKeyIterator it(fileListSec, "spv"); !it.atEnd(); ++it)
	{
		const KVSection* itemSec = *it;

		EqString kindExt;
		const char* kindStr = KV_GetValueString(itemSec, 1);
		const int vertexLayoutIdx = KV_GetValueInt(itemSec);
		int kind = 0;
		{
			if (!stricmp(kindStr, "Vertex"))
			{
				kind = SHADERKIND_VERTEX;
				kindExt = ".vert";
			}
			else if (!stricmp(kindStr, "Fragment"))
			{
				kind = SHADERKIND_FRAGMENT;
				kindExt = ".frag";
			}
			else if (!stricmp(kindStr, "Compute"))
			{
				kind = SHADERKIND_COMPUTE;
				kindExt = ".comp";
			}
		}
		ASSERT_MSG(kind != 0, "Shader kind is not valid");
		const char* queryStr = KV_GetValueString(itemSec, 2);
		const int queryStrHash = StringToHash(queryStr, true);

		const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, kind);
		const EqString shaderFileName = EqString::Format("%s-%s%s", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr, kindExt.ToCString());
		const int shaderModuleFileIndex = shaderInfo.shaderPackFile->FindFileIndex(shaderFileName);
		WGPUShaderModule rhiShaderModule = nullptr;
		if (wgpu_preload_shaders.GetBool())
		{
			CMemoryStream shaderData(PP_SL);
			{
				IFilePtr shaderFile = shaderInfo.shaderPackFile->Open(shaderModuleFileIndex, VS_OPEN_READ);
				if (!shaderFile)
				{
					MsgError("Can't open shader %s in shader package!\n", shaderFileName.ToCString());
					return 0;
				}
				shaderData.Open(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, shaderFile->GetSize());
				shaderData.AppendStream(shaderFile);
			}

			rhiShaderModule = CreateShaderSPIRV(reinterpret_cast<uint32*>(shaderData.GetBasePointer()), shaderData.GetSize(), EqString::Format("%s-%s", shaderInfoKvs.GetName(), shaderFileName.ToCString()));
			if (!rhiShaderModule)
			{
				MsgError("Can't create shader module %s!\n", shaderFileName.ToCString());
				return 0;
			}
		}

		const int moduleIndex = shaderInfo.modules.append({ rhiShaderModule, static_cast<EShaderKind>(kind), shaderModuleFileIndex });
		auto exIt = shaderInfo.modulesMap.find(shaderModuleId);
		if (!exIt.atEnd())
		{
			ASSERT_FAIL("%s-%s module already added at idx %d (check for hash collisions)", shaderInfo.shaderName.ToCString(), kindStr, queryStr, exIt.value());
		}
		shaderInfo.modulesMap.insert(shaderModuleId, moduleIndex);
		++filesFound;
	}

	// we need to validate references so collect refs in second pass
	int refIdx = 0;
	for (KVKeyIterator it(fileListSec, "ref"); !it.atEnd(); ++it)
	{
		const KVSection* itemSec = *it;

		const char* kindStr = KV_GetValueString(itemSec, 1);
		const int vertexLayoutIdx = KV_GetValueInt(itemSec);
		int kind = 0;
		{
			if (!stricmp(kindStr, "Vertex"))
				kind = SHADERKIND_VERTEX;
			else if (!stricmp(kindStr, "Fragment"))
				kind = SHADERKIND_FRAGMENT;
			else if (!stricmp(kindStr, "Compute"))
				kind = SHADERKIND_COMPUTE;
		}
		ASSERT_MSG(kind != 0, "Shader kind is not valid");
		const char* queryStr = KV_GetValueString(itemSec, 2);
		const int queryStrHash = StringToHash(queryStr, true);

		const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, kind);
		const int refSpvIndex = KV_GetValueInt(itemSec, 3);

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

void CWGPURenderAPI::PrintAPIInfo() const
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
		CWGPUTexture* pTexture = static_cast<CWGPUTexture*>(*it);
		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(), pTexture->GetHeight());
	}
}

bool CWGPURenderAPI::IsDeviceActive() const
{
	return !m_deviceLost;
}

IVertexFormat* CWGPURenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CWGPUVertexFormat(name, formatDesc);
	m_VFList.append(pVF);
	return pVF;
}

// Destroy vertex format
void CWGPURenderAPI::DestroyVertexFormat(IVertexFormat* pFormat)
{
	if (m_VFList.fastRemove(pFormat))
		delete pFormat;
}

//-------------------------------------------------------------
// Textures

ITexturePtr CWGPURenderAPI::CreateTextureResource(const char* pszName)
{
	CRefPtr<CWGPUTexture> texture = CRefPtr_new(CWGPUTexture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}

// It will add new rendertarget
ITexturePtr	CWGPURenderAPI::CreateRenderTarget(const TextureDesc& targetDesc)
{
	CRefPtr<CWGPUTexture> texture = CRefPtr_new(CWGPUTexture);
	texture->SetName(targetDesc.name);
	texture->SetFlags(targetDesc.flags | TEXFLAG_RENDERTARGET);
	texture->SetFormat(targetDesc.format);
	texture->SetSamplerState(targetDesc.sampler);
	texture->m_imgType = (targetDesc.flags & TEXFLAG_CUBEMAP) ? IMAGE_TYPE_CUBE : IMAGE_TYPE_2D;

	ResizeRenderTarget(texture, targetDesc.size, targetDesc.mipmapCount, targetDesc.sampleCount);

	if (!texture->m_rhiTextures.numElem()) 
		return nullptr;

	if (!(targetDesc.flags & TEXFLAG_TRANSIENT))
	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(texture);
		m_TextureList.insert(texture->m_nameHash, texture);
	}

	return ITexturePtr(texture);
}

void CWGPURenderAPI::ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount, int sampleCount)
{
	CWGPUTexture* texture = static_cast<CWGPUTexture*>(renderTarget);
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

	texture->SetDimensions(newSize.width, newSize.height, newSize.arraySize);
	texture->SetMipCount(mipmapCount);
	texture->SetSampleCount(sampleCount);
	texture->Release();

	const int flags = texture->GetFlags();

	const bool isCubeMap = (flags & TEXFLAG_CUBEMAP);

	int texDepth = newSize.arraySize;
	if (isCubeMap)
		texDepth = 6; // TODO: CubeArray, WGPU supports it

	WGPUTextureUsageFlags rhiUsageFlags = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
	if (flags & TEXFLAG_STORAGE) rhiUsageFlags |= WGPUTextureUsage_StorageBinding;
	if (flags & TEXFLAG_COPY_SRC) rhiUsageFlags |= WGPUTextureUsage_CopySrc;
	if (flags & TEXFLAG_COPY_DST) rhiUsageFlags |= WGPUTextureUsage_CopyDst;

	WGPUTextureDescriptor rhiTextureDesc = {};
	rhiTextureDesc.label = texture->GetName();
	rhiTextureDesc.mipLevelCount = mipmapCount;
	rhiTextureDesc.size = WGPUExtent3D{ (uint)newSize.width, (uint)newSize.height, (uint)texDepth };
	rhiTextureDesc.sampleCount = sampleCount;
	rhiTextureDesc.usage = rhiUsageFlags;
	rhiTextureDesc.format = GetWGPUTextureFormat(texture->GetFormat());
	rhiTextureDesc.dimension = WGPUTextureDimension_2D;
	rhiTextureDesc.viewFormatCount = 0;
	rhiTextureDesc.viewFormats = nullptr;

	if (rhiTextureDesc.format == WGPUTextureFormat_Undefined)
	{
		MsgError("Invalid or unsupported texture format %d\n", texture->GetFormat());
		return;
	}

	WGPUTexture rhiTexture = wgpuDeviceCreateTexture(m_rhiDevice, &rhiTextureDesc);
	if (!rhiTexture)
	{
		ErrorMsg("Failed to create render target %s\n", texture->GetName());
		return;
	}

	texture->m_rhiTextures.append(rhiTexture);

	// add default view
	{
		WGPUTextureViewDescriptor rhiTexViewDesc = {};
		rhiTexViewDesc.label = rhiTextureDesc.label;
		rhiTexViewDesc.format = GetWGPUTextureFormat(texture->GetFormat());
		rhiTexViewDesc.aspect = WGPUTextureAspect_All;
		rhiTexViewDesc.arrayLayerCount = isCubeMap ? 6 : newSize.arraySize;
		rhiTexViewDesc.baseArrayLayer = 0;
		rhiTexViewDesc.baseMipLevel = 0;
		rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;

		// TODO: CubeArray
		rhiTexViewDesc.dimension = isCubeMap ? WGPUTextureViewDimension_Cube : (newSize.arraySize > 1 ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D);

		WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
		texture->m_rhiViews.append(rhiView);
	}

	// add individual cubemap views
	if (isCubeMap)
	{
		for (int i = 0; i < 6; ++i)
		{
			WGPUTextureViewDescriptor rhiTexViewDesc = {};
			rhiTexViewDesc.label = rhiTextureDesc.label;
			rhiTexViewDesc.format = GetWGPUTextureFormat(texture->GetFormat());
			rhiTexViewDesc.aspect = WGPUTextureAspect_All;
			rhiTexViewDesc.arrayLayerCount = 1;
			rhiTexViewDesc.baseArrayLayer = i;
			rhiTexViewDesc.baseMipLevel = 0;
			rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
			rhiTexViewDesc.dimension = WGPUTextureViewDimension_2D;

			WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
			texture->m_rhiViews.append(rhiView);
		}
	}
	else if(newSize.arraySize > 1)
	{
		// add array views
		for (int i = 0; i < newSize.arraySize; ++i)
		{
			WGPUTextureViewDescriptor rhiTexViewDesc = {};
			rhiTexViewDesc.label = rhiTextureDesc.label;
			rhiTexViewDesc.format = GetWGPUTextureFormat(texture->GetFormat());
			rhiTexViewDesc.aspect = WGPUTextureAspect_All;
			rhiTexViewDesc.arrayLayerCount = 1;
			rhiTexViewDesc.baseArrayLayer = i;
			rhiTexViewDesc.baseMipLevel = 0;
			rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
			rhiTexViewDesc.dimension = WGPUTextureViewDimension_2D;

			WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
			texture->m_rhiViews.append(rhiView);
		}
	}
}

//-------------------------------------------------------------
// Pipeline management

IGPUBufferPtr CWGPURenderAPI::CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name) const
{
	CRefPtr<CWGPUBuffer> buffer = CRefPtr_new(CWGPUBuffer, bufferInfo, bufferUsageFlags, name);
	//TODO: buffer->IsValid();

	return IGPUBufferPtr(buffer);
}

IGPUPipelineLayoutPtr CWGPURenderAPI::CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const
{
	// Pipeline layout and bind group layout
	// are also objects of IGPURenderPipeline
	// There are 3 distinctive bind groups or buffers as our MatSystem design defines:
	//		- Material Constant Properties (static buffer)
	//		- Material Proxy Properties (buffers of these group updated every frame)
	//		- Scene Properties (camera, transform, fog, clip planes)
	Array<WGPUBindGroupLayout> rhiBindGroupLayout(PP_SL);
	Array<WGPUBindGroupLayoutEntry> rhiBindGroupLayoutEntry(PP_SL);
	for(const BindGroupLayoutDesc& bindGroupDesc : layoutDesc.bindGroups)
	{
		rhiBindGroupLayoutEntry.clear();

		for(const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
		{
			WGPUBindGroupLayoutEntry bglEntry = {};
			bglEntry.binding = entry.binding;

			if (entry.visibility & SHADERKIND_VERTEX)	bglEntry.visibility |= WGPUShaderStage_Vertex;
			if (entry.visibility & SHADERKIND_FRAGMENT) bglEntry.visibility |= WGPUShaderStage_Fragment;
			if (entry.visibility & SHADERKIND_COMPUTE)	bglEntry.visibility |= WGPUShaderStage_Compute;

			switch (entry.type)
			{
				case BINDENTRY_BUFFER:
					bglEntry.buffer.hasDynamicOffset = entry.buffer.hasDynamicOffset;
					bglEntry.buffer.type = g_wgpuBufferBindingType[entry.buffer.bindType];
					break;
				case BINDENTRY_SAMPLER:
					bglEntry.sampler.type = g_wgpuSamplerBindingType[entry.sampler.bindType];
					break;
				case BINDENTRY_TEXTURE:
					bglEntry.texture.sampleType = g_wgpuTexSampleType[entry.texture.sampleType];
					bglEntry.texture.viewDimension = g_wgpuTexViewDimensions[entry.texture.dimension];
					bglEntry.texture.multisampled = entry.texture.multisampled;
					break;
				case BINDENTRY_STORAGETEXTURE:
					bglEntry.storageTexture.access = g_wgpuStorageTexAccess[entry.storageTexture.access];
					bglEntry.storageTexture.viewDimension = g_wgpuTexViewDimensions[entry.storageTexture.dimension];
					bglEntry.storageTexture.format = GetWGPUTextureFormat(entry.storageTexture.format);
					break;
			}
			rhiBindGroupLayoutEntry.append(bglEntry);
		}

		WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
		bindGroupLayoutDesc.entryCount = rhiBindGroupLayoutEntry.numElem();
		bindGroupLayoutDesc.entries = rhiBindGroupLayoutEntry.ptr();

		WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_rhiDevice, &bindGroupLayoutDesc);
		if (!bindGroupLayout)
			return nullptr;

		rhiBindGroupLayout.append(bindGroupLayout);
	}

	WGPUPipelineLayoutDescriptor rhiPipelineLayoutDesc = {};
	rhiPipelineLayoutDesc.label = layoutDesc.name.Length() ? layoutDesc.name.ToCString() : nullptr;
	rhiPipelineLayoutDesc.bindGroupLayoutCount = rhiBindGroupLayout.numElem();
	rhiPipelineLayoutDesc.bindGroupLayouts = rhiBindGroupLayout.ptr();

	WGPUPipelineLayout rhiPipelineLayout = wgpuDeviceCreatePipelineLayout(m_rhiDevice, &rhiPipelineLayoutDesc);
	if (!rhiPipelineLayout)
		return nullptr;

	CRefPtr<CWGPUPipelineLayout> pipelineLayout = CRefPtr_new(CWGPUPipelineLayout);
	pipelineLayout->m_rhiBindGroupLayout = std::move(rhiBindGroupLayout);
	pipelineLayout->m_rhiPipelineLayout = rhiPipelineLayout;
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

IGPUBindGroupPtr CWGPURenderAPI::CreateBindGroup(const IGPUPipelineLayout* layoutDesc, const BindGroupDesc& bindGroupDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	const CWGPUPipelineLayout* pipelineLayout = static_cast<const CWGPUPipelineLayout*>(layoutDesc);
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
	
	rhiBindGroupDesc.label = bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr;
	rhiBindGroupDesc.layout = pipelineLayout->m_rhiBindGroupLayout[bindGroupDesc.groupIdx];
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;
	
	CRefPtr<CWGPUBindGroup> bindGroup = CRefPtr_new(CWGPUBindGroup);
	bindGroup->m_rhiBindGroup = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CWGPURenderAPI::CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const
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

	rhiBindGroupDesc.label = bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr;
	rhiBindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(static_cast<const CWGPURenderPipeline*>(renderPipeline)->m_rhiRenderPipeline, bindGroupDesc.groupIdx);
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;

	CRefPtr<CWGPUBindGroup> bindGroup = CRefPtr_new(CWGPUBindGroup);
	bindGroup->m_rhiBindGroup = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CWGPURenderAPI::CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const
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

	rhiBindGroupDesc.label = bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr;
	rhiBindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(static_cast<const CWGPUComputePipeline*>(computePipeline)->m_rhiComputePipeline, bindGroupDesc.groupIdx);
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;

	CRefPtr<CWGPUBindGroup> bindGroup = CRefPtr_new(CWGPUBindGroup);
	bindGroup->m_rhiBindGroup = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

WGPUShaderModule CWGPURenderAPI::CreateShaderSPIRV(const uint32* code, uint32 size, const char* name) const
{
	PROF_EVENT_F();

	WGPUDawnShaderModuleSPIRVOptionsDescriptor rhiDawnShaderModuleDesc = {};
	rhiDawnShaderModuleDesc.chain.sType = WGPUSType_DawnShaderModuleSPIRVOptionsDescriptor;
	rhiDawnShaderModuleDesc.allowNonUniformDerivatives = true;

	WGPUShaderModuleSPIRVDescriptor rhiSpirvDesc = {};
	rhiSpirvDesc.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
	rhiSpirvDesc.chain.next = &rhiDawnShaderModuleDesc.chain;
	rhiSpirvDesc.codeSize = size / sizeof(uint32_t);
	rhiSpirvDesc.code = code;

	WGPUShaderModuleDescriptor rhiShaderModuleDesc = {};
	rhiShaderModuleDesc.nextInChain = &rhiSpirvDesc.chain;
	rhiShaderModuleDesc.label = name;

	WGPUShaderModule shaderModule = nullptr;
	g_renderWorker.WaitForExecute(__func__, [this, &shaderModule, &rhiShaderModuleDesc]() {
		shaderModule = wgpuDeviceCreateShaderModule(m_rhiDevice, &rhiShaderModuleDesc);
		return 0;
	});

	return shaderModule;
}

WGPUShaderModule CWGPURenderAPI::GetOrLoadShaderModule(const ShaderInfoWGPUImpl& shaderInfo, int shaderModuleIdx) const
{
	ShaderInfoWGPUImpl::Module& mod = const_cast<ShaderInfoWGPUImpl::Module&>(shaderInfo.modules[shaderModuleIdx]);
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
	WGPUShaderModule rhiShaderModule = CreateShaderSPIRV(reinterpret_cast<uint32*>(shaderData.GetBasePointer()), shaderData.GetSize(), shaderModuleName);
	if (!rhiShaderModule)
	{
		MsgError("Can't create shader module %s!\n", shaderModuleName.ToCString());
		return nullptr;
	}
	mod.rhiModule = rhiShaderModule;

	return rhiShaderModule;
}

void CWGPURenderAPI::LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines) const
{
	const int shaderNameHash = StringToHash(shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		MsgError("LoadShaderModules: unknown shader '%s' specified\n", shaderName);
		return;
	}

	const ShaderInfoWGPUImpl& shaderInfo = *shaderIt;
	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(defines, queryStrHash))
	{
		MsgError("LoadShaderModules: unknown defines in query for shader '%s'\n", shaderName);
		return;
	}

	for (int i = 0; i < shaderInfo.vertexLayouts.numElem(); ++i)
	{
		const ShaderInfoWGPUImpl::VertLayout& layout = shaderInfo.vertexLayouts[i];
		if (layout.aliasOf != -1)
			continue;

		if(shaderInfo.shaderKinds & SHADERKIND_FRAGMENT)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_FRAGMENT);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_VERTEX)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_VERTEX);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_COMPUTE)
		{
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, i, SHADERKIND_COMPUTE);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
	}
}

IGPURenderPipelinePtr CWGPURenderAPI::CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout) const
{
	PROF_EVENT("CWGPURenderAPI::CreateRenderPipeline");

	const int shaderNameHash = StringToHash(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Render pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfoWGPUImpl& shaderInfo = *shaderIt;
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

	int vertexLayoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoWGPUImpl::VertLayout& layout) {
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
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX);
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
		rhiVertexState.entryPoint = pipelineDesc.vertex.shaderEntryPoint;
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
		rhiDepthStencil.depthWriteEnabled = pipelineDesc.depthStencil.depthWrite;
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
			const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT);
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
		rhiFragmentState.entryPoint = pipelineDesc.fragment.shaderEntryPoint;
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
	rhiRenderPipelineDesc.label = pipelineName;

	{
		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		WGPURenderPipeline rhiRenderPipeline = wgpuDeviceCreateRenderPipeline(m_rhiDevice, &rhiRenderPipelineDesc);
		if (!rhiRenderPipeline)
		{
			ASSERT_FAIL("Render pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CWGPURenderPipeline> renderPipeline = CRefPtr_new(CWGPURenderPipeline);
		renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;

		return IGPURenderPipelinePtr(renderPipeline);
	}
}

IGPUCommandRecorderPtr CWGPURenderAPI::CreateCommandRecorder(const char* name, void* userData) const
{
	WGPUCommandEncoderDescriptor rhiEncoderDesc = {};
	rhiEncoderDesc.label = name;
	WGPUCommandEncoder rhiCommandEncoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
	if (!rhiCommandEncoder)
		return nullptr;

	CRefPtr<CWGPUCommandRecorder> commandRecorder = CRefPtr_new(CWGPUCommandRecorder);
	commandRecorder->m_rhiCommandEncoder = rhiCommandEncoder;
	commandRecorder->m_userData = userData;

	return IGPUCommandRecorderPtr(commandRecorder);
}

IGPURenderPassRecorderPtr CWGPURenderAPI::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	WGPURenderPassDescriptor rhiRenderPassDesc = {};
	FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS> rhiColorAttachmentList;
	WGPURenderPassDepthStencilAttachment rhiDepthStencilAttachment = {};
	FillWGPURenderPassDescriptor(renderPassDesc, rhiRenderPassDesc, rhiColorAttachmentList, rhiDepthStencilAttachment);

	WGPUCommandEncoder rhiCommandEncoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
	if (!rhiCommandEncoder)
		return nullptr;

	WGPURenderPassEncoder rhiRenderPassEncoder = wgpuCommandEncoderBeginRenderPass(rhiCommandEncoder, &rhiRenderPassDesc);
	if (!rhiRenderPassEncoder)
		return nullptr;

	IVector2D renderTargetDims = 0;
	CRefPtr<CWGPURenderPassRecorder> renderPass = CRefPtr_new(CWGPURenderPassRecorder);
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		if (colorTarget.target.texture)
		{
			renderTargetDims = IVector2D(colorTarget.target.texture->GetWidth(), colorTarget.target.texture->GetHeight());
			renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target.texture->GetFormat() : FORMAT_NONE;

			if (colorTarget.target)
				renderPass->m_renderTargetMSAASamples = colorTarget.target.texture->GetSampleCount();
		}
	}

	if (renderPassDesc.depthStencil)
	{
		renderTargetDims = IVector2D(renderPassDesc.depthStencil.texture->GetWidth(), renderPassDesc.depthStencil.texture->GetHeight());
		renderPass->m_depthTargetFormat = renderPassDesc.depthStencil.texture->GetFormat();
	}
	else
		renderPass->m_depthTargetFormat = FORMAT_NONE;

	renderPass->m_depthReadOnly = renderPassDesc.depthReadOnly;
	renderPass->m_stencilReadOnly = renderPassDesc.stencilReadOnly;

	renderPass->m_rhiCommandEncoder = rhiCommandEncoder;
	renderPass->m_rhiRenderPassEncoder = rhiRenderPassEncoder;
	renderPass->m_renderTargetDims = renderTargetDims;
	renderPass->m_userData = userData;

	return IGPURenderPassRecorderPtr(renderPass);
}

IGPUComputePipelinePtr CWGPURenderAPI::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) const
{
	const int shaderNameHash = StringToHash(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Render pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfoWGPUImpl& shaderInfo = *shaderIt;
	ASSERT_MSG(shaderInfo.shaderName == pipelineDesc.shaderName, "Shader name mismatch, requested '%s' got '%s' (hash collision?)", pipelineDesc.shaderName.ToCString(), shaderInfo.shaderName.ToCString());

	if (!(shaderInfo.shaderKinds & SHADERKIND_COMPUTE))
	{
		ASSERT_FAIL("Shader %s must have Compute kind", shaderInfo.shaderName.ToCString());
		return nullptr;
	}

	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(pipelineDesc.shaderQuery, queryStrHash))
	{
		ASSERT_FAIL("Render pipeline has unknown defines in query for shader '%s'", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	int layoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfoWGPUImpl::VertLayout& layout) {
		return layout.nameHash == pipelineDesc.shaderLayoutId;
	});

	if (layoutIdx == -1)
	{
		ASSERT_FAIL("Compute pipeline %s has unknown layout id", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}
	if (shaderInfo.vertexLayouts[layoutIdx].aliasOf != -1)
		layoutIdx = shaderInfo.vertexLayouts[layoutIdx].aliasOf;

	WGPUShaderModule rhiComputeShaderModule = nullptr;
	{
		const uint shaderModuleId = PackShaderModuleId(queryStrHash, layoutIdx, SHADERKIND_COMPUTE);
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

	Array<WGPUConstantEntry> rhiComputePipelineConstants(PP_SL);

	WGPUComputePipelineDescriptor rhiComputePipelineDesc = {};
	rhiComputePipelineDesc.compute.constantCount = rhiComputePipelineConstants.numElem();
	rhiComputePipelineDesc.compute.constants = rhiComputePipelineConstants.ptr();
	rhiComputePipelineDesc.compute.entryPoint = pipelineDesc.shaderEntryPoint;
	rhiComputePipelineDesc.compute.module = rhiComputeShaderModule;

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[layoutIdx].name.ToCString());
	rhiComputePipelineDesc.label = pipelineName;

	{
		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		WGPUComputePipeline rhiComputePipeline = wgpuDeviceCreateComputePipeline(m_rhiDevice, &rhiComputePipelineDesc);
		if (!rhiComputePipeline)
		{
			ASSERT_FAIL("Render pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CWGPUComputePipeline> renderPipeline = CRefPtr_new(CWGPUComputePipeline);
		renderPipeline->m_rhiComputePipeline = rhiComputePipeline;

		return IGPUComputePipelinePtr(renderPipeline);
	}
}

IGPUComputePassRecorderPtr CWGPURenderAPI::BeginComputePass(const char* name, void* userData) const
{
	WGPUCommandEncoder rhiCommandEncoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
	if (!rhiCommandEncoder)
		return nullptr;

	WGPUComputePassDescriptor rhiComputePassDesc = {};
	rhiComputePassDesc.label = name;
	//rhiComputePassDesc.timestampWrites TODO
	WGPUComputePassEncoder rhiComputePassEncoder = wgpuCommandEncoderBeginComputePass(rhiCommandEncoder, &rhiComputePassDesc);
	if (!rhiComputePassEncoder)
		return nullptr;

	CRefPtr<CWGPUComputePassRecorder> renderPass = CRefPtr_new(CWGPUComputePassRecorder);

	renderPass->m_rhiCommandEncoder = rhiCommandEncoder;
	renderPass->m_rhiComputePassEncoder = rhiComputePassEncoder;
	renderPass->m_userData = userData;

	return IGPUComputePassRecorderPtr(renderPass);
}

void CWGPURenderAPI::SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	PROF_EVENT_F();
	g_renderWorker.WaitForExecute(__func__, [this, cmdBuffers]() {
		Array<WGPUCommandBuffer> rhiSubmitBuffers(PP_SL);
		rhiSubmitBuffers.reserve(cmdBuffers.numElem());

		for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
		{
			if (!cmdBuffer)
				continue;

			const CWGPUCommandBuffer* bufferImpl = static_cast<const CWGPUCommandBuffer*>(cmdBuffer);
			WGPUCommandBuffer rhiCmdBuffer = bufferImpl->m_rhiCommandBuffer;
			ASSERT(rhiCmdBuffer);

			rhiSubmitBuffers.append(rhiCmdBuffer);
		}
		wgpuQueueSubmit(m_rhiQueue, rhiSubmitBuffers.numElem(), rhiSubmitBuffers.ptr());
		return 0;
	});
}


Future<bool> CWGPURenderAPI::SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	Array<WGPUCommandBuffer> rhiSubmitBuffers(PP_SL);
	rhiSubmitBuffers.reserve(cmdBuffers.numElem());
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;
		const CWGPUCommandBuffer* bufferImpl = static_cast<const CWGPUCommandBuffer*>(cmdBuffer);
		WGPUCommandBuffer rhiCmdBuffer = bufferImpl->m_rhiCommandBuffer;
		ASSERT(rhiCmdBuffer);

		rhiSubmitBuffers.append(rhiCmdBuffer);
		wgpuCommandBufferReference(rhiCmdBuffer);
	}

	if (!rhiSubmitBuffers.numElem())
	{
		return Future<bool>::Succeed(true);
	}

	Promise<bool> promise;
	g_renderWorker.Execute(__func__, [this, submitBuffers = std::move(rhiSubmitBuffers), promiseData = promise.GrabDataPtr()]() {
		wgpuQueueSubmit(m_rhiQueue, submitBuffers.numElem(), submitBuffers.ptr());
		wgpuQueueOnSubmittedWorkDone(m_rhiQueue, [](WGPUQueueWorkDoneStatus status, void* userdata) {
			Promise<bool> promise(reinterpret_cast<Promise<bool>::Data*>(userdata));

			if(status != WGPUQueueWorkDoneStatus_Success)
			{
				const char* str = "Invalid";
				switch (status)
				{
				case WGPUQueueWorkDoneStatus_Error:
					str = "Error";
					break;
				case WGPUQueueWorkDoneStatus_Unknown:
					str = "UnknownStatus";
					break;
				case WGPUQueueWorkDoneStatus_DeviceLost:
					str = "DeviceLost";
					break;
				}
				promise.SetError(-1, str);
			}
			else
			{
				promise.SetResult(true);
			}
		}, promiseData);

		for (WGPUCommandBuffer rhiCmdBuffer : submitBuffers)
			wgpuCommandBufferRelease(rhiCmdBuffer);

		return 0;
	});

	return promise.CreateFuture();
}

void CWGPURenderAPI::Flush()
{
	WGPU_INSTANCE_SPIN;
}

static void CreateQuerySet()
{
	WGPUQuerySetDescriptor rhiQuerySetDesc = {};
	rhiQuerySetDesc.label = "name";
	rhiQuerySetDesc.type = WGPUQueryType_Occlusion;
	rhiQuerySetDesc.count = 32;
	WGPUQuerySet rhiQuerySet = wgpuDeviceCreateQuerySet(nullptr, &rhiQuerySetDesc);
}
