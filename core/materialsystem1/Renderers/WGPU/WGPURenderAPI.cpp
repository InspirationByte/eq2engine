//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#include "core/IPackFileReader.h"
#include "core/ConVar.h"
#include "imaging/ImageLoader.h"
#include "utils/KeyValues.h"

#include "../RenderWorker.h"

#include "WGPURenderAPI.h"
#include "WGPURenderDefs.h"
#include "WGPUStates.h"
#include "WGPUCommandRecorder.h"
#include "WGPURenderPassRecorder.h"
#include "WGPUComputePassRecorder.h"
#include "VertexFormat.h"

DECLARE_CVAR(wgpu_preloadShaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);
DECLARE_CVAR(wgpu_forceUseSPIRV, "0", "Use SPIR-V shaders provided in shader packages.", CV_ARCHIVE);

CWGPURenderAPI CWGPURenderAPI::Instance;
ShaderAPI_Base& ShaderAPI_Base::Instance = CWGPURenderAPI::Instance;
IShaderAPI* g_renderAPI = &CWGPURenderAPI::Instance;

//------------------------------------------

void CWGPURenderAPI::Shutdown()
{
	ShaderAPI_Base::Shutdown();
	m_rhiDevice = nullptr;
	m_rhiQueue = nullptr;
}

void CWGPURenderAPI::FreeShaderPackage(int id)
{
	if (id == 0)
		return;

	auto it = m_shaderCache.find(id);
	if (it.atEnd())
		return;

	for (ShaderInfo::Module module : it->modules)
	{
		if (module.rhiModule)
			wgpuShaderModuleRelease(reinterpret_cast<WGPUShaderModule>(module.rhiModule));
	}

	DevMsg(DEVMSG_RENDER, "Freed shader package %s\n", it->shaderName.ToCString());
	m_shaderCache.remove(it);
}

void CWGPURenderAPI::ClearShaderPackages()
{
	for (auto it = m_shaderCache.begin(); !it.atEnd(); ++it)
	{
		for (ShaderInfo::Module module : it->modules)
		{
			if(module.rhiModule)
				wgpuShaderModuleRelease(reinterpret_cast<WGPUShaderModule>(module.rhiModule));
		}
	}
	m_shaderCache.clear(true);
}

int CWGPURenderAPI::LoadShaderPackage(const char* filename)
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

	if (wgpu_preloadShaders.GetBool())
	{
		for (int i = 0; i < shaderInfo.modules.numElem(); ++i)
			GetOrLoadShaderModule(shaderInfo, i, nullptr);
	}

	DevMsg(DEVMSG_RENDER, "Loaded %d shader modules from %s package\n", filesFound, shaderInfoKvs.GetName());

	return shaderNameId;
}

