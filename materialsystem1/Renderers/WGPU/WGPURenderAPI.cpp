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
#include "WGPURenderPassRecorder.h"

#include "../RenderWorker.h"

#define ASSERT_DEPRECATED() // ASSERT_FAIL("Deprecated API %s", __func__)

DECLARE_CVAR(wgpu_preload_shaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);

CWGPURenderAPI CWGPURenderAPI::Instance;

static uint PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind)
{
	return queryStrHash | (static_cast<uint>(vertexLayoutIdx) << StringHashBits) | (static_cast<uint>(kind) << (StringHashBits + 4));
}

ShaderInfoWGPUImpl::~ShaderInfoWGPUImpl()
{
	g_fileSystem->ClosePackage(shaderPackFile);
}

void ShaderInfoWGPUImpl::Release()
{
	for (ShaderInfoWGPUImpl::Module module : modules)
		wgpuShaderModuleRelease(module.rhiModule);
}

bool ShaderInfoWGPUImpl::GetShaderQueryHash(const Array<EqString>& findDefines, int& outHash) const
{
	Array<int> defineIds(PP_SL);
	for (EqString& define : findDefines)
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
	outHash = StringToHash(queryStr);
	return true;
}

//------------------------------------------

void CWGPURenderAPI::Init(const ShaderAPIParams& params)
{
	ShaderAPI_Base::Init(params);

	int shaderPackCount = 0;
	int shaderModCount = 0;
	EqString shaderPackPath;
	CFileSystemFind fsFind("shaders/*.shd");
	while (fsFind.Next())
	{
		if (fsFind.IsDirectory())
			continue;

		EqString path = fsFind.GetPath();
		CombinePath(shaderPackPath, "shaders", path);
		auto shaderInfoIt = m_shaderCache.insert(StringToHash(path.Path_Strip_Ext()));

		ShaderInfoWGPUImpl& shaderInfo = *shaderInfoIt;
		shaderModCount += LoadShaderPackage(shaderPackPath, shaderInfo);
		++shaderPackCount;
	}

	Msg("Init shader cache from %d packages, %d shader modules loaded\n", shaderPackCount, shaderModCount);
}

