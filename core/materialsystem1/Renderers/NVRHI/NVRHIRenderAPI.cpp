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
#include "NVRHIBackend.h"
#include "NVRHIRenderDefs.h"
#include "NVRHIStates.h"
#include "NVRHICommandRecorder.h"
#include "NVRHIRenderPassRecorder.h"

#include "../RenderWorker.h"
#include "NVRHIComputePassRecorder.h"

static void nvrhiFillTransientTextureHeapDesc(nvrhi::HeapDesc& heapDesc)
{
	// 32 MB per texture
	static constexpr int s_maxTransientTextureHeapSize = imgCalcMipMappedSize(FORMAT_RGBA16F, 4096, 1024, 1, 0, 1);
	heapDesc
		.setDebugName("TransientTextureHeap")
		.setCapacity(s_maxTransientTextureHeapSize)
		.setType(nvrhi::HeapType::DeviceLocal);
}

static void nvrhiFillTransientBufferHeapDesc(nvrhi::HeapDesc& heapDesc)
{
	// 8 MB
	static constexpr int s_maxTransientBufferHeapSize = 8 * 1024 * 1024;
	heapDesc
		.setDebugName("TransientBufferHeap")
		.setCapacity(s_maxTransientBufferHeapSize)
		.setType(nvrhi::HeapType::DeviceLocal);
}

constexpr EqStringRef s_shaderKindVertexName = "Vertex";
constexpr EqStringRef s_shaderKindFragmentName = "Fragment";
constexpr EqStringRef s_shaderKindComputeName = "Compute";
constexpr EqStringRef s_DefaultVertexLayoutName = "Default";

DECLARE_CVAR(nvrhi_preloadShaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);
DECLARE_CVAR_F(nvrhi_validation);

static Threading::CEqMutex s_transientHeapsMutex;
static Threading::CEqMutex s_cmdListMutex;

CNVRHIRenderAPI CNVRHIRenderAPI::Instance;
ShaderAPI_Base& ShaderAPI_Base::Instance = CNVRHIRenderAPI::Instance;
IShaderAPI* g_renderAPI = &CNVRHIRenderAPI::Instance;

//------------------------------------------

void CNVRHIRenderAPI::Init(const ShaderAPIParams& params)
{
	ShaderAPI_Base::Init(params);

	{
		constexpr int s_minTransientTextureHeaps = 4;

		nvrhi::HeapDesc heapDesc;
		nvrhiFillTransientTextureHeapDesc(heapDesc);

		m_rhiTransientTextureHeaps.reserve(s_minTransientTextureHeaps);
		m_rhiFreeTransientTextureHeaps.reserve(s_minTransientTextureHeaps);
		for (int i = 0; i < s_minTransientTextureHeaps; ++i)
		{
			m_rhiTransientTextureHeaps.append(m_rhiDevice->createHeap(heapDesc));
			m_rhiFreeTransientTextureHeaps.append(i);
		}
	}

	{
		constexpr int s_minTransientBufferHeaps = 4;

		nvrhi::HeapDesc heapDesc;
		nvrhiFillTransientBufferHeapDesc(heapDesc);

		m_rhiTransientBufferHeaps.reserve(s_minTransientBufferHeaps);
		m_rhiFreeTransientBufferHeaps.reserve(s_minTransientBufferHeaps);
		for (int i = 0; i < s_minTransientBufferHeaps; ++i)
		{
			m_rhiTransientBufferHeaps.append(m_rhiDevice->createHeap(heapDesc));
			m_rhiFreeTransientBufferHeaps.append(i);
		}
	}
}

void CNVRHIRenderAPI::Shutdown()
{
	ShaderAPI_Base::Shutdown();

	ASSERT_MSG(m_rhiCommandLists.numElem() == m_rhiFreeCommandLists.numElem(), "Found command lists were not executed. Tell programmer to fix that");

	m_shaderCache.clear(true);
	m_rhiTransientTextureHeaps.clear(true);
	m_rhiFreeTransientTextureHeaps.clear(true);
	m_rhiTransientBufferHeaps.clear(true);
	m_rhiFreeTransientBufferHeaps.clear(true);
	m_rhiCommandLists.clear(true);
	m_rhiFreeCommandLists.clear(true);
	m_rhiDevice = nullptr;
}

bool CNVRHIRenderAPI::IsDeviceValidationActive() const
{
	return nvrhi_validation.GetBool();
}

void CNVRHIRenderAPI::FreeShaderPackage(int id)
{
	if (id == 0)
		return;

	auto it = m_shaderCache.find(id);
	if (it.atEnd())
		return;

	for (ShaderInfo::Module module : it->modules)
	{
		if(module.rhiModule)
			reinterpret_cast<nvrhi::IShader*>(module.rhiModule)->Release();
	}

	DevMsg(DEVMSG_RENDER, "Freed shader package %s\n", it->shaderName.ToCString());
	m_shaderCache.remove(it);
}

void CNVRHIRenderAPI::ClearShaderPackages()
{
	for (auto it = m_shaderCache.begin(); !it.atEnd(); ++it)
	{
		for (ShaderInfo::Module module : it->modules)
		{
			if (module.rhiModule)
				reinterpret_cast<nvrhi::IShader*>(module.rhiModule)->Release();
		}
	}
	m_shaderCache.clear(true);
}