void CWGPURenderAPI::ReloadShaderPackage(int id)
{
	auto it = m_shaderCache.find(id);
	if (it.atEnd())
		return;

	ShaderInfo& shaderInfo = *it;
	EqString packageName = shaderInfo.shaderPackFile->GetName();

	IPackFileReaderPtr shaderPackFile = g_fileSystem->OpenPackage(packageName, SP_MOD | SP_DATA);
	if (!shaderPackFile)
	{
		MsgError("Cannot open shader package '%s'\n", packageName.ToCString());
		return;
	}

	KVSection shaderInfoKvs;
	{
		IFileStreamPtr file = shaderPackFile->Open("ShaderInfo", FS_OPEN_READ);
		if (!KeyValues::Parse(file, shaderInfoKvs))
		{
			Msg("No ShaderInfo in file %s\n", packageName.ToCString());
			return;
		}
	}

	DevMsg(DEVMSG_RENDER, "Reloading shader package %s\n", shaderInfoKvs.GetName());
	if (!CString::SubString(packageName.ToCString(), shaderInfoKvs.GetName()))
	{
		ASSERT_FAIL("Shader package '%s' file name doesn't match it's name '%s' in desc", packageName.ToCString(), shaderInfoKvs.GetName());
		return;
	}

	// re-initialize shader info
	shaderInfo = {};

	int filesFound = 0;
	if (!ShaderInfo::ParseShaderInfo(shaderInfo, shaderPackFile, shaderInfoKvs, filesFound, false))
	{
		return;
	}

	if (wgpu_preloadShaders.GetBool())
	{
		for (int i = 0; i < shaderInfo.modules.numElem(); ++i)
			GetOrLoadShaderModule(shaderInfo, i, nullptr);
	}

	DevMsg(DEVMSG_RENDER, "Loaded %d shader modules from %s package\n", filesFound, shaderInfoKvs.GetName());
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

	DevMsg(DEVMSG_RENDER, "Resize render target %s (%dx%d -> %dx%d)\n", texture->GetName(), texture->GetWidth(), texture->GetHeight(), newSize.width, newSize.height);

	const int flags = texture->GetFlags();

	const bool isArray = newSize.arraySize > 1;
	const bool isCubeMap = (flags & TEXFLAG_CUBEMAP);

	texture->SetDimensions(newSize.width, newSize.height, newSize.arraySize);
	texture->SetMipCount(mipmapCount);
	texture->SetSampleCount(sampleCount);
	texture->Release();

	WGPUTextureUsage rhiUsageFlags = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
	if (flags & TEXFLAG_STORAGE) rhiUsageFlags |= WGPUTextureUsage_StorageBinding;
	if (flags & TEXFLAG_COPY_SRC) rhiUsageFlags |= WGPUTextureUsage_CopySrc;
	if (flags & TEXFLAG_COPY_DST) rhiUsageFlags |= WGPUTextureUsage_CopyDst;

	const int arrayLayerCount = isCubeMap ? ITexture::CubeArraySlice(0, newSize.arraySize) : newSize.arraySize;

	WGPUTextureDescriptor rhiTextureDesc = {};
	rhiTextureDesc.label = _WSTR(texture->GetName());
	rhiTextureDesc.mipLevelCount = mipmapCount;
	rhiTextureDesc.size = WGPUExtent3D{ (uint)newSize.width, (uint)newSize.height, (uint)arrayLayerCount };
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

	texture->m_rhiTexture = rhiTexture;

	// add default view
	{
		WGPUTextureViewDescriptor rhiTexViewDesc = {};
		rhiTexViewDesc.label = rhiTextureDesc.label;
		rhiTexViewDesc.format = GetWGPUTextureFormat(texture->GetFormat());
		rhiTexViewDesc.aspect = WGPUTextureAspect_All;
		rhiTexViewDesc.arrayLayerCount = arrayLayerCount;
		rhiTexViewDesc.baseArrayLayer = 0;
		rhiTexViewDesc.baseMipLevel = 0;
		rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;

		if (isArray)
			rhiTexViewDesc.dimension = isCubeMap ? WGPUTextureViewDimension_CubeArray : WGPUTextureViewDimension_2DArray;
		else
			rhiTexViewDesc.dimension = isCubeMap ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;

		WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
		texture->m_rhiViews.append(rhiView);
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
				WGPUTextureViewDescriptor rhiTexViewDesc = {};
				rhiTexViewDesc.label = rhiTextureDesc.label;
				rhiTexViewDesc.format = GetWGPUTextureFormat(texture->GetFormat());
				rhiTexViewDesc.aspect = WGPUTextureAspect_All;
				rhiTexViewDesc.arrayLayerCount = 1;
				rhiTexViewDesc.baseArrayLayer = ITexture::CubeArraySlice(i, slice);
				rhiTexViewDesc.baseMipLevel = 0;
				rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
				rhiTexViewDesc.dimension = WGPUTextureViewDimension_2D;

				WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
				texture->m_rhiViews.append(rhiView);
			}
		}
	}
	else if(isArray)
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

IGPUBindingLayoutPtr CWGPURenderAPI::CreateBindingLayout(const BindingLayoutDesc& layoutDesc) const
{
	FixedArray<WGPUBindGroupLayout, MAX_BINDGROUPS> rhiBindGroupLayout;
	static thread_local Array<WGPUBindGroupLayoutEntry> rhiBindGroupLayoutEntry(PP_SL);
	rhiBindGroupLayoutEntry.clear();

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
	rhiPipelineLayoutDesc.label = _WSTR(layoutDesc.name.Length() ? layoutDesc.name.ToCString() : nullptr);
	rhiPipelineLayoutDesc.bindGroupLayoutCount = rhiBindGroupLayout.numElem();
	rhiPipelineLayoutDesc.bindGroupLayouts = rhiBindGroupLayout.ptr();

	WGPUPipelineLayout rhiPipelineLayout = wgpuDeviceCreatePipelineLayout(m_rhiDevice, &rhiPipelineLayoutDesc);
	if (!rhiPipelineLayout)
		return nullptr;

	CRefPtr<CWGPUBindingLayout> pipelineLayout = CRefPtr_new(CWGPUBindingLayout);
	pipelineLayout->m_rhiBindGroupLayout.append(rhiBindGroupLayout);
	pipelineLayout->m_rhiPipelineLayout = rhiPipelineLayout;
	return IGPUBindingLayoutPtr(pipelineLayout);
}