int CWGPURenderAPI::LoadShaderPackage(const char* filename, ShaderInfoWGPUImpl& output)
{
	output.shaderPackFile = g_fileSystem->OpenPackage(filename, SP_MOD | SP_DATA);
	if (!output.shaderPackFile)
		return 0;

	KVSection shaderInfoKvs;
	{
		IFilePtr file = output.shaderPackFile->Open("ShaderInfo", VS_OPEN_READ);
		if (!KV_LoadFromStream(file, &shaderInfoKvs))
		{
			Msg("No ShaderInfo in file %s\n", filename);
			return false;
		}
	}

	if (!strstr(filename, shaderInfoKvs.GetName()))
	{
		ASSERT_FAIL("Shader package '%s' file name doesn't match it's name '%s' in desc", filename, shaderInfoKvs.GetName());
	}

	output.shaderName = shaderInfoKvs.GetName();

	const KVSection* defines = shaderInfoKvs.FindSection("Defines");
	if (defines)
	{
		output.defines.reserve(defines->KeyCount());
		for (KVValueIterator<EqString> it(defines); !it.atEnd(); ++it)
			output.defines.append(it);
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
	output.shaderKinds = shaderKinds;

	for (KVKeyIterator it(shaderInfoKvs.FindSection("VertexLayouts")); !it.atEnd(); ++it)
	{
		ShaderInfoWGPUImpl::VertLayout& layout = output.vertexLayouts.append();
		layout.name = EqString(it);
		layout.nameHash = StringToHash(layout.name);
		if (!stricmp(KV_GetValueString(*it, 0), "aliasOf"))
		{
			layout.aliasOf = arrayFindIndexF(output.vertexLayouts, [&](const ShaderInfoWGPUImpl::VertLayout& layout) {
				return layout.name == KV_GetValueString(*it, 1);
			});
		}
	}

	int filesFound = 0;
	for (KVKeyIterator it(shaderInfoKvs.FindSection("FileList"), "spv"); !it.atEnd(); ++it)
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
		const int queryStrHash = StringToHash(queryStr);

		const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, kind);
		const EqString shaderFileName = EqString::Format("%s-%s%s", output.vertexLayouts[vertexLayoutIdx].name.ToCString(), queryStr, kindExt.ToCString());
		const int shaderModuleFileIndex = output.shaderPackFile->FindFileIndex(shaderFileName);
		if (wgpu_preload_shaders.GetBool())
		{
			CMemoryStream shaderData(PP_SL);
			{
				IFilePtr shaderFile = output.shaderPackFile->Open(shaderModuleFileIndex, VS_OPEN_READ);
				if (!shaderFile)
				{
					MsgError("Can't open shader %s in shader package!\n", shaderFileName.ToCString());
					return false;
				}
				shaderData.Open(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, shaderFile->GetSize());
				shaderData.AppendStream(shaderFile);
			}

			WGPUShaderModule rhiShaderModule = CreateShaderSPIRV(reinterpret_cast<uint32*>(shaderData.GetBasePointer()), shaderData.GetSize(), EqString::Format("%s-%s", shaderInfoKvs.GetName(), shaderFileName.ToCString()));
			if (!rhiShaderModule)
			{
				MsgError("Can't create shader module %s!\n", shaderFileName.ToCString());
				return false;
			}
		}

		const int moduleIndex = output.modules.append({ nullptr, static_cast<EShaderKind>(kind), shaderModuleFileIndex });
		ASSERT_MSG(output.modulesMap.find(shaderModuleId).atEnd(), "%s-%s module already added, fix shader compiler", kindStr, queryStr);
		output.modulesMap.insert(shaderModuleId, moduleIndex);
		++filesFound;
	}

	// we need to validate references so collect refs in second pass
	for (KVKeyIterator it(shaderInfoKvs.FindSection("FileList"), "ref"); !it.atEnd(); ++it)
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
		const int queryStrHash = StringToHash(queryStr);

		const uint shaderModuleId = PackShaderModuleId(queryStrHash, vertexLayoutIdx, kind);
		const int refIndex = KV_GetValueInt(itemSec, 3);

		ASSERT_MSG(output.modules[refIndex].kind == static_cast<EShaderKind>(kind), "%s ref %d (%s-%s) points to invalid shader kind", output.shaderName.ToCString(), refIndex, kindStr, queryStr);

		ASSERT_MSG(output.modulesMap.find(shaderModuleId).atEnd(), "%s %s-%s already added, fix shader compiler\n", output.shaderName.ToCString(), kindStr, queryStr);
		output.modulesMap.insert(shaderModuleId, refIndex);
	}

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
	return true;
}

IVertexFormat* CWGPURenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CWGPUVertexFormat(name, formatDesc);
	m_VFList.append(pVF);
	return pVF;
}

IVertexBuffer* CWGPURenderAPI::CreateVertexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUVertexBuffer* buffer = PPNew CWGPUVertexBuffer(bufferInfo);
	m_VBList.append(buffer);
	return buffer;
}

IIndexBuffer* CWGPURenderAPI::CreateIndexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUIndexBuffer* buffer = PPNew CWGPUIndexBuffer(bufferInfo);
	m_IBList.append(buffer);
	return buffer;
}

// Destroy vertex format
void CWGPURenderAPI::DestroyVertexFormat(IVertexFormat* pFormat)
{
	if (m_VFList.fastRemove(pFormat))
		delete pFormat;
}

// Destroy vertex buffer
void CWGPURenderAPI::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	if (m_VBList.fastRemove(pVertexBuffer))
		delete pVertexBuffer;
}

// Destroy index buffer
void CWGPURenderAPI::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (m_IBList.fastRemove(pIndexBuffer))
		delete pIndexBuffer;
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
ITexturePtr	CWGPURenderAPI::CreateRenderTarget(const char* pszName, int width, int height, ETextureFormat nRTFormat, ETexFilterMode textureFilterType, ETexAddressMode textureAddress, ECompareFunc comparison, int nFlags)
{
	CRefPtr<CWGPUTexture> texture = CRefPtr_new(CWGPUTexture);
	texture->SetName(pszName);
	texture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	texture->SetFormat(nRTFormat);
	texture->SetSamplerState(SamplerStateParams(textureFilterType, textureAddress));
	texture->m_imgType = (nFlags & TEXFLAG_CUBEMAP) ? IMAGE_TYPE_CUBE : IMAGE_TYPE_2D;

	ResizeRenderTarget(ITexturePtr(texture), width, height);

	if (!texture->m_rhiTextures.numElem()) 
		return nullptr;

	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(texture);
		m_TextureList.insert(texture->m_nameHash, texture);
	}

	return ITexturePtr(texture);
}

