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

#pragma optimize("", off)

constexpr EqStringRef s_shaderKindVertexName = "Vertex";
constexpr EqStringRef s_shaderKindFragmentName = "Fragment";
constexpr EqStringRef s_shaderKindComputeName = "Compute";
constexpr EqStringRef s_DefaultVertexLayoutName = "Default";

DECLARE_CVAR(nvrhi_preloadShaders, "0", "Preload all shaders during startup. This affects engine startup time but allows name display.", CV_ARCHIVE);

CNVRHIRenderAPI CNVRHIRenderAPI::Instance;
ShaderAPI_Base& ShaderAPI_Base::Instance = CNVRHIRenderAPI::Instance;
IShaderAPI* g_renderAPI = &CNVRHIRenderAPI::Instance;

//------------------------------------------

void CNVRHIRenderAPI::Shutdown()
{
	ShaderAPI_Base::Shutdown();
	m_shaderCache.clear(true);
	m_rhiDevice = nullptr;
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

IVertexFormatPtr CNVRHIRenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	IVertexFormatPtr pVF = IVertexFormatPtr(CRefPtr_new(CNVRHIVertexFormat, name, formatDesc));
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

nvrhi::BindingLayoutHandle CNVRHIRenderAPI::CreateBindingLayout(const BindGroupLayoutDesc& bindGroupDesc, int bindGroupIndex) const
{
	auto rhiBindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setRegisterSpace(bindGroupIndex)
		.setRegisterSpaceIsDescriptorSet(true);

	int constantCount = 0;
	int uavCount = 0;
	int srvCount = 0;

	int rhiShaderTypeVisbility = 0;
	for (const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
	{
		if (entry.visibility & SHADERKIND_VERTEX)	rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Vertex);
		if (entry.visibility & SHADERKIND_FRAGMENT) rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Pixel);
		if (entry.visibility & SHADERKIND_COMPUTE)	rhiShaderTypeVisbility |= static_cast<int>(nvrhi::ShaderType::Compute);

		switch (entry.type)
		{
		case BINDENTRY_BUFFER:
			switch (entry.buffer.bindType)
			{
			case BUFFERBIND_UNIFORM:
				rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(entry.binding));
				break;
			case BUFFERBIND_STORAGE_READONLY:
				// I'm not sure if it should be TypedBuffer, StructuredBuffer or RawBuffer
				rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(srvCount++));
				break;
			case BUFFERBIND_STORAGE:
				rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(uavCount++));
				break;
			}
			break;
		case BINDENTRY_SAMPLER:
			rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(entry.binding));
			break;
		case BINDENTRY_TEXTURE:
			rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(srvCount++));
			break;
		case BINDENTRY_STORAGETEXTURE:
			// all storage images are supposed to be UAV
			if(entry.storageTexture.access == STORAGETEX_READONLY)
				rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(srvCount++));
			else
				rhiBindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(uavCount++));
			break;
		}
	}
	rhiBindingLayoutDesc.setVisibility(static_cast<nvrhi::ShaderType>(rhiShaderTypeVisbility));

	return m_rhiDevice->createBindingLayout(rhiBindingLayoutDesc);
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
		nvrhi::BindingLayoutHandle rhiBindingLayout = CreateBindingLayout(bindGroupDesc, bindGroupIndex);
		if (!rhiBindingLayout)
		{
			ASSERT_FAIL("Failed to create pipeline layout for bind group %d", bindGroupIndex);
			return nullptr;
		}

		pipelineLayout->m_rhiBindingLayout[pipelineLayout] = rhiBindingLayout;
		++bindGroupIndex;
	}

	return IGPUPipelineLayoutPtr(pipelineLayout);
}

static void FillNVRHIBindGroupEntries(nvrhi::IDevice* rhiDevice, const BindGroupDesc& bindGroupDesc, Array<nvrhi::SamplerHandle>& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc)
{
	for (const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		//const bool isSRV = (entry.visibility & (SHADERKIND_VERTEX | SHADERKIND_FRAGMENT));
		//const bool isUAV = (entry.visibility & (SHADERKIND_COMPUTE));

		switch (bindGroupEntry.type)
		{
		case BINDENTRY_BUFFER:
		{
			CNVRHIBuffer* buffer = static_cast<CNVRHIBuffer*>(bindGroupEntry.buffer.buffer.Ptr());
			if (buffer)
			{
				const uint64 bufferSize = bindGroupEntry.buffer.size < 0 ? nvrhi::EntireBuffer.byteSize : bindGroupEntry.buffer.size;
				rhiBindingSetDesc.addItem(nvrhi::BindingSetItem()
					.RawBuffer_SRV(bindGroupEntry.binding, buffer->GetNVRHIBufferHandle(), bindGroupEntry.buffer.size < 0 ? nvrhi::EntireBuffer : nvrhi::BufferRange(bindGroupEntry.buffer.offset, bufferSize))
				);
			}
			else
				ASSERT_FAIL("NULL buffer for bindGroup %d binding %d", bindGroupDesc.groupIdx, bindGroupEntry.binding);

			break;
		}
		case BINDENTRY_SAMPLER:
		{
			auto rhiSamplerDesc = nvrhi::SamplerDesc();
			FillNVRHISamplerDescriptor(bindGroupEntry.sampler, rhiSamplerDesc);

			nvrhi::SamplerHandle rhiSampler = rhiDevice->createSampler(rhiSamplerDesc);
			ASSERT(bindGroupEntry.sampler.maxAnisotropy > 0);

			rhiBindingSetDesc.addItem(nvrhi::BindingSetItem()
				.Sampler(bindGroupEntry.binding, rhiSampler)
			);
			rhiSamplers.append(rhiSampler);
			break;
		}
		case BINDENTRY_STORAGETEXTURE:
		case BINDENTRY_TEXTURE:
			CNVRHITexture* texture = static_cast<CNVRHITexture*>(bindGroupEntry.texture.texture.Ptr());

			// NOTE: animated textures aren't that supported, so it would need array lookup through the shader
			if (texture)
			{
				ASSERT_MSG(texture->GetNVRHITextureViewCount(), "Texture '%s' has no views", texture->GetName());
				rhiBindingSetDesc.addItem(nvrhi::BindingSetItem()
					.Texture_SRV(bindGroupEntry.binding, texture->GetNVRHITextureHandle(), nvrhi::Format::UNKNOWN, texture->GetNVRHITextureView(bindGroupEntry.texture.arraySlice))
				);
			}
			else
				ASSERT_FAIL("NULL texture for bindGroup %d binding %d", bindGroupDesc.groupIdx, bindGroupEntry.binding);
			break;
		}
	}
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroupImpl(const NVRHIBindingLayoutList& rhiBindingLayouts, const BindGroupDesc& bindGroupDesc) const
{
	if (!rhiBindingLayouts.inRange(bindGroupDesc.groupIdx))
	{
		ASSERT_FAIL("invalid binding group index %d", bindGroupDesc.groupIdx);
		return nullptr;
	}

	Array<nvrhi::SamplerHandle> rhiSamplers(PP_SL);

	auto rhiBindingSetDesc = nvrhi::BindingSetDesc();
	FillNVRHIBindGroupEntries(m_rhiDevice, bindGroupDesc, rhiSamplers, rhiBindingSetDesc);

	nvrhi::BindingSetHandle rhiBindSet = m_rhiDevice->createBindingSet(rhiBindingSetDesc, rhiBindingLayouts[bindGroupDesc.groupIdx]);
	if (!rhiBindSet)
		return nullptr;

	CRefPtr<CNVRHIBindGroup> bindGroup = CRefPtr_new(CNVRHIBindGroup);
	bindGroup->m_rhiBindingSet = rhiBindSet;
	bindGroup->m_dbgName = bindGroupDesc.name;

	return IGPUBindGroupPtr(bindGroup);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPUPipelineLayout* layoutDesc, const BindGroupDesc& bindGroupDesc) const
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	const CNVRHIPipelineLayout* pipelineLayoutImpl = static_cast<const CNVRHIPipelineLayout*>(layoutDesc);
	return CreateBindGroupImpl(pipelineLayoutImpl->m_rhiBindingLayout, bindGroupDesc);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!renderPipeline)
	{
		ASSERT_FAIL("computePipeline is null");
		return nullptr;
	}

	const CNVRHIRenderPipeline* renderPipelineImpl = static_cast<const CNVRHIRenderPipeline*>(renderPipeline);
	return CreateBindGroupImpl(renderPipelineImpl->m_rhiBindingLayout, bindGroupDesc);
}

IGPUBindGroupPtr CNVRHIRenderAPI::CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const
{
	if (!computePipeline)
	{
		ASSERT_FAIL("computePipeline is null");
		return nullptr;
	}

	const CNVRHIComputePipeline* computePipelineImpl = static_cast<const CNVRHIComputePipeline*>(computePipeline);
	return CreateBindGroupImpl(computePipelineImpl->m_rhiBindingLayout, bindGroupDesc);
}

const ShaderInfo::Module& CNVRHIRenderAPI::GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx, const char* dbgName) const
{
	ShaderInfo::Module& mod = const_cast<ShaderInfo::Module&>(shaderInfo.modules[shaderModuleIdx]);
	if (mod.rhiModule)
		return mod;

	CMemoryStream shaderBlobData(PP_SL);
	auto loadShaderBlob = [&](EShaderModuleType type)
	{
		IFileStreamPtr shaderFile = shaderInfo.shaderPackFile->Open(mod.fileIndex[type], FS_OPEN_READ);
		if (!shaderFile)
			return nullptr;

		shaderBlobData.Open(FS_OPEN_WRITE | FS_OPEN_READ);
		shaderBlobData.AppendStream(shaderFile);
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

	if (m_backendType == NVRHI_BACKEND_D3D11)
		loadShaderBlob(SHADERMODULE_DXBC);
	else if (m_backendType == NVRHI_BACKEND_D3D12)
		loadShaderBlob(SHADERMODULE_DXIL);
	else if (m_backendType == NVRHI_BACKEND_VULKAN)
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

static void ShaderBindingsToPipelineLayout(PipelineLayoutDesc& layoutDesc, ArrayCRef<ShaderInfo::Binding> bindings, int visibility)
{
	for (const ShaderInfo::Binding& binding : bindings)
	{
		BindGroupLayoutDesc& bindGroupDesc = layoutDesc.bindGroups[binding.descriptorSetIdx];
		BindGroupLayoutDesc::Entry& entry = bindGroupDesc.entries.append();
		entry.name = binding.name;
		entry.binding = binding.index;
		entry.type = binding.type;
		entry.visibility = visibility;
		switch (entry.type)
		{
		case BINDENTRY_BUFFER:
			if (binding.rwFlags & RWFLAG_UNIFORM)
				entry.buffer.bindType = BUFFERBIND_UNIFORM;
			else if (binding.rwFlags & RWFLAG_WRITE)
				entry.buffer.bindType = BUFFERBIND_STORAGE;
			else
				entry.buffer.bindType = BUFFERBIND_STORAGE_READONLY;
			break;
		case BINDENTRY_STORAGETEXTURE:
			if ((binding.rwFlags & (RWFLAG_WRITE | RWFLAG_WRITE)) == (RWFLAG_WRITE | RWFLAG_WRITE))
				entry.storageTexture.access = STORAGETEX_READWRITE;
			else if (binding.rwFlags & RWFLAG_READ)
				entry.storageTexture.access = STORAGETEX_READONLY;
			else if (binding.rwFlags & RWFLAG_WRITE)
				entry.storageTexture.access = STORAGETEX_WRITEONLY;
			break;
		}
	}
}

IGPURenderPipelinePtr CNVRHIRenderAPI::CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout) const
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
	const ShaderInfo::Module* vertexShaderModule = nullptr;
	const ShaderInfo::Module* fragmentShaderModule = nullptr;
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");

		Array<nvrhi::VertexAttributeDesc> rhiVertexAttribList(PP_SL);
		for(const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			for(const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				if (attrib.format == ATTRIBUTEFORMAT_NONE)
					continue;

				auto rhiVertAttr = rhiVertexAttribList.append()
					.setFormat(g_nvrhiVertexFormats[attrib.format][attrib.count - 1])
					.setOffset(attrib.offset)
					.setBufferIndex(attrib.location)
					.setIsInstanced(vertexLayout.stepMode == VERTEX_STEPMODE_INSTANCE)
					.setElementStride(vertexLayout.stride);
				rhiVertAttr.name = attrib.name.ToCString();
			}
		}

		{
			const int entryPointStrHash = StringId24(pipelineDesc.vertex.shaderEntryPoint);
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_VERTEX, entryPointStrHash);
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
				vertexShaderModule = &GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
			}
		}

		if (!vertexShaderModule || !vertexShaderModule->rhiModule)
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

		rhiInputLayout = m_rhiDevice->createInputLayout(rhiVertexAttribList.ptr(), rhiVertexAttribList.numElem(), reinterpret_cast<nvrhi::IShader*>(vertexShaderModule->rhiModule));
		rhiGraphicsPipelineDesc.setVertexShader(reinterpret_cast<nvrhi::IShader*>(vertexShaderModule->rhiModule));
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

	// Setup fragment pipeline
	// Fragment state
	// When opted out, requires rhiDepthStencil state
	if(pipelineDesc.fragment.shaderEntryPoint.Length())
	{
		auto& rhiBlendState = rhiGraphicsPipelineDesc.renderState.blendState;
		int targetNum = 0;
		for(const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
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

		{
			const int entryPointStrHash = StringId24(pipelineDesc.fragment.shaderEntryPoint);
			const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertexLayoutIdx, SHADERKIND_FRAGMENT, entryPointStrHash);
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
				fragmentShaderModule = &GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
			}
		}

		if(!fragmentShaderModule || !fragmentShaderModule->rhiModule)
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
		rhiGraphicsPipelineDesc.setPixelShader(reinterpret_cast<nvrhi::IShader*>(fragmentShaderModule->rhiModule));
	}

	const CNVRHIPipelineLayout* pipelineLayoutImpl = static_cast<const CNVRHIPipelineLayout*>(pipelineLayout);
	if (pipelineLayoutImpl)
	{
		for (nvrhi::BindingLayoutHandle& rhiLayout : pipelineLayoutImpl->m_rhiBindingLayout)
			rhiGraphicsPipelineDesc.addBindingLayout(rhiLayout);
	}
	else
	{
		// create shader pipeline layout
		int maxBindGroups = 0;
		for (const ShaderInfo::Binding& binding : vertexShaderModule->bindings)
			maxBindGroups = max(binding.descriptorSetIdx, maxBindGroups);

		ArrayCRef<ShaderInfo::Binding> fragmentBindings = fragmentShaderModule ? fragmentShaderModule->bindings : ArrayCRef<ShaderInfo::Binding>(nullptr);
		for (const ShaderInfo::Binding& binding : fragmentBindings)
			maxBindGroups = max(binding.descriptorSetIdx, maxBindGroups);

		if (maxBindGroups)
		{
			PipelineLayoutDesc shaderPipelineLayoutDesc;
			shaderPipelineLayoutDesc.bindGroups.setNum(maxBindGroups);
			ShaderBindingsToPipelineLayout(shaderPipelineLayoutDesc, vertexShaderModule->bindings, SHADERKIND_VERTEX);

			for (const ShaderInfo::Binding& binding : fragmentBindings)
			{
				BindGroupLayoutDesc& bindGroupDesc = shaderPipelineLayoutDesc.bindGroups[binding.descriptorSetIdx];
				const int existingIdx = arrayFindIndexF(bindGroupDesc.entries, [&](const BindGroupLayoutDesc::Entry& entry) {
					return entry.name == binding.name;
				});
				if (existingIdx != -1)
				{
					// TODO: also validate
					bindGroupDesc.entries[existingIdx].visibility |= SHADERKIND_FRAGMENT;
				}
			}

			int bindGroupIdx = 0;
			for (BindGroupLayoutDesc& bindGroupDesc : shaderPipelineLayoutDesc.bindGroups)
			{
				rhiGraphicsPipelineDesc.addBindingLayout(CreateBindingLayout(bindGroupDesc, bindGroupIdx));
				++bindGroupIdx;
			}
		}
	}

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

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[vertexLayoutIdx].name.ToCString());
	
#if 0
	{
		PROF_EVENT(EqString::Format("CreateRenderPipeline for %s", pipelineName.ToCString()));
		nvrhi::GraphicsPipelineHandle rhiRenderPipeline = m_rhiDevice->createGraphicsPipeline(rhiGraphicsPipelineDesc, rhiFramebufferInfo);
		if (!rhiRenderPipeline)
		{
			ASSERT_FAIL("Render pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CNVRHIRenderPipeline> renderPipeline = CRefPtr_new(CNVRHIRenderPipeline);
		renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;
		renderPipeline->m_dbgName = std::move(pipelineName);
		renderPipeline->m_rhiBindingLayout = pipelineLayoutImpl->m_rhiBindingLayout;

		return IGPURenderPipelinePtr(renderPipeline);
	}
#endif
	ASSERT_FAIL("createGraphicsPipeline must be in CommitGraphicsState");
	return nullptr;
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
	{
		const int entryPointStrHash = StringId24(pipelineDesc.shaderEntryPoint);
		const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, layoutIdx, SHADERKIND_COMPUTE, entryPointStrHash);
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
			computeShaderModule = &GetOrLoadShaderModule(shaderInfo, *itShaderModuleId);
		}
	}

	auto rhiComputePipelineDesc = nvrhi::ComputePipelineDesc()
		.setComputeShader(reinterpret_cast<nvrhi::IShader*>(computeShaderModule->rhiModule));

	const CNVRHIPipelineLayout* pipelineLayoutImpl = static_cast<const CNVRHIPipelineLayout*>(pipelineLayout);
	if (pipelineLayoutImpl)
	{
		for (nvrhi::BindingLayoutHandle& rhiLayout : pipelineLayoutImpl->m_rhiBindingLayout)
			rhiComputePipelineDesc.addBindingLayout(rhiLayout);
	}
	else
	{
		// create shader pipeline layout
		int maxBindGroupIdx = -1;
		for (const ShaderInfo::Binding& binding : computeShaderModule->bindings)
			maxBindGroupIdx = max(binding.descriptorSetIdx, maxBindGroupIdx);

		if (maxBindGroupIdx >= 0)
		{
			PipelineLayoutDesc shaderPipelineLayoutDesc;
			shaderPipelineLayoutDesc.bindGroups.setNum(maxBindGroupIdx + 1);
			ShaderBindingsToPipelineLayout(shaderPipelineLayoutDesc, computeShaderModule->bindings, SHADERKIND_COMPUTE);

			int bindGroupIdx = 0;
			for (BindGroupLayoutDesc& bindGroupDesc : shaderPipelineLayoutDesc.bindGroups)
			{
				rhiComputePipelineDesc.addBindingLayout(CreateBindingLayout(bindGroupDesc, bindGroupIdx));
				++bindGroupIdx;
			}
		}
	}

	EqString pipelineName = EqString::Format("%s-%s", pipelineDesc.shaderName.ToCString(), shaderInfo.vertexLayouts[layoutIdx].name.ToCString());

	{
		PROF_EVENT(EqString::Format("CreateComputePipeline for %s", pipelineName.ToCString()));
		nvrhi::ComputePipelineHandle rhiComputePipeline = m_rhiDevice->createComputePipeline(rhiComputePipelineDesc);
		if (!rhiComputePipeline)
		{
			ASSERT_FAIL("Compute pipeline creation failed");
			return nullptr;
		}

		CRefPtr<CNVRHIComputePipeline> computePipeline = CRefPtr_new(CNVRHIComputePipeline);
		computePipeline->m_rhiComputePipeline = rhiComputePipeline;
		computePipeline->m_dbgName = std::move(pipelineName);
		for(int i = 0; i < rhiComputePipelineDesc.bindingLayouts.size(); ++i)
			computePipeline->m_rhiBindingLayout.append(rhiComputePipelineDesc.bindingLayouts[i]);

		return IGPUComputePipelinePtr(computePipeline);
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