static void FindWGPUBindGroupEntry(WGPUDevice rhiDevice, WGPUBindGroupEntry& rhiBindGroupEntryDesc, const BindGroupDesc::Entry& bindGroupEntry, const char* dbgName)
{
	rhiBindGroupEntryDesc.binding = bindGroupEntry.binding;
	switch (bindGroupEntry.type)
	{
	case BINDENTRY_BUFFER:
	{
		CWGPUBuffer* buffer = static_cast<CWGPUBuffer*>(bindGroupEntry.buffer.buffer.Ptr());
		if (buffer)
			rhiBindGroupEntryDesc.buffer = buffer->GetWGPUBuffer();
		else
			ASSERT_FAIL("NULL buffer for binding %d %s", bindGroupEntry.binding, dbgName);

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
			ASSERT_FAIL("NULL texture for binding %d %s", bindGroupEntry.binding, dbgName);
		break;
	}
}

static void FillWGPUBindGroupEntries(WGPUDevice rhiDevice, const BindGroupDesc& bindGroupDesc, Array<WGPUBindGroupEntry>& rhiBindGroupEntryList)
{
	for (const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		WGPUBindGroupEntry rhiBindGroupEntryDesc = {};
		FindWGPUBindGroupEntry(rhiDevice, rhiBindGroupEntryDesc, bindGroupEntry, "");
		rhiBindGroupEntryList.append(rhiBindGroupEntryDesc);
	}
}

static void FillWGPUBindGroupEntries(WGPUDevice rhiDevice, const BindGroupDesc& bindGroupDesc, Array<WGPUBindGroupEntry>& rhiBindGroupEntryList, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs)
{
	int bindingsToResolve = 0;

	static thread_local BitArray::STORAGE_TYPE usedBindEntryBits[32];
	memset(usedBindEntryBits, 0, sizeof(usedBindEntryBits));

	BitArray usedBindingEntries(usedBindEntryBits, sizeof(usedBindEntryBits) * 8);
	
	for (const int moduleIdx : shaderModuleIdxs)
	{
		if (moduleIdx < 0)
			continue;

		const ShaderInfo::Module& shaderModule = shaderInfo.modules[moduleIdx];
		ArrayCRef<int> bindingIds = shaderInfo.GetBindingIds(shaderModule);
		for (int i = 0; i < bindingIds.numElem(); ++i)
		{
			if (usedBindingEntries[bindingIds[i]])
				continue;

			if (!shaderModule.usedBindings[i])
				continue;

			const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIds[i]];
			if (binding.descriptorSetIdx != bindGroupDesc.groupIdx)
				continue;

			++bindingsToResolve;

			const int entryIdx = arrayFindIndexF(bindGroupDesc.entries, [&](const BindGroupDesc::Entry& bindGroupEntry) {
				// check if name id is used
				if (bindGroupEntry.binding > bindingIds.numElem())
					return bindGroupEntry.binding == binding.nameId;

				return bindGroupDesc.groupIdx == binding.descriptorSetIdx && bindGroupEntry.binding == binding.index;
			});

			if (entryIdx == -1)
				continue;

			usedBindingEntries.setTrue(bindingIds[i]);

			const BindGroupDesc::Entry& bindGroupEntry = bindGroupDesc.entries[entryIdx];
			WGPUBindGroupEntry& rhiBindGroupEntryDesc = rhiBindGroupEntryList.append();
			FindWGPUBindGroupEntry(rhiDevice, rhiBindGroupEntryDesc, bindGroupEntry, "");

			// store correct index
			rhiBindGroupEntryDesc.binding = binding.index;
		}
	}

	ASSERT_MSG(bindGroupDesc.entries.numElem() >= bindingsToResolve, "Bad binding entry count: %d, expected %d", rhiBindGroupEntryList.numElem(), bindingsToResolve);
	ASSERT_MSG(rhiBindGroupEntryList.numElem() == bindingsToResolve, "Incorrect binding ids, resolved: %d, expected %d", rhiBindGroupEntryList.numElem(), bindingsToResolve);
}