void CWGPURenderAPI::ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall)
{
	CWGPUTexture* texture = static_cast<CWGPUTexture*>(renderTarget.Ptr());
	if (!texture)
		return;

	if (texture->GetWidth() == newWide && texture->GetHeight() == newTall)
		return;

	texture->SetDimensions(newWide, newTall);
	texture->Release();

	const bool isCubeMap = (texture->GetFlags() & TEXFLAG_CUBEMAP);

	int texDepth = 1;
	if (isCubeMap)
		texDepth = 6;

	WGPUTextureDescriptor rhiTextureDesc = {};
	rhiTextureDesc.label = texture->GetName();
	rhiTextureDesc.mipLevelCount = 1;
	rhiTextureDesc.size = WGPUExtent3D{ (uint)newWide, (uint)newTall, (uint)texDepth };
	rhiTextureDesc.sampleCount = 1;
	rhiTextureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
	rhiTextureDesc.format = GetWGPUTextureFormat(texture->GetFormat());
	rhiTextureDesc.dimension = WGPUTextureDimension_2D;
	rhiTextureDesc.viewFormatCount = 0;
	rhiTextureDesc.viewFormats = nullptr;

	if (rhiTextureDesc.format == WGPUTextureFormat_Undefined)
	{
		MsgError("Unsupported texture format %d\n", texture->GetFormat());
		return;
	}

	WGPUTexture rhiTexture = wgpuDeviceCreateTexture(m_rhiDevice, &rhiTextureDesc);
	if (!rhiTexture)
	{
		ErrorMsg("Failed to create render target %s\n", texture->GetName());
		return;
	}

	texture->m_rhiTextures.append(rhiTexture);
	{
		WGPUTextureViewDescriptor rhiTexViewDesc = {};
		rhiTexViewDesc.format = rhiTextureDesc.format;
		rhiTexViewDesc.aspect = WGPUTextureAspect_All;
		rhiTexViewDesc.arrayLayerCount = isCubeMap ? 6 : 1;
		rhiTexViewDesc.baseArrayLayer = 0;
		rhiTexViewDesc.baseMipLevel = 0;
		rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
		rhiTexViewDesc.dimension = isCubeMap ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;

		WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
		texture->m_rhiViews.append(rhiView);
	}
}

//-------------------------------------------------------------
// Pipeline management

IGPUBufferPtr CWGPURenderAPI::CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name) const
{
	CRefPtr<CWGPUBuffer> buffer = CRefPtr_new(CWGPUBuffer);
	int wgpuUsageFlags = 0;
	
	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM) wgpuUsageFlags |= WGPUBufferUsage_Uniform;
	if (bufferUsageFlags & BUFFERUSAGE_VERTEX)	wgpuUsageFlags |= WGPUBufferUsage_Vertex;
	if (bufferUsageFlags & BUFFERUSAGE_INDEX)	wgpuUsageFlags |= WGPUBufferUsage_Index;
	if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)wgpuUsageFlags |= WGPUBufferUsage_Indirect;
	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)	wgpuUsageFlags |= WGPUBufferUsage_Storage;

	buffer->Init(bufferInfo, wgpuUsageFlags , name);
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