int CNVRHIRenderAPI::LoadShaderPackage(const char* filename)
{
	IPackFileReaderPtr shaderPackFile = g_fileSystem->OpenPackage(filename, SP_MOD | SP_DATA);
	if (!shaderPackFile)
	{
		MsgError("Cannot open shader package '%s'\n", filename);
		return 0;
	}

	KVSection shaderInfoKvs;
	{
		IFileStreamPtr file = shaderPackFile->Open("ShaderInfo", FS_OPEN_READ);
		if (!KeyValues::Parse(file, shaderInfoKvs))
		{
			Msg("No ShaderInfo in file %s\n", filename);
			return 0;
		}
	}

	DevMsg(DEVMSG_RENDER, "Loading shader package %s\n", shaderInfoKvs.GetName());
	if (!CString::SubString(filename, shaderInfoKvs.GetName()))
	{
		ASSERT_FAIL("Shader package '%s' file name doesn't match it's name '%s' in desc", filename, shaderInfoKvs.GetName());
		return 0;
	}

	const int shaderNameId = StringId24(shaderInfoKvs.GetName());
	auto it = m_shaderCache.find(shaderNameId);
	if (!it.atEnd())
	{
		ASSERT_FAIL("Shader '%s' has been already loaded from different package", shaderInfoKvs.GetName());
		return 0;
	}

	it = m_shaderCache.insert(shaderNameId);

	ShaderInfo& shaderInfo = *it;

	int filesFound = 0;
	if (!ShaderInfo::ParseShaderInfo(shaderInfo, shaderPackFile, shaderInfoKvs, filesFound))
	{
		m_shaderCache.remove(it);
		return 0;
	}

	if (nvrhi_preloadShaders.GetBool())
	{
		for (int i = 0; i < shaderInfo.modules.numElem(); ++i)
			GetOrLoadShaderModule(shaderInfo, i);
	}

	DevMsg(DEVMSG_RENDER, "Loaded %d shader modules from %s package\n", filesFound, shaderInfoKvs.GetName());

	return shaderNameId;
}