IGPUBindGroupPtr CWGPURenderAPI::CreateSharedBindGroup(const IGPUBindingLayout* layoutDesc, const BindGroupDesc& bindGroupDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	const CWGPUBindingLayout* pipelineLayout = static_cast<const CWGPUBindingLayout*>(layoutDesc);

	const ArrayCRef<WGPUBindGroupLayout> rhiLayout = pipelineLayout->m_rhiBindGroupLayout;
	if (!rhiLayout.inRange(bindGroupDesc.groupIdx))
		return nullptr;

	static thread_local Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	rhiBindGroupEntryList.clear();
	rhiBindGroupEntryList.reserve(bindGroupDesc.entries.numElem());

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

	static thread_local Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	rhiBindGroupEntryList.clear();
	rhiBindGroupEntryList.reserve(bindGroupDesc.entries.numElem());

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

	const CWGPURenderPipeline* pipelineImpl = static_cast<const CWGPURenderPipeline*>(renderPipeline);

	const int moduleIds[] = {
		pipelineImpl->m_vertexShaderModuleIdx,
		pipelineImpl->m_fragmentShaderModuleIdx
	};
	FillWGPUBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiBindGroupEntryList, *pipelineImpl->m_shaderInfo, moduleIds);

	rhiBindGroupDesc.label = _WSTR(bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr);
	rhiBindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(pipelineImpl->m_rhiRenderPipeline, bindGroupDesc.groupIdx);
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

	static thread_local Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);
	rhiBindGroupEntryList.clear();
	rhiBindGroupEntryList.reserve(bindGroupDesc.entries.numElem());

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

	const CWGPUComputePipeline* pipelineImpl = static_cast<const CWGPUComputePipeline*>(computePipeline);
	FillWGPUBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiBindGroupEntryList, *pipelineImpl->m_shaderInfo, ArrayCRef(&pipelineImpl->m_computeShaderModuleIdx, 1));

	rhiBindGroupDesc.label = _WSTR(bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr);
	rhiBindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(pipelineImpl->m_rhiComputePipeline, bindGroupDesc.groupIdx);
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;

	CRefPtr<CWGPUBindGroup> bindGroup = CRefPtr_new(CWGPUBindGroup);
	bindGroup->m_rhiBindGroup = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

WGPUShaderModule CWGPURenderAPI::CreateShaderSPIRV(const uint32* code, uint32 size, const char* dbgName) const
{
	PROF_EVENT_F();

	WGPUDeviceErrorContext crashCtxDumper;

	WGPUDawnShaderModuleSPIRVOptionsDescriptor rhiDawnShaderModuleDesc = {};
	rhiDawnShaderModuleDesc.chain.sType = WGPUSType_DawnShaderModuleSPIRVOptionsDescriptor;
	rhiDawnShaderModuleDesc.allowNonUniformDerivatives = true;

	WGPUShaderSourceSPIRV rhiSpirvDesc = {};
	rhiSpirvDesc.chain.sType = WGPUSType_ShaderSourceSPIRV;
	rhiSpirvDesc.chain.next = &rhiDawnShaderModuleDesc.chain;
	rhiSpirvDesc.codeSize = size / sizeof(uint32_t);
	rhiSpirvDesc.code = code;

	WGPUShaderModuleDescriptor rhiShaderModuleDesc = {};
	rhiShaderModuleDesc.nextInChain = &rhiSpirvDesc.chain;
	rhiShaderModuleDesc.label = _WSTR(dbgName);

	WGPUShaderModule shaderModule = shaderModule = wgpuDeviceCreateShaderModule(m_rhiDevice, &rhiShaderModuleDesc);
	if (crashCtxDumper.hasError)
	{
		wgpuShaderModuleRelease(shaderModule);
		ASSERT_FAIL("Failed to create SPIRV source shader module %s", dbgName);
		return nullptr;
	}

	return shaderModule;
}

WGPUShaderModule CWGPURenderAPI::CreateShaderWGSL(const char* szText, const char* dbgName) const
{
	PROF_EVENT_F();

	WGPUDeviceErrorContext crashCtxDumper;

	WGPUShaderSourceWGSL rhiWgslDesc = {};
	rhiWgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
	rhiWgslDesc.code = _WSTR(szText);

	WGPUShaderModuleDescriptor rhiShaderModuleDesc = {};
	rhiShaderModuleDesc.nextInChain = &rhiWgslDesc.chain;
	rhiShaderModuleDesc.label = _WSTR(dbgName);

	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(m_rhiDevice, &rhiShaderModuleDesc);
	if(crashCtxDumper.hasError)
	{
		wgpuShaderModuleRelease(shaderModule);
		ASSERT_FAIL("Failed to create WGSL source shader module %s", dbgName);
		return nullptr;
	}
	return shaderModule;
}

WGPUShaderModule CWGPURenderAPI::GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx, const char* dbgName) const
{
	ShaderInfo::Module& mod = const_cast<ShaderInfo::Module&>(shaderInfo.modules[shaderModuleIdx]);
	if (mod.rhiModule)
		return reinterpret_cast<WGPUShaderModule>(mod.rhiModule);

	BitArray& usedBindings = mod.usedBindings;
	usedBindings.resize(shaderInfo.GetBindingIds(mod).numElem());

	CMemoryStream shaderBlobData(PP_SL);
	auto loadShaderBlob = [&](EShaderModuleType type)
	{
		IFileStreamPtr shaderFile = shaderInfo.shaderPackFile->Open(mod.fileIndex[type], FS_OPEN_READ);
		if (!shaderFile)
			return;

		shaderBlobData.Close();
		shaderBlobData.Open(FS_OPEN_WRITE | FS_OPEN_READ);

		int blobSize;
		shaderFile->ReadObj(blobSize);
		shaderBlobData.AppendStream(shaderFile, blobSize);

		shaderFile->Seek(blobSize, FS_SEEK_CUR);
		shaderFile->ReadArray(usedBindings.ptr(), bitArray2Dword(usedBindings.numBits()) );
	};

	const EqString shaderModuleName = EqString::Format("%s-%d", shaderInfo.shaderName.ToCString(), shaderModuleIdx);

	const bool forceUseSpirV = wgpu_forceUseSPIRV.GetBool() && mod.fileIndex[SHADERMODULE_SPIRV] != -1 || mod.fileIndex[SHADERMODULE_WGSL] == -1;

	WGPUShaderModule rhiShaderModule = nullptr;
	if (forceUseSpirV && mod.fileIndex[SHADERMODULE_SPIRV] != -1)
	{
		loadShaderBlob(SHADERMODULE_SPIRV);
		if(!shaderBlobData.IsValid())
		{
			ASSERT_FAIL("Shader module %s (found in package %s) not found for specific backend", shaderModuleName.ToCString(), shaderInfo.shaderName.ToCString());
			return nullptr;
		}

		rhiShaderModule = CreateShaderSPIRV(reinterpret_cast<uint32*>(shaderBlobData.GetBasePointer()), shaderBlobData.GetSize(), dbgName ? dbgName : shaderModuleName.ToCString());
	}

	if (!rhiShaderModule && mod.fileIndex[SHADERMODULE_WGSL] != -1)
	{
		loadShaderBlob(SHADERMODULE_WGSL);
		if (!shaderBlobData.IsValid())
		{
			ASSERT_FAIL("Shader module %s (found in package %s) not found for specific backend", shaderModuleName.ToCString(), shaderInfo.shaderName.ToCString());
			return nullptr;
		}

		const int _zero = 0;
		shaderBlobData.Write(&_zero, 1, sizeof(_zero));
		rhiShaderModule = CreateShaderWGSL(reinterpret_cast<char*>(shaderBlobData.GetBasePointer()), dbgName ? dbgName : shaderModuleName.ToCString());
	}
	
	if (!rhiShaderModule)
	{
		MsgError("Can't create shader module %s!\n", shaderModuleName.ToCString());
		return nullptr;
	}
	mod.rhiModule = rhiShaderModule;

	return rhiShaderModule;
}

void CWGPURenderAPI::LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines, const char* entryPointName) const
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

		if(shaderInfo.shaderKinds & SHADERKIND_FRAGMENT)
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

IGPURenderPipelinePtr CWGPURenderAPI::CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUBindingLayout* pipelineLayout) const
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
		rhiRenderPipelineDesc.layout = static_cast<const CWGPUBindingLayout*>(pipelineLayout)->m_rhiPipelineLayout;
	}

	// Setup vertex pipeline
	// Required
	Array<WGPUVertexAttribute> rhiVertexAttribList(PP_SL);
	Array<WGPUVertexBufferLayout> rhiVertexBufferLayoutList(PP_SL);
	int vertexShaderModuleIdx = -1;
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");
		WGPUShaderModule rhiVertexShaderModule = nullptr;
		{
			const int entryPointStrHash = StringId24(pipelineDesc.vertex.shaderEntryPoint);
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX, entryPointStrHash);

			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
			{
				vertexShaderModuleIdx = *itShaderModuleId;

				ASSERT_MSG(shaderInfo.modules[vertexShaderModuleIdx].kind == SHADERKIND_VERTEX, 
					"Incorrect shader kind for %s %s in shader package %s", 
					shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), 
					shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(),
					pipelineDesc.shaderName.ToCString());

				rhiVertexShaderModule = GetOrLoadShaderModule(shaderInfo, vertexShaderModuleIdx);
			}
		}

		for (const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			const int firstVertexAttrib = rhiVertexAttribList.numElem();
			for (const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				if (attrib.format == ATTRIBUTEFORMAT_NONE)
					continue;

				WGPUVertexAttribute vertAttr = {};
				vertAttr.format = g_wgpuVertexFormats[attrib.format][attrib.count - 1];
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

		WGPUVertexState& rhiVertexState = rhiRenderPipelineDesc.vertex;
		rhiVertexState.module = rhiVertexShaderModule;
		rhiVertexState.entryPoint = _WSTR(pipelineDesc.vertex.shaderEntryPoint);
		rhiVertexState.bufferCount = rhiVertexBufferLayoutList.numElem();
		rhiVertexState.buffers = rhiVertexBufferLayoutList.ptr();
		rhiVertexState.constants = rhiVertexPipelineConstants.ptr();
		rhiVertexState.constantCount = rhiVertexPipelineConstants.numElem();

		if (!rhiVertexState.module)
		{
			EqStringRef queryStr = shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery);
			ASSERT_FAIL("Missing VS module %s:%s (%s) in '%s'", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), pipelineDesc.vertex.shaderEntryPoint.ToCString(), queryStr.ToCString(), shaderInfo.shaderName.ToCString());
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
	int fragmentShaderModuleIdx = -1;
	if(pipelineDesc.fragment.shaderEntryPoint.Length())
	{
		WGPUShaderModule rhiFragmentShaderModule = nullptr;
		{
			const int entryPointStrHash = StringId24(pipelineDesc.fragment.shaderEntryPoint);
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT, entryPointStrHash);

			auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);
			if (!itShaderModuleId.atEnd())
			{
				fragmentShaderModuleIdx = *itShaderModuleId;

				ASSERT_MSG(shaderInfo.modules[fragmentShaderModuleIdx].kind == SHADERKIND_FRAGMENT,
					"Incorrect shader kind for %s %s in shader package %s",
					shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), 
					shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(),
					pipelineDesc.shaderName.ToCString());

				rhiFragmentShaderModule = GetOrLoadShaderModule(shaderInfo, fragmentShaderModuleIdx);
			}
		}

		for (const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
		{
			WGPUColorTargetState rhiColorTarget = {};

			if (target.blendEnable)
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

		rhiFragmentState.module = rhiFragmentShaderModule;
		rhiFragmentState.entryPoint = _WSTR(pipelineDesc.fragment.shaderEntryPoint);
		rhiFragmentState.targetCount = rhiColorTargets.numElem();
		rhiFragmentState.targets = rhiColorTargets.ptr();
		rhiFragmentState.constants = rhiFragmentPipelineConstants.ptr();
		rhiFragmentState.constantCount = rhiFragmentPipelineConstants.numElem();

		if(!rhiFragmentState.module)
		{
			EqStringRef queryStr = shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery);
			ASSERT_FAIL("Missing PS module %s:%s (%s) in '%s'", shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), pipelineDesc.fragment.shaderEntryPoint.ToCString(), queryStr.ToCString(), shaderInfo.shaderName.ToCString());
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

	EqString pipelineName = EqString::Format("%s-%s:%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString(), shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString());
	rhiRenderPipelineDesc.label = _WSTR(pipelineName);

	{
		WGPUDeviceErrorContext crashCtxDumper([&]() {
			// dump shaders
			Msg("CreateRenderPipeline for %s failure\n", pipelineName.ToCString());
			Msg("Dumping WGSL shader for VERTEX\n");
			// dump vertex shader
			{
				const int entryPointStrHash = StringId24(pipelineDesc.vertex.shaderEntryPoint);
				const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX, entryPointStrHash);
				auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

				if (!itShaderModuleId.atEnd())
				{
					const int fileIdx = shaderInfo.modules[*itShaderModuleId].fileIndex[SHADERMODULE_WGSL];
					IFileStreamPtr shaderFile = shaderInfo.shaderPackFile->Open(fileIdx, FS_OPEN_READ);
					if (shaderFile)
					{
						CMemoryStream shaderBlobData(PP_SL);
						shaderBlobData.Open(FS_OPEN_WRITE | FS_OPEN_READ);
						shaderBlobData.AppendStream(shaderFile);
						int _zero = 0;
						shaderBlobData.WriteObj(&_zero);

						Msg("%s", shaderBlobData.GetBasePointer());
					}
				}
			}

			Msg("\n\nDumping WGSL shader for FRAGMENT\n");
			// dump fragment shader
			{
				const int entryPointStrHash = StringId24(pipelineDesc.fragment.shaderEntryPoint);
				const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT, entryPointStrHash);
				auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

				if (!itShaderModuleId.atEnd())
				{
					const int fileIdx = shaderInfo.modules[*itShaderModuleId].fileIndex[SHADERMODULE_WGSL];
					IFileStreamPtr shaderFile = shaderInfo.shaderPackFile->Open(fileIdx, FS_OPEN_READ);
					if (shaderFile)
					{
						CMemoryStream shaderBlobData(PP_SL);
						shaderBlobData.Open(FS_OPEN_WRITE | FS_OPEN_READ);
						shaderBlobData.AppendStream(shaderFile);
						int _zero = 0;
						shaderBlobData.WriteObj(&_zero);

						Msg("%s", shaderBlobData.GetBasePointer());
					}
				}
			}
			});

		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		WGPURenderPipeline rhiRenderPipeline = wgpuDeviceCreateRenderPipeline(m_rhiDevice, &rhiRenderPipelineDesc);
		if (crashCtxDumper.hasError)
		{
			wgpuRenderPipelineRelease(rhiRenderPipeline);
			ASSERT_FAIL("Render pipeline %s creation failed", pipelineName.ToCString());
			return nullptr;
		}

		CRefPtr<CWGPURenderPipeline> renderPipeline = CRefPtr_new(CWGPURenderPipeline);
		renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;
		renderPipeline->m_shaderInfo = &shaderInfo;
		renderPipeline->m_vertexShaderModuleIdx = vertexShaderModuleIdx;
		renderPipeline->m_fragmentShaderModuleIdx = fragmentShaderModuleIdx;
		renderPipeline->m_pipelineId = m_pipelineIdCounter++;

		return IGPURenderPipelinePtr(renderPipeline);
	}
}

IGPUCommandRecorderPtr CWGPURenderAPI::CreateCommandRecorder(const char* name, void* userData) const
{
	WGPUCommandEncoderDescriptor rhiEncoderDesc = {};
	rhiEncoderDesc.label = _WSTR(name);
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
			renderTargetDims = colorTarget.target.texture->GetSize().xy();
			renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target.texture->GetFormat() : FORMAT_NONE;

			if (colorTarget.target)
				renderPass->m_renderTargetMSAASamples = colorTarget.target.texture->GetSampleCount();
		}
	}

	if (renderPassDesc.depthStencil)
	{
		renderTargetDims = renderPassDesc.depthStencil.texture->GetSize().xy();
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

IGPUComputePipelinePtr CWGPURenderAPI::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc, const IGPUBindingLayout* pipelineLayout) const
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

	WGPUShaderModule rhiComputeShaderModule = nullptr;
	int computeShaderModuleIdx = -1;
	{
		const int entryPointStrHash = StringId24(pipelineDesc.shaderEntryPoint);
		const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, layoutIdx, SHADERKIND_COMPUTE, entryPointStrHash);
		auto itShaderModuleId = shaderInfo.modulesMap.find(shaderModuleId);

		if (!itShaderModuleId.atEnd())
		{
			computeShaderModuleIdx = *itShaderModuleId;

			ASSERT_MSG(shaderInfo.modules[*itShaderModuleId].kind == SHADERKIND_COMPUTE, 
				"Incorrect shader kind for %s %s in shader package %s", 
				shaderInfo.vertexLayouts[layoutIdx].name.ToCString(), 
				shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString(), 
				pipelineDesc.shaderName.ToCString());

			rhiComputeShaderModule = GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}

		if(!rhiComputeShaderModule)
		{
			EqStringRef queryStr = shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery);
			ASSERT_FAIL("Missing CS module %s:%s (%s) in '%s'", shaderInfo.vertexLayouts[layoutIdx].name.ToCString(), pipelineDesc.shaderEntryPoint.ToCString(), queryStr.ToCString(), shaderInfo.shaderName.ToCString());
			return nullptr;
		}
	}

	Array<WGPUConstantEntry> rhiComputePipelineConstants(PP_SL);

	for (const PipelineConst& constant : pipelineDesc.constants)
		rhiComputePipelineConstants.append({ nullptr, _WSTR(constant.name), constant.value});

	WGPUComputePipelineDescriptor rhiComputePipelineDesc = {};
	rhiComputePipelineDesc.compute.constantCount = rhiComputePipelineConstants.numElem();
	rhiComputePipelineDesc.compute.constants = rhiComputePipelineConstants.ptr();
	rhiComputePipelineDesc.compute.entryPoint = _WSTR(pipelineDesc.shaderEntryPoint);
	rhiComputePipelineDesc.compute.module = rhiComputeShaderModule;

	if (pipelineLayout)
		rhiComputePipelineDesc.layout = static_cast<const CWGPUBindingLayout*>(pipelineLayout)->m_rhiPipelineLayout;

	EqString pipelineName = EqString::Format("%s-%s:%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[layoutIdx].name.ToCString(), shaderInfo.GetShaderQueryStr(pipelineDesc.shaderQuery).ToCString());
	rhiComputePipelineDesc.label = _WSTR(pipelineName);

	{
		WGPUDeviceErrorContext crashCtxDumper;

		PROF_EVENT(EqString::Format("CreateComputePipeline for %s", pipelineName.ToCString()));
		WGPUComputePipeline rhiComputePipeline = wgpuDeviceCreateComputePipeline(m_rhiDevice, &rhiComputePipelineDesc);
		if (!rhiComputePipeline)
		{
			wgpuComputePipelineRelease(rhiComputePipeline);
			ASSERT_FAIL("Compute pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CWGPUComputePipeline> computePipeline = CRefPtr_new(CWGPUComputePipeline);
		computePipeline->m_rhiComputePipeline = rhiComputePipeline;
		computePipeline->m_shaderInfo = &shaderInfo;
		computePipeline->m_computeShaderModuleIdx = computeShaderModuleIdx;
		computePipeline->m_pipelineId = m_pipelineIdCounter++;

		return IGPUComputePipelinePtr(computePipeline);
	}
}

IGPUComputePassRecorderPtr CWGPURenderAPI::BeginComputePass(const char* name, void* userData) const
{
	WGPUCommandEncoder rhiCommandEncoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
	if (!rhiCommandEncoder)
		return nullptr;

	WGPUComputePassDescriptor rhiComputePassDesc = {};
	rhiComputePassDesc.label = _WSTR(name);
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
		static Array<WGPUCommandBuffer> rhiSubmitBuffers(PP_SL);
		rhiSubmitBuffers.clear();
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
#if 0
		WGPUQueueWorkDoneCallbackInfo rhiCbInfo{};
		rhiCbInfo.mode = WGPUCallbackMode_WaitAnyOnly;
		WGPUFuture rhiFuture = wgpuQueueOnSubmittedWorkDone(m_rhiQueue, rhiCbInfo);

		WGPUFutureWaitInfo rhiWaitInfo{};
		rhiWaitInfo.future = rhiFuture;
		wgpuInstanceWaitAny(m_rhiInstance, 1, &rhiWaitInfo, UINT64_MAX);
#endif
		return 0;
	});
}