IGPUBindGroupPtr CWGPURenderAPI::CreateBindGroup(const IGPUPipelineLayoutPtr layoutDesc, int layoutBindGroupIdx, const BindGroupDesc& bindGroupDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	// Bind group is a collection of GPU resources (buffers, samplers, textures)
	// layed out for the pipelines so you can set resources in one shot.
	Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);

	// release samplers after bind groups has been made
	// other things like textures and buffer are released by user
	// and still can be used by this bind groups
	defer{
		for (WGPUBindGroupEntry& entry : rhiBindGroupEntryList)
		{
			if (entry.sampler)
				wgpuSamplerRelease(entry.sampler);
		}
	};

	for(const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		WGPUBindGroupEntry rhiBindGroupEntryDesc = {};
		rhiBindGroupEntryDesc.binding = bindGroupEntry.binding;
		switch (bindGroupEntry.type)
		{
			case BINDENTRY_BUFFER:
			{
				CWGPUBuffer* buffer = static_cast<CWGPUBuffer*>(bindGroupEntry.buffer);
				if (buffer)
					rhiBindGroupEntryDesc.buffer = buffer->GetWGPUBuffer();
				else
					ASSERT_FAIL("NULL buffer for bindGroup %d binding %d", layoutBindGroupIdx, bindGroupEntry.binding);

				rhiBindGroupEntryDesc.size = bindGroupEntry.bufferSize;
				rhiBindGroupEntryDesc.offset = bindGroupEntry.bufferOffset;
				break;
			}
			case BINDENTRY_SAMPLER:
			{
				WGPUSamplerDescriptor rhiSamplerDesc = {};
				FillWGPUSamplerDescriptor(bindGroupEntry.sampler, rhiSamplerDesc);

				ASSERT(bindGroupEntry.sampler.maxAnisotropy > 0);

				rhiBindGroupEntryDesc.sampler = wgpuDeviceCreateSampler(m_rhiDevice, &rhiSamplerDesc);
				break;
			}
			case BINDENTRY_STORAGETEXTURE:
			case BINDENTRY_TEXTURE:
				CWGPUTexture* texture = static_cast<CWGPUTexture*>(bindGroupEntry.texture);

				// NOTE: animated textures aren't that supported, so it would need array lookup through the shader
				if(texture)
				{
					ASSERT_MSG(texture->m_rhiViews.numElem(), "Texture '%s' has no views", texture->GetName());
					rhiBindGroupEntryDesc.textureView = texture->GetWGPUTextureView();
				}
				else
					ASSERT_FAIL("NULL texture for bindGroup %d binding %d", layoutBindGroupIdx, bindGroupEntry.binding);
				break;
		}

		rhiBindGroupEntryList.append(rhiBindGroupEntryDesc);
	}

	const CWGPUPipelineLayout* pipelineLayoutDesc = static_cast<CWGPUPipelineLayout*>(layoutDesc.Ptr());

	WGPUBindGroupDescriptor rhiBindGroupDesc = {};
	rhiBindGroupDesc.label = bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr;
	rhiBindGroupDesc.layout = pipelineLayoutDesc->m_rhiBindGroupLayout[layoutBindGroupIdx];
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
	WGPUShaderModuleSPIRVDescriptor rhiSpirvDesc = {};
	rhiSpirvDesc.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
	rhiSpirvDesc.codeSize = size / sizeof(uint32_t);
	rhiSpirvDesc.code = code;

	WGPUShaderModuleDescriptor rhiShaderModuleDesc = {};
	rhiShaderModuleDesc.nextInChain = &rhiSpirvDesc.chain;
	rhiShaderModuleDesc.label = name;

	return wgpuDeviceCreateShaderModule(m_rhiDevice, &rhiShaderModuleDesc);
}

WGPUShaderModule CWGPURenderAPI::GetOrLoadShaderModule(const ShaderInfoWGPUImpl& shaderInfo, int shaderModuleIdx) const
{
	ShaderInfoWGPUImpl::Module& mod = const_cast<ShaderInfoWGPUImpl::Module&>(shaderInfo.modules[shaderModuleIdx]);
	if (!mod.rhiModule)
	{
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

		EqString shaderModuleName = EqString::Format("%s-%d", shaderInfo.shaderName.ToCString(), shaderModuleIdx);
		WGPUShaderModule rhiShaderModule = CreateShaderSPIRV(reinterpret_cast<uint32*>(shaderData.GetBasePointer()), shaderData.GetSize(), shaderModuleName);
		if (!rhiShaderModule)
		{
			MsgError("Can't create shader module %s!\n", shaderModuleName.ToCString());
			return false;
		}
		mod.rhiModule = rhiShaderModule;
	}
	return mod.rhiModule;
}