void CNVRHIRenderAPI::ReloadShaderPackage(int id)
{
	ASSERT_FAIL("Not implemented yet");
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

const char* CNVRHIRenderAPI::GetRendererName() const
{
	switch (m_rhiBackendType)
	{
	case NVRHI_BACKEND_D3D11:
		return "NVRHI/D3D11";
	case NVRHI_BACKEND_D3D12:
		return "NVRHI/D3D12";
	case NVRHI_BACKEND_VULKAN:
		return "NVRHI/Vulkan";
	}
	return "NVRHI/UNK";
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

int CNVRHIRenderAPI::AcquireRHITransientTextureHeap()
{
	int heapIdx = -1;
	CScopedMutex m(s_transientHeapsMutex);
	if (m_rhiFreeTransientTextureHeaps.isEmpty())
	{
		// allocate extra heap at device
		nvrhi::HeapDesc heapDesc;
		nvrhiFillTransientTextureHeapDesc(heapDesc);
		heapIdx = m_rhiTransientTextureHeaps.append(m_rhiDevice->createHeap(heapDesc));

		DevMsg(DEVMSG_RENDER, "RHI allocating extra transient texture heap\n");
	}
	else
	{
		heapIdx = m_rhiFreeTransientTextureHeaps.popBack();
		
	}
	return heapIdx;
}

void CNVRHIRenderAPI::ReleaseRHITransientTextureHeap(int heapIdx)
{
	if (heapIdx == -1)
		return;

	CScopedMutex m(s_transientHeapsMutex);
	m_rhiFreeTransientTextureHeaps.append(heapIdx);
}

int CNVRHIRenderAPI::AcquireRHITransientBufferHeap()
{
	int heapIdx = -1;
	CScopedMutex m(s_transientHeapsMutex);
	if (m_rhiFreeTransientBufferHeaps.isEmpty())
	{
		// allocate extra heap at device
		nvrhi::HeapDesc heapDesc;
		nvrhiFillTransientBufferHeapDesc(heapDesc);
		heapIdx = m_rhiTransientBufferHeaps.append(m_rhiDevice->createHeap(heapDesc));

		DevMsg(DEVMSG_RENDER, "RHI allocating extra transient buffer heap\n");
	}
	else
	{
		heapIdx = m_rhiFreeTransientBufferHeaps.popBack();
	}
	return heapIdx;
}

void CNVRHIRenderAPI::ReleaseRHITransientBufferHeap(int heapIdx)
{
	if (heapIdx == -1)
		return;

	CScopedMutex m(s_transientHeapsMutex);
	m_rhiFreeTransientBufferHeaps.append(heapIdx);
}

nvrhi::CommandListHandle CNVRHIRenderAPI::AcquireRHICommandList(int& cmdListIdx) const
{
	CScopedMutex m(s_cmdListMutex);
	cmdListIdx = -1;
	if (m_rhiFreeCommandLists.isEmpty())
	{
		auto rhiCmdListParams = nvrhi::CommandListParameters()
			.setEnableImmediateExecution(false)
			.setUploadChunkSize(512 * 1024)
			.setScratchChunkSize(64 * 1024);

		cmdListIdx = m_rhiCommandLists.append(m_rhiDevice->createCommandList(rhiCmdListParams));
	}
	else
	{
		cmdListIdx = m_rhiFreeCommandLists.popBack();
	}

	return m_rhiCommandLists[cmdListIdx];
}

void CNVRHIRenderAPI::ReleaseCommandList(int cmdListIdx)
{
	if (cmdListIdx == -1)
		return;
	CScopedMutex m(s_cmdListMutex);
	m_rhiFreeCommandLists.append(cmdListIdx);
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
	const bool isStorage = (flags & TEXFLAG_STORAGE) != 0;

	texture->SetDimensions(newSize.width, newSize.height, newSize.arraySize);
	texture->SetMipCount(mipmapCount);
	texture->SetSampleCount(sampleCount);
	texture->Release();

	auto rhiTextureDesc = nvrhi::TextureDesc()
		.setMipLevels(mipmapCount)
		.setSampleCount(sampleCount)
		.setIsUAV(isStorage)
		.setIsTypeless(isStorage)
		.setFormat(GetNVRHITextureFormat(texture->GetFormat()));

	const bool isDepth = IsDepthFormat(texture->GetFormat());

	const int arrayLayerCount = isCubeMap ? ITexture::CubeArraySlice(0, newSize.arraySize) : newSize.arraySize;
	rhiTextureDesc
		.setDebugName(texture->GetName())
		.setWidth((uint)newSize.width)
		.setHeight((uint)newSize.height)
		.setArraySize((uint)newSize.arraySize)
		.setIsRenderTarget(true)
		.setIsVirtual(flags & TEXFLAG_TRANSIENT);

	if (!isDepth)
	{
		rhiTextureDesc
			.setInitialState(nvrhi::ResourceStates::RenderTarget)
			.setKeepInitialState(true);
	}

	if (flags & TEXFLAG_CUBEMAP)
	{
		rhiTextureDesc.dimension = (newSize.arraySize > 1) ? nvrhi::TextureDimension::TextureCubeArray : nvrhi::TextureDimension::TextureCube;
		rhiTextureDesc.arraySize *= 6;
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
		ASSERT_FAIL("Failed to create render target %s\n", texture->GetName());
		return;
	}

	texture->m_rhiTexture = rhiTexture;
	texture->m_rhiDimension = rhiTextureDesc.dimension;

	if (flags & TEXFLAG_TRANSIENT)
	{
		const int heapIdx = AcquireRHITransientTextureHeap();
		texture->m_transientHeapIdx = heapIdx;
		m_rhiDevice->bindTextureMemory(rhiTexture, m_rhiTransientTextureHeaps[heapIdx], 0);
	}

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

IGPUBindingLayoutPtr CNVRHIRenderAPI::CreateBindingLayout(const BindingLayoutDesc& layoutDesc) const
{
	CRefPtr<CNVRHIBindingLayout> pipelineLayout = CRefPtr_new(CNVRHIBindingLayout);
	pipelineLayout->m_dbgName = layoutDesc.name;

	// make name to index map
	int bindGroupIdx = 0;
	for (const BindGroupLayoutDesc& bindGroupDesc : layoutDesc.bindGroups)
	{
		CNVRHIBindingLayout::BindGroupLayoutOrder& layoutOrder = pipelineLayout->m_layoutOrder.append();
		for (const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
		{
			layoutOrder.append(CNVRHIBindingLayout::EntryId{ entry.nameId, entry.visibility });
			pipelineLayout->m_maxBindingIndex[bindGroupIdx] = max(pipelineLayout->m_maxBindingIndex[bindGroupIdx], entry.binding);
		}
		++bindGroupIdx;
	}

	return IGPUBindingLayoutPtr(pipelineLayout);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroupImpl(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHIBindingLayoutsCRef rhiBindingLayouts) const
{
	if (!rhiBindingLayouts.inRange(bindGroupDesc.groupIdx))
	{
		ASSERT_FAIL("invalid binding group index %d", bindGroupDesc.groupIdx);
		return nullptr;
	}

	nvrhi::BindingSetDesc rhiBindingSetDesc;
	nvrhiFillBindingSetDesc(bindGroupDesc, shaderInfo, shaderModuleIdxs, rhiBindingSetDesc);

	nvrhi::BindingSetHandle rhiBindSet = m_rhiDevice->createBindingSet(rhiBindingSetDesc, rhiBindingLayouts[bindGroupDesc.groupIdx]);
	if (!rhiBindSet)
	{
		ASSERT_FAIL("Failed to create bind group %s\n", bindGroupDesc.name.ToCString());
		return nullptr;
	}

	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_dbgName = bindGroupDesc.name;
	bindGroup->m_rhiBindingSets.insert(0, std::move(rhiBindSet));

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateSharedBindGroup(const IGPUBindingLayout* bindingLayout, const BindGroupDesc& bindGroupDesc) const
{
	if (!bindingLayout)
	{
		ASSERT_FAIL("bindingLayout is null");
		return nullptr;
	}

	const CNVRHIBindingLayout* bindingLayoutImpl = static_cast<const CNVRHIBindingLayout*>(bindingLayout);

	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_bindingLayout.Assign(bindingLayoutImpl);
	bindGroup->m_dbgName = bindGroupDesc.name;
	bindGroup->MakeResourceRefs(bindGroupDesc);

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!renderPipeline)
	{
		ASSERT_FAIL("renderPipeline is null");
		return nullptr;
	}

	const CNVRHIRenderPipeline* pipelineImpl = static_cast<const CNVRHIRenderPipeline*>(renderPipeline);
	const int moduleIds[] = {
		pipelineImpl->m_vertexShaderModuleIdx,
		pipelineImpl->m_fragmentShaderModuleIdx
	};
	const nvrhi::BindingLayoutVector& bindingLayouts = pipelineImpl->m_rhiPipelineDesc.bindingLayouts;
	return CreateBindGroupImpl(bindGroupDesc, *pipelineImpl->m_shaderInfo, moduleIds, ArrayCRef(&bindingLayouts.front(), bindingLayouts.size()));
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!computePipeline)
	{
		ASSERT_FAIL("computePipeline is null");
		return nullptr;
	}

	const CNVRHIComputePipeline* pipelineImpl = static_cast<const CNVRHIComputePipeline*>(computePipeline);
	return CreateBindGroupImpl(bindGroupDesc, *pipelineImpl->m_shaderInfo, ArrayCRef(&pipelineImpl->m_computeShaderModuleIdx, 1), pipelineImpl->m_rhiBindingLayout);
}

const ShaderInfo::Module& CNVRHIRenderAPI::GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx, const char* dbgName) const
{
	ShaderInfo::Module& mod = const_cast<ShaderInfo::Module&>(shaderInfo.modules[shaderModuleIdx]);
	if (mod.rhiModule)
		return mod;

	BitArray& usedBindings = mod.usedBindings;
	usedBindings.resize(shaderInfo.GetBindingIds(mod).numElem());

	CMemoryStream shaderBlobData(PP_SL);
	auto loadShaderBlob = [&](EShaderModuleType type)
	{
		IFileStreamPtr shaderFile = shaderInfo.shaderPackFile->Open(mod.fileIndex[type], FS_OPEN_READ);
		if (!shaderFile)
			return;

		shaderBlobData.Open(FS_OPEN_WRITE | FS_OPEN_READ);
		int blobSize;
		shaderFile->ReadObj(blobSize);
		shaderBlobData.AppendStream(shaderFile, blobSize);

		shaderFile->Seek(blobSize, FS_SEEK_CUR);
		shaderFile->ReadArray(usedBindings.ptr(), bitArray2Dword(usedBindings.numBits()));
	};

	const EqString shaderModuleName = EqString::Format("%s-%d", shaderInfo.shaderName.ToCString(), shaderModuleIdx);

	nvrhi::ShaderHandle rhiShaderModule = nullptr;
	nvrhi::ShaderDesc rhiShaderDesc{};
	rhiShaderDesc.debugName = shaderModuleName;
	rhiShaderDesc.entryName = mod.entryPoint;

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

	if (m_rhiBackendType == NVRHI_BACKEND_D3D11)
		loadShaderBlob(SHADERMODULE_DXBC);
	else if (m_rhiBackendType == NVRHI_BACKEND_D3D12)
		loadShaderBlob(SHADERMODULE_DXIL);
	else if (m_rhiBackendType == NVRHI_BACKEND_VULKAN)
		loadShaderBlob(SHADERMODULE_SPIRV);

	if (!shaderBlobData.IsValid())
	{
		ASSERT_FAIL("Shader module %s (found in package %s) not found for specific backend", shaderModuleName.ToCString(), shaderInfo.shaderName.ToCString());
		return mod;
	}

	rhiShaderModule = m_rhiDevice->createShader(rhiShaderDesc, shaderBlobData.GetBasePointer(), shaderBlobData.GetSize());
	
	if (!rhiShaderModule)
	{
		MsgError("Can't create shader module %s!\n", dbgName ? dbgName : shaderModuleName.ToCString());
		return mod;
	}

	rhiShaderModule->AddRef();
	mod.rhiModule = rhiShaderModule;

	return mod;
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

	const ShaderInfo& shaderInfo = *shaderIt;
	int queryStrHash = 0;
	if (!shaderInfo.GetShaderQueryHash(defines, queryStrHash))
	{
		MsgError("LoadShaderModules: unknown defines in query for shader '%s'\n", shaderName);
		return;
	}

	const int entryPointStrHash = StringId24(entryPointName);

	for (int i = 0; i < shaderInfo.vertexLayouts.numElem(); ++i)
	{
		const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[i];
		if (layout.aliasOf != -1)
			continue;

		if (shaderInfo.shaderKinds & SHADERKIND_FRAGMENT)
		{
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, i, SHADERKIND_FRAGMENT, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_VERTEX)
		{
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, i, SHADERKIND_VERTEX, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
		if (shaderInfo.shaderKinds & SHADERKIND_COMPUTE)
		{
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, i, SHADERKIND_COMPUTE, entryPointStrHash);
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
				GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
	}
}

nvrhi::SamplerHandle CNVRHIRenderAPI::GetRHISampler(const SamplerStateParams& samplerStateParams)
{
	const uint samplerId = samplerStateParams.minFilter
		| (samplerStateParams.magFilter << 3)		// 3
		| (samplerStateParams.mipmapFilter << 6)	// 3
		| (samplerStateParams.compareFunc << 9)		// 3
		| (samplerStateParams.addressU << 11)		// 2
		| (samplerStateParams.addressV << 13)		// 2
		| (samplerStateParams.addressW << 15)		// 2
		| (samplerStateParams.maxAnisotropy << 24);

	auto it = m_rhiSamplers.find(samplerId);
	if (it)
		return *it;

	auto rhiSamplerDesc = nvrhi::SamplerDesc();
	nvrhiFillSamplerDesc(samplerStateParams, rhiSamplerDesc);

	nvrhi::SamplerHandle rhiSampler = m_rhiDevice->createSampler(rhiSamplerDesc);
	m_rhiSamplers.insert(samplerId, rhiSampler);

	return rhiSampler;
}

IGPURenderPipelinePtr CNVRHIRenderAPI::CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUBindingLayout* bindingLayout) const
{
	PROF_EVENT("CreateRenderPipeline");

	const int shaderNameHash = StringId24(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Render pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfo& shaderInfo = *shaderIt;
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

	int vertexLayoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfo::VertLayout& layout) {
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

	auto rhiGraphicsPipelineDesc = nvrhi::GraphicsPipelineDesc();

	// Setup vertex pipeline
	// Required
	nvrhi::InputLayoutHandle rhiInputLayout;
	int vertexShaderModuleIdx = -1;
	int fragmentShaderModuleIdx = -1;
	nvrhi::IShader* vertexShader = nullptr;
	nvrhi::IShader* fragmentShader = nullptr;
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");

		{
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX, StringId24(pipelineDesc.vertex.shaderEntryPoint));
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (itShaderModuleId)
			{
				vertexShaderModuleIdx = *itShaderModuleId;
				ASSERT_MSG(shaderInfo.modules[vertexShaderModuleIdx].kind == SHADERKIND_VERTEX,
					"Incorrect shader kind for %s %s in shader package %s",
					shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(),
					shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(),
					pipelineDesc.shaderName.ToCString());
				vertexShader = reinterpret_cast<nvrhi::IShader*>(GetOrLoadShaderModule(shaderInfo, vertexShaderModuleIdx).rhiModule);
			}
		}

		if (!vertexShader)
		{
			ASSERT_FAIL("No vertex shader module found for %s %s in shader package %s", 
				shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), 
				shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(), 
				pipelineDesc.shaderName.ToCString());
			return nullptr;
		}

		ArrayCRef<int> vertexAttribIds = shaderInfo.GetVertexAttribIds(shaderInfo.modules[vertexShaderModuleIdx]);
		BitArray usedVertexAttribs(PP_SL, vertexAttribIds.numElem());

		FixedArray<nvrhi::VertexAttributeDesc, 48> rhiVertexAttribList;
		int bufferIndex = 0;
		for (const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			for (const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				if (attrib.format == ATTRIBUTEFORMAT_NONE)
					continue;

				const int attribIdIdx = arrayFindIndexF(vertexAttribIds, [&](const int attribIdx) {
					const ShaderInfo::VertexAttrib& shaderAttrib = shaderInfo.vertexAttribs[attribIdx];
					return shaderAttrib.nameId == attrib.nameId;
				});

				if (attribIdIdx == -1)
					continue;	// not used, check later

				const ShaderInfo::VertexAttrib& shaderAttrib = shaderInfo.vertexAttribs[vertexAttribIds[attribIdIdx]];
				usedVertexAttribs.setTrue(attribIdIdx);

				ASSERT_MSG(!rhiVertexAttribList.isFull(), "Too many vertex attributes");

				auto rhiVertAttr = rhiVertexAttribList.append()
					.setName(shaderAttrib.semantic.ToCString())
					.setFormat(g_nvrhiVertexFormats[attrib.format][attrib.count - 1])
					.setOffset(attrib.offset)
					.setBufferIndex(bufferIndex)	// TODO
					.setIsInstanced(vertexLayout.stepMode == VERTEX_STEPMODE_INSTANCE)
					.setElementStride(vertexLayout.stride);
			}
			++bufferIndex;
		}
#ifdef DEBUG_SHADER_BINDINGS
		if (usedVertexAttribs.numTrue() < vertexAttribIds.numElem())
		{
			for (int i = 0; i < vertexAttribIds.numElem(); ++i)
			{
				if (usedVertexAttribs[i])
					continue;

				const ShaderInfo::VertexAttrib& shaderAttrib = shaderInfo.vertexAttribs[vertexAttribIds[i]];
				ASSERT_FAIL("Vertex attrib %s not used while creating pipeline %s:%s", shaderAttrib.name.ToCString(), shaderInfo.shaderName.ToCString(), shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString());
			}
		}
#endif

		if(!pipelineDesc.vertex.vertexLayout.isEmpty())
		{
			ASSERT_MSG(!rhiVertexAttribList.isEmpty(), "No vertex attributes - invalid vertex format");
		}

		rhiInputLayout = m_rhiDevice->createInputLayout(rhiVertexAttribList.ptr(), rhiVertexAttribList.numElem(), vertexShader);
		rhiGraphicsPipelineDesc.setVertexShader(vertexShader);
	}

	auto rhiFramebufferInfo = nvrhi::FramebufferInfo();
	rhiFramebufferInfo.sampleCount = pipelineDesc.multiSample.count;
	
	// Depth state
	// Optional when depth read = false
	if (pipelineDesc.depthStencil.format != FORMAT_NONE)
	{
		rhiFramebufferInfo.depthFormat = GetNVRHITextureFormat(pipelineDesc.depthStencil.format);

		auto& rhiDepthStencil = rhiGraphicsPipelineDesc.renderState.depthStencilState;
		rhiDepthStencil.depthWriteEnable = pipelineDesc.depthStencil.depthWrite;
		rhiDepthStencil.depthFunc = pipelineDesc.depthStencil.depthTest ? g_nvrhiCompareFunc[pipelineDesc.depthStencil.depthFunc] : nvrhi::ComparisonFunc::Always;
		rhiDepthStencil.stencilReadMask = pipelineDesc.depthStencil.stencilMask;
		rhiDepthStencil.stencilWriteMask = pipelineDesc.depthStencil.stencilWriteMask;

		auto& rhiRasterState = rhiGraphicsPipelineDesc.renderState.rasterState;
		rhiRasterState.depthBias = pipelineDesc.depthStencil.depthBias;
		rhiRasterState.slopeScaledDepthBias = pipelineDesc.depthStencil.depthBiasSlopeScale;
		rhiRasterState.depthBiasClamp = 0; // TODO

		rhiDepthStencil.stencilRefValue = pipelineDesc.depthStencil.stencilRef;
		rhiDepthStencil.stencilEnable = pipelineDesc.depthStencil.stencilTest;

		// back
		rhiDepthStencil.backFaceStencil.stencilFunc = g_nvrhiCompareFunc[pipelineDesc.depthStencil.stencilBack.compareFunc];
		rhiDepthStencil.backFaceStencil.failOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilBack.failOp];
		rhiDepthStencil.backFaceStencil.depthFailOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilBack.depthFailOp];
		rhiDepthStencil.backFaceStencil.passOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilBack.passOp];

		// front
		rhiDepthStencil.frontFaceStencil.stencilFunc = g_nvrhiCompareFunc[pipelineDesc.depthStencil.stencilFront.compareFunc];
		rhiDepthStencil.frontFaceStencil.failOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilFront.failOp];
		rhiDepthStencil.frontFaceStencil.depthFailOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilFront.depthFailOp];
		rhiDepthStencil.frontFaceStencil.passOp = g_nvrhiStencilOp[pipelineDesc.depthStencil.stencilFront.passOp];
	}
	else
	{
		auto& rhiDepthStencil = rhiGraphicsPipelineDesc.renderState.depthStencilState;
		rhiDepthStencil.depthTestEnable = false;
		rhiDepthStencil.depthWriteEnable = false;
		rhiDepthStencil.stencilEnable = false;
	}

	// Setup fragment pipeline
	// Fragment state
	// When opted out, requires rhiDepthStencil state
	if(pipelineDesc.fragment.shaderEntryPoint.Length())
	{
		{
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT, StringId24(pipelineDesc.fragment.shaderEntryPoint));
			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (itShaderModuleId)
			{
				fragmentShaderModuleIdx = *itShaderModuleId;

				ASSERT_MSG(shaderInfo.modules[fragmentShaderModuleIdx].kind == SHADERKIND_FRAGMENT,
					"Incorrect shader kind for %s %s in shader package %s",
					shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), 
					shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(),
					pipelineDesc.shaderName.ToCString());

				fragmentShader = reinterpret_cast<nvrhi::IShader*>(GetOrLoadShaderModule(shaderInfo, fragmentShaderModuleIdx).rhiModule);
			}
		}

		if(!fragmentShader)
		{
			ASSERT_FAIL("No fragment shader module found for %s %s in shader package %s", 
				shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), 
				shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(), 
				pipelineDesc.shaderName.ToCString());
			return nullptr;
		}
		rhiGraphicsPipelineDesc.setPixelShader(fragmentShader);

		auto& rhiBlendState = rhiGraphicsPipelineDesc.renderState.blendState;
		int targetNum = 0;
		for (const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
		{
			rhiBlendState.targets[targetNum]
				.setBlendEnable(target.blendEnable)
				.setBlendOp(g_nvrhiBlendOp[target.colorBlend.blendFunc])
				.setSrcBlend(g_nvrhiBlendFactor[target.colorBlend.srcFactor])
				.setDestBlend(g_nvrhiBlendFactor[target.colorBlend.dstFactor])
				.setBlendOpAlpha(g_nvrhiBlendOp[target.alphaBlend.blendFunc])
				.setSrcBlendAlpha(g_nvrhiBlendFactor[target.alphaBlend.srcFactor])
				.setDestBlendAlpha(g_nvrhiBlendFactor[target.alphaBlend.dstFactor])
				.setColorWriteMask((nvrhi::ColorMask)target.writeMask);

			rhiFramebufferInfo.colorFormats.push_back(GetNVRHITextureFormat(target.format));

			targetNum++;
		}
	}

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString());

	const int shaderModuleIdxs[] = {
		vertexShaderModuleIdx,
		fragmentShaderModuleIdx
	};

	NVRHIBindingLayoutList rhiBindingLayouts;
	nvrhiCreateBindingLayouts(shaderInfo, bindingLayout, shaderModuleIdxs, nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel, rhiBindingLayouts);
	for (nvrhi::BindingLayoutHandle rhiLayout : rhiBindingLayouts)
		rhiGraphicsPipelineDesc.addBindingLayout(rhiLayout);

	if (pipelineDesc.depthStencil.format == FORMAT_NONE && pipelineDesc.fragment.shaderEntryPoint.Length() == 0)
	{
		ASSERT_FAIL("Render pipeline requires either depthStencil or fragment states (or both)");
		return nullptr;
	}

	// Multisampling
	rhiGraphicsPipelineDesc.renderState.rasterState.multisampleEnable = pipelineDesc.multiSample.count > 1;

	// Primitive toplogy
	rhiGraphicsPipelineDesc.renderState.rasterState.cullMode = g_nvrhiCullMode[pipelineDesc.primitive.cullMode];
	rhiGraphicsPipelineDesc.setPrimType(g_nvrhiPrimitiveType[pipelineDesc.primitive.topology]);
	rhiGraphicsPipelineDesc.setInputLayout(rhiInputLayout);

	// TODO: use framebuffer info and VK_KHR_dynamic_rendering on Vulkan
	CRefPtr<CNVRHIRenderPipeline> renderPipeline;
#if 1
	{
		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		nvrhi::GraphicsPipelineHandle rhiRenderPipeline;
		g_renderWorker.WaitForExecute(__func__, [&]() {
			rhiRenderPipeline = m_rhiDevice->createGraphicsPipeline(rhiGraphicsPipelineDesc, rhiFramebufferInfo);
			return 0;
		});
		if (!rhiRenderPipeline)
		{
			ASSERT_FAIL("Render pipeline creation failed");
			return nullptr;
		}

		renderPipeline = CRefPtr_new(CNVRHIRenderPipeline);
		renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;
	}
#else
	renderPipeline = CRefPtr_new(CNVRHIRenderPipeline);
#endif

	renderPipeline->m_shaderInfo = &shaderInfo;
	renderPipeline->m_rhiFramebufferinfo = rhiFramebufferInfo;
	renderPipeline->m_rhiPipelineDesc = rhiGraphicsPipelineDesc;
	renderPipeline->m_dbgName = std::move(pipelineName);
	renderPipeline->m_vertexShaderModuleIdx = vertexShaderModuleIdx;
	renderPipeline->m_fragmentShaderModuleIdx = fragmentShaderModuleIdx;
	renderPipeline->m_pipelineId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, 0, StringId24(pipelineDesc.vertex.shaderEntryPoint));

	return IGPURenderPipelinePtr(renderPipeline);
}

IGPUComputePipelinePtr CNVRHIRenderAPI::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc, const IGPUBindingLayout* bindingLayout) const
{
	const int shaderNameHash = StringId24(pipelineDesc.shaderName);
	auto shaderIt = m_shaderCache.find(shaderNameHash);
	if (shaderIt.atEnd())
	{
		ASSERT_FAIL("Compute pipeline has unknown shader '%s' specified", pipelineDesc.shaderName.ToCString());
		return nullptr;
	}

	const ShaderInfo& shaderInfo = *shaderIt;
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

	int layoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfo::VertLayout& layout) {
		return layout.nameHash == pipelineDesc.shaderLayoutId;
	});

	if (layoutIdx == -1)
	{
		ASSERT_FAIL("Compute pipeline %s has unknown layout id %d", pipelineDesc.shaderName.ToCString(), pipelineDesc.shaderLayoutId);
		return nullptr;
	}
	if (shaderInfo.vertexLayouts[layoutIdx].aliasOf != -1)
		layoutIdx = shaderInfo.vertexLayouts[layoutIdx].aliasOf;

	const ShaderInfo::Module* computeShaderModule = nullptr;
	int computeShaderModuleIdx = -1;
	{
		const int entryPointStrHash = StringId24(pipelineDesc.shaderEntryPoint);
		const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, layoutIdx, SHADERKIND_COMPUTE, entryPointStrHash);
		auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

		if (itShaderModuleId)
		{
			computeShaderModuleIdx = *itShaderModuleId;
			ASSERT_MSG(shaderInfo.modules[computeShaderModuleIdx].kind == SHADERKIND_COMPUTE,
				"Incorrect shader kind for %s %s in shader package %s", 
				shaderInfo.vertexLayouts[layoutIdx].name.ToCString(),
				shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(),
				pipelineDesc.shaderName.ToCString());

			computeShaderModule = &GetOrLoadShaderModule(shaderInfo, computeShaderModuleIdx);
		}
	}

	auto rhiComputePipelineDesc = nvrhi::ComputePipelineDesc()
		.setComputeShader(reinterpret_cast<nvrhi::IShader*>(computeShaderModule->rhiModule));

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[layoutIdx].name.ToCString());

	NVRHIBindingLayoutList rhiBindingLayouts;
	nvrhiCreateBindingLayouts(shaderInfo, bindingLayout, ArrayCRef(&computeShaderModuleIdx, 1), nvrhi::ShaderType::Compute, rhiBindingLayouts);
	for (nvrhi::BindingLayoutHandle rhiLayout : rhiBindingLayouts)
		rhiComputePipelineDesc.addBindingLayout(rhiLayout);

	{
		PROF_EVENT(EqString::Format("CreateComputePipeline for %s", pipelineName.ToCString()));
		nvrhi::ComputePipelineHandle rhiComputePipeline = m_rhiDevice->createComputePipeline(rhiComputePipelineDesc);
		if (!rhiComputePipeline)
		{
			ASSERT_FAIL("Compute pipeline %s creation failed", pipelineName.ToCString());
			return nullptr;
		}

		CRefPtr<CNVRHIComputePipeline> computePipeline = CRefPtr_new(CNVRHIComputePipeline);
		computePipeline->m_shaderInfo = &shaderInfo;
		computePipeline->m_rhiComputePipeline = rhiComputePipeline;
		computePipeline->m_dbgName = std::move(pipelineName);
		computePipeline->m_computeShaderModuleIdx = computeShaderModuleIdx;
		computePipeline->m_pipelineId = ShaderInfo::PackShaderModuleId(queryStrHash, layoutIdx, 0, StringId24(pipelineDesc.shaderEntryPoint));

		for(int i = 0; i < rhiComputePipelineDesc.bindingLayouts.size(); ++i)
			computePipeline->m_rhiBindingLayout.append(rhiComputePipelineDesc.bindingLayouts[i]);

		return IGPUComputePipelinePtr(computePipeline);
	}
}

IGPUCommandRecorderPtr CNVRHIRenderAPI::CreateCommandRecorder(const char* name, void* userData) const
{
	auto rhiCmdListParams = nvrhi::CommandListParameters()
		.setEnableImmediateExecution(false)
		.setUploadChunkSize(512 * 1024)
		.setScratchChunkSize(64 * 1024);

	int cmdListIdx = -1;
	nvrhi::CommandListHandle rhiCommandList = AcquireRHICommandList(cmdListIdx);
	rhiCommandList->open();

	CRefPtr<CNVRHICommandRecorder> commandRecorder = CRefPtr_new(CNVRHICommandRecorder);
	commandRecorder->m_dbgName = name;
	commandRecorder->m_rhiCommandList = rhiCommandList;
	commandRecorder->m_userData = userData;
	commandRecorder->m_cmdListIdx = cmdListIdx;

	return IGPUCommandRecorderPtr(commandRecorder);
}

IGPURenderPassRecorderPtr CNVRHIRenderAPI::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	int cmdListIdx = -1;
	nvrhi::CommandListHandle rhiCommandList = AcquireRHICommandList(cmdListIdx);
	rhiCommandList->open();

	CRefPtr<CNVRHIRenderPassRecorder> renderPass = CRefPtr_new(CNVRHIRenderPassRecorder, rhiCommandList, cmdListIdx, userData);
	renderPass->InternalBeginRenderPass(renderPassDesc);

	return IGPURenderPassRecorderPtr(renderPass);
}

IGPUComputePassRecorderPtr CNVRHIRenderAPI::BeginComputePass(const char* name, void* userData) const
{
	int cmdListIdx = -1;
	nvrhi::CommandListHandle rhiCommandList = AcquireRHICommandList(cmdListIdx);
	rhiCommandList->open();

	CRefPtr<CNVRHIComputePassRecorder> renderPass = CRefPtr_new(CNVRHIComputePassRecorder, rhiCommandList, cmdListIdx, userData, name);
	return IGPUComputePassRecorderPtr(renderPass);
}

void CNVRHIRenderAPI::SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	if (cmdBuffers.isEmpty())
		return;

	PROF_EVENT_F();

	static thread_local Array<int> pendingCmdListIdxs(PP_SL);
	static thread_local Array<nvrhi::ICommandList*> rhiSubmitBuffers(PP_SL);
	rhiSubmitBuffers.clear();
	rhiSubmitBuffers.reserve(cmdBuffers.numElem());
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;

		const CNVRHICommandBuffer* bufferImpl = static_cast<const CNVRHICommandBuffer*>(cmdBuffer);
		
		ASSERT(bufferImpl->m_rhiCommandList);
		rhiSubmitBuffers.append(bufferImpl->m_rhiCommandList);
		pendingCmdListIdxs.append(bufferImpl->m_cmdListIdx);
	}
	if (rhiSubmitBuffers.isEmpty())
		return;

	uint64_t lastSubmitInstance;
	g_renderWorker.WaitForExecute(__func__, [&lastSubmitInstance, this, rhiCmdLists = ArrayCRef<nvrhi::ICommandList*>(rhiSubmitBuffers)]() {
		lastSubmitInstance = m_rhiDevice->executeCommandLists(rhiCmdLists.ptr(), rhiCmdLists.numElem());
		m_rhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Graphics, lastSubmitInstance);
		return 0;
	});

	{
		CScopedMutex m(s_cmdListMutex);
		for(int cmdListIdx : pendingCmdListIdxs)
			m_rhiFreeCommandLists.append(cmdListIdx);
		pendingCmdListIdxs.clear();
	}
}

Future<bool> CNVRHIRenderAPI::SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	if (cmdBuffers.isEmpty())
		return Future<bool>::Succeed(true);

	static thread_local Array<int> executingCmdLists(PP_SL);
	executingCmdLists.reserve(cmdBuffers.numElem());
	executingCmdLists.clear();
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;

		const CNVRHICommandBuffer* bufferImpl = static_cast<const CNVRHICommandBuffer*>(cmdBuffer);
		executingCmdLists.append(bufferImpl->m_cmdListIdx);
	}

	if(executingCmdLists.isEmpty())
		return Future<bool>::Succeed(true);

	Promise<bool> promise;
	g_renderWorker.Execute(__func__, [this, cmdListIdxs = executingCmdLists, promise]() {
		static Array<nvrhi::ICommandList*> rhiCmdLists{ PP_SL };
		rhiCmdLists.reserve(cmdListIdxs.numElem());
		rhiCmdLists.clear(true);

		for (int cmdListIdx : cmdListIdxs)
			rhiCmdLists.append(m_rhiCommandLists[cmdListIdx]);

		uint64_t lastSubmitInstance = m_rhiDevice->executeCommandLists(rhiCmdLists.ptr(), rhiCmdLists.numElem());
		m_rhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Graphics, lastSubmitInstance);

		// release command lists after execution
		{
			CScopedMutex m(s_cmdListMutex);
			for (int cmdListIdx : cmdListIdxs)
				m_rhiFreeCommandLists.append(cmdListIdx);
		}
		promise.SetResult(true);
		return 0;
	});

	return promise.CreateFuture();
}

void CNVRHIRenderAPI::Flush()
{
	g_renderWorker.SignalWork();
}