Future<bool> CWGPURenderAPI::SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const
{
	static thread_local Array<WGPUCommandBuffer> rhiSubmitBuffers(PP_SL);
	rhiSubmitBuffers.clear();
	rhiSubmitBuffers.reserve(cmdBuffers.numElem());
	for (IGPUCommandBuffer* cmdBuffer : cmdBuffers)
	{
		if (!cmdBuffer)
			continue;
		const CWGPUCommandBuffer* bufferImpl = static_cast<const CWGPUCommandBuffer*>(cmdBuffer);
		WGPUCommandBuffer rhiCmdBuffer = bufferImpl->m_rhiCommandBuffer;
		ASSERT(rhiCmdBuffer);

		rhiSubmitBuffers.append(rhiCmdBuffer);
		wgpuCommandBufferAddRef(rhiCmdBuffer);
	}

	if (!rhiSubmitBuffers.numElem())
	{
		return Future<bool>::Succeed(true);
	}

	Promise<bool> promise;
	g_renderWorker.Execute(__func__, [this, submitBuffers = std::move(rhiSubmitBuffers), promiseData = promise.GrabDataPtr()]() {
		wgpuQueueSubmit(m_rhiQueue, submitBuffers.numElem(), submitBuffers.ptr());
		WGPUQueueWorkDoneCallbackInfo rhiCbInfo{};
		rhiCbInfo.callback = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
			Promise<bool> promise(reinterpret_cast<Promise<bool>::Data*>(userdata1));

			if(status != WGPUQueueWorkDoneStatus_Success)
			{
				const char* str = "Invalid";
				switch (status)
				{
				case WGPUQueueWorkDoneStatus_Error:
					str = "Error";
					break;
				case WGPUQueueWorkDoneStatus_CallbackCancelled:
					str = "CallbackCancelled";
					break;
				}
				promise.SetError(-1, str);
			}
			else
			{
				promise.SetResult(true);
			}
		};

		rhiCbInfo.userdata1 = promiseData;
		rhiCbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
		wgpuQueueOnSubmittedWorkDone(m_rhiQueue, rhiCbInfo);

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
	rhiQuerySetDesc.label = _WSTR("querySet");
	rhiQuerySetDesc.type = WGPUQueryType_Occlusion;
	rhiQuerySetDesc.count = 32;
	WGPUQuerySet rhiQuerySet = wgpuDeviceCreateQuerySet(nullptr, &rhiQuerySetDesc);
}