IGPURenderPipelinePtr CWGPURenderAPI::CreateRenderPipeline(const IGPUPipelineLayoutPtr layoutDesc, const RenderPipelineDesc& pipelineDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	const int shaderNameHash = StringToHash(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Render pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfoWGPUImpl& shaderInfo = *shaderIt;
	ASSERT_MSG(shaderInfo.shaderName == pipelineDesc.shaderName, "Shader name mismatch, requested '%s' got '%s' (hash collision?)", pipelineDesc.shaderName.ToCString(), shaderInfo.shaderName.ToCString());

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
		ASSERT_FAIL("Render pipeline %s has unknown vertex layout for vertex shader", pipelineDesc.shaderName.ToCString());
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
	rhiRenderPipelineDesc.layout = static_cast<CWGPUPipelineLayout*>(layoutDesc.Ptr())->m_rhiPipelineLayout;

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
				for (EqString& str : pipelineDesc.shaderQuery)
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
			for (EqString& str : pipelineDesc.shaderQuery)
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
	if (pipelineDesc.depthStencil.depthTest)
	{
		ASSERT_MSG(pipelineDesc.depthStencil.format != FORMAT_NONE, "Must set valid depthStencil texture format");
		
		rhiDepthStencil.format = GetWGPUTextureFormat(pipelineDesc.depthStencil.format);
		rhiDepthStencil.depthWriteEnabled = pipelineDesc.depthStencil.depthWrite;
		rhiDepthStencil.depthCompare = g_wgpuCompareFunc[pipelineDesc.depthStencil.depthFunc];
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
	if(pipelineDesc.fragment.targets.numElem())
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No fragment shader entrypoint set");

		for(const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
		{
			{
				WGPUBlendState rhiBlend = {};
				FillWGPUBlendComponent(target.colorBlend, rhiBlend.color);
				FillWGPUBlendComponent(target.alphaBlend, rhiBlend.alpha);
				rhiColorTargetBlends.append(rhiBlend);
			}

			WGPUColorTargetState rhiColorTarget = {};
			rhiColorTarget.format = GetWGPUTextureFormat(target.format);
			rhiColorTarget.blend = &rhiColorTargetBlends.back();
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
				for (EqString& str : pipelineDesc.shaderQuery)
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
			for (EqString& str : pipelineDesc.shaderQuery)
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

IGPURenderPassRecorderPtr CWGPURenderAPI::BeginRenderPass(const RenderPassDesc& renderPassDesc) const
{
	WGPURenderPassDescriptor rhiRenderPassDesc = {};
	rhiRenderPassDesc.label = renderPassDesc.name.Length() ? renderPassDesc.name.ToCString() : nullptr;

	FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS> rhiColorAttachmentList;

	IVector2D renderTargetDims = 0;

	for(const RenderPassDesc::ColorTargetDesc& colorTarget : renderPassDesc.colorTargets)
	{
		// TODO: backbuffer alteration?
		const CWGPUTexture* targetTexture = static_cast<CWGPUTexture*>(colorTarget.target.Ptr());
		ASSERT_MSG(targetTexture, "NULL texture for color target");

		WGPURenderPassColorAttachment rhiColorAttachment = {};
		rhiColorAttachment.loadOp = g_wgpuLoadOp[colorTarget.loadOp];
		rhiColorAttachment.storeOp = g_wgpuStoreOp[colorTarget.storeOp];
		rhiColorAttachment.depthSlice = colorTarget.depthSlice;
		rhiColorAttachment.view = targetTexture->GetWGPUTextureView();
		rhiColorAttachment.resolveTarget = nullptr; // TODO
		rhiColorAttachment.clearValue = WGPUColor{ colorTarget.clearColor.r, colorTarget.clearColor.g, colorTarget.clearColor.b, colorTarget.clearColor.a };
		rhiColorAttachmentList.append(rhiColorAttachment);

		renderTargetDims = IVector2D(targetTexture->GetWidth(), targetTexture->GetHeight());
	}
	rhiRenderPassDesc.colorAttachmentCount = rhiColorAttachmentList.numElem();
	rhiRenderPassDesc.colorAttachments = rhiColorAttachmentList.ptr();

	WGPURenderPassDepthStencilAttachment rhiDepthStencilAttachment = {};

	if(renderPassDesc.depthStencil)
	{
		const CWGPUTexture* depthTexture = static_cast<CWGPUTexture*>(renderPassDesc.depthStencil.Ptr());

		rhiDepthStencilAttachment.depthClearValue = renderPassDesc.depthClearValue;
		rhiDepthStencilAttachment.depthReadOnly = false; // TODO
		rhiDepthStencilAttachment.depthLoadOp = g_wgpuLoadOp[renderPassDesc.depthLoadOp];
		rhiDepthStencilAttachment.depthStoreOp = g_wgpuStoreOp[renderPassDesc.depthStoreOp];

		rhiDepthStencilAttachment.stencilClearValue = renderPassDesc.stencilClearValue;
		rhiDepthStencilAttachment.stencilReadOnly = false;  // TODO
		rhiDepthStencilAttachment.stencilLoadOp = g_wgpuLoadOp[renderPassDesc.stencilLoadOp];
		rhiDepthStencilAttachment.stencilStoreOp = g_wgpuStoreOp[renderPassDesc.stencilStoreOp];

		rhiDepthStencilAttachment.view = depthTexture->GetWGPUTextureView();
		rhiRenderPassDesc.depthStencilAttachment = &rhiDepthStencilAttachment;
	}
	
	// TODO:
	// rhiRenderPassDesc.occlusionQuerySet

	WGPUCommandEncoder rhiCommandEncoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
	if (!rhiCommandEncoder)
		return nullptr;

	WGPURenderPassEncoder rhiRenderPassEncoder = wgpuCommandEncoderBeginRenderPass(rhiCommandEncoder, &rhiRenderPassDesc);
	if (!rhiRenderPassEncoder)
		return nullptr;

	CRefPtr<CWGPURenderPassRecorder> renderPass = CRefPtr_new(CWGPURenderPassRecorder);
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target->GetFormat() : FORMAT_NONE;
	}
	renderPass->m_depthTargetFormat = renderPassDesc.depthStencil ? renderPassDesc.depthStencil->GetFormat() : FORMAT_NONE;
	renderPass->m_rhiCommandEncoder = rhiCommandEncoder;
	renderPass->m_rhiRenderPassEncoder = rhiRenderPassEncoder;
	renderPass->m_renderTargetDims = renderTargetDims;

	return IGPURenderPassRecorderPtr(renderPass);
}

void CWGPURenderAPI::SubmitCommandBuffer(const IGPUCommandBuffer* cmdBuffer) const
{
	if (!cmdBuffer)
		return;

	g_renderWorker.Execute(__func__, [this, buffer = IGPUCommandBufferPtr(const_cast<IGPUCommandBuffer*>(cmdBuffer))]() {
		const CWGPUCommandBuffer* bufferImpl = static_cast<const CWGPUCommandBuffer*>(buffer.Ptr());
		wgpuQueueSubmit(m_rhiQueue, 1, &bufferImpl->m_rhiCommandBuffer);
		return 0;
	});
}

static void CreateQuerySet()
{
	WGPUQuerySetDescriptor rhiQuerySetDesc = {};
	rhiQuerySetDesc.label = "name";
	rhiQuerySetDesc.type = WGPUQueryType_Occlusion;
	rhiQuerySetDesc.count = 32;
	WGPUQuerySet rhiQuerySet = wgpuDeviceCreateQuerySet(nullptr, &rhiQuerySetDesc);
}

//-------------------------------------------------------------
// Shaders and it's operations

IShaderProgramPtr CWGPURenderAPI::CreateNewShaderProgram(const char* pszName, const char* query)
{
	return nullptr;
}

void CWGPURenderAPI::FreeShaderProgram(IShaderProgram* pShaderProgram)
{
}

bool CWGPURenderAPI::CompileShadersFromStream(IShaderProgramPtr pShaderOutput, const ShaderProgCompileInfo& info, const char* extra)
{
	return true; 
}

//-------------------------------------------------------------
// Occlusion query

// creates occlusion query object
IOcclusionQuery* CWGPURenderAPI::CreateOcclusionQuery()
{
	ASSERT_DEPRECATED();
	return nullptr;
}

// removal of occlusion query object
void CWGPURenderAPI::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Render states management

IRenderState* CWGPURenderAPI::CreateBlendingState( const BlendStateParams &blendDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

IRenderState* CWGPURenderAPI::CreateDepthStencilState( const DepthStencilStateParams &depthDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

IRenderState* CWGPURenderAPI::CreateRasterizerState( const RasterizerStateParams &rasterDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

void CWGPURenderAPI::DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Render target operations

void CWGPURenderAPI::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const MColor& fillColor, float fDepth, int nStencil)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::CopyFramebufferToTexture(const ITexturePtr& pTargetTexture)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect, IAARectangle* destRect)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeRenderTargetToBackBuffer()
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Various setup functions for drawing

void CWGPURenderAPI::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// State management

void  CWGPURenderAPI::SetScissorRectangle(const IAARectangle& rect)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetDepthRange(float fZNear, float fZFar)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Renderer state managemet

void CWGPURenderAPI::SetShader(IShaderProgramPtr pShader)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetTexture(int nameHash, const ITexturePtr& texture)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetShaderConstantRaw(int nameHash, const void* data, int nSize)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Primitive drawing

void CWGPURenderAPI::DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices)
{
	ASSERT_DEPRECATED();
}