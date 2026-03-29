#include "core/core_common.h"

#include "WGPUStates.h"
#include "WGPURenderAPI.h"
#include "WGPURenderDefs.h"

static void FillWGPUSamplerDescriptor(const SamplerStateParams& samplerParams, WGPUSamplerDescriptor& rhiSamplerDesc)
{
	rhiSamplerDesc.addressModeU = g_wgpuAddressMode[samplerParams.addressU];
	rhiSamplerDesc.addressModeV = g_wgpuAddressMode[samplerParams.addressV];
	rhiSamplerDesc.addressModeW = g_wgpuAddressMode[samplerParams.addressW];
	rhiSamplerDesc.compare = g_wgpuCompareFunc[samplerParams.compareFunc];
	rhiSamplerDesc.minFilter = g_wgpuFilterMode[samplerParams.minFilter];
	rhiSamplerDesc.magFilter = g_wgpuFilterMode[samplerParams.magFilter];
	rhiSamplerDesc.mipmapFilter = g_wgpuMipmapFilterMode[samplerParams.mipmapFilter];
	rhiSamplerDesc.lodMinClamp = 0.0f;
	rhiSamplerDesc.lodMaxClamp = 8192.0f;

	if (rhiSamplerDesc.minFilter == WGPUFilterMode_Nearest)
		rhiSamplerDesc.maxAnisotropy = 1;
	else
		rhiSamplerDesc.maxAnisotropy = samplerParams.maxAnisotropy;
}

void FillWGPUBlendComponent(const BlendStateParams& blendParams, WGPUBlendComponent& rhiBlendComponent)
{
	rhiBlendComponent.operation = g_wgpuBlendOp[blendParams.blendFunc];
	rhiBlendComponent.srcFactor = g_wgpuBlendFactor[blendParams.srcFactor];
	rhiBlendComponent.dstFactor = g_wgpuBlendFactor[blendParams.dstFactor];
}

void FillWGPURenderPassDescriptor(const RenderPassDesc& renderPassDesc, WGPURenderPassDescriptor& rhiRenderPassDesc, FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS>& rhiColorAttachmentList, WGPURenderPassDepthStencilAttachment& rhiDepthStencilAttachment)
{
	rhiRenderPassDesc.label = _WSTR(renderPassDesc.name.Length() ? renderPassDesc.name.ToCString() : nullptr);
	for (const RenderPassDesc::ColorTargetDesc& colorTarget : renderPassDesc.colorTargets)
	{
		const CWGPUTexture* targetTexture = static_cast<CWGPUTexture*>(colorTarget.target.texture.Ptr());
		const CWGPUTexture* resolveTargetTexture = static_cast<CWGPUTexture*>(colorTarget.resolveTarget.texture.Ptr());

		WGPURenderPassColorAttachment rhiColorAttachment = {};
		rhiColorAttachment.loadOp = g_wgpuLoadOp[colorTarget.loadOp];
		rhiColorAttachment.storeOp = g_wgpuStoreOp[colorTarget.storeOp];
		rhiColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
		rhiColorAttachment.clearValue = WGPUColor{ colorTarget.clearColor.r, colorTarget.clearColor.g, colorTarget.clearColor.b, colorTarget.clearColor.a };

		if (targetTexture)
			rhiColorAttachment.view = targetTexture->GetWGPUTextureView(colorTarget.target.arraySlice);

		if (resolveTargetTexture)
			rhiColorAttachment.resolveTarget = resolveTargetTexture->GetWGPUTextureView(colorTarget.resolveTarget.arraySlice);

		rhiColorAttachmentList.append(rhiColorAttachment);
	}
	rhiRenderPassDesc.colorAttachmentCount = rhiColorAttachmentList.numElem();
	rhiRenderPassDesc.colorAttachments = rhiColorAttachmentList.ptr();

	if (renderPassDesc.depthStencil)
	{
		const CWGPUTexture* depthTexture = static_cast<CWGPUTexture*>(renderPassDesc.depthStencil.texture.Ptr());
		rhiDepthStencilAttachment.view = depthTexture->GetWGPUTextureView(renderPassDesc.depthStencil.arraySlice);

		rhiDepthStencilAttachment.depthReadOnly = renderPassDesc.depthReadOnly;
		if (!renderPassDesc.depthReadOnly)
		{
			rhiDepthStencilAttachment.depthClearValue = renderPassDesc.depthClearValue;
			rhiDepthStencilAttachment.depthLoadOp = g_wgpuLoadOp[renderPassDesc.depthLoadOp];
			rhiDepthStencilAttachment.depthStoreOp = g_wgpuStoreOp[renderPassDesc.depthStoreOp];
		}

		const bool hasStencil = IsStencilFormat(renderPassDesc.depthStencil.texture->GetFormat());
		rhiDepthStencilAttachment.stencilReadOnly = renderPassDesc.stencilReadOnly;
		if (hasStencil && !renderPassDesc.stencilReadOnly)
		{
			rhiDepthStencilAttachment.stencilClearValue = renderPassDesc.stencilClearValue;
			rhiDepthStencilAttachment.stencilLoadOp = g_wgpuLoadOp[renderPassDesc.stencilLoadOp];
			rhiDepthStencilAttachment.stencilStoreOp = g_wgpuStoreOp[renderPassDesc.stencilStoreOp];
		}
		rhiRenderPassDesc.depthStencilAttachment = &rhiDepthStencilAttachment;
	}

	// TODO:
	// rhiRenderPassDesc.occlusionQuerySet
}

void FindWGPUBindGroupEntry(const BindGroupDesc::Entry& bindGroupEntry, const char* dbgName, WGPUBindGroupEntry& rhiBindGroupEntryDesc)
{
	WGPUDevice rhiDevice = CWGPURenderAPI::Instance.GetWGPUDevice();

	rhiBindGroupEntryDesc.binding = bindGroupEntry.binding;
	switch (bindGroupEntry.type)
	{
	case BINDENTRY_BUFFER:
	{
		CWGPUBuffer* buffer = static_cast<CWGPUBuffer*>(bindGroupEntry.buffer.ptr);
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
		CWGPUTexture* texture = static_cast<CWGPUTexture*>(bindGroupEntry.texture.ptr);

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

void FillWGPUBindGroupEntriesByLayoutMap(const BindGroupDesc& bindGroupDesc, const CWGPUBindingLayout::BindGroupLayoutMap& groupLayoutMap, int maxBindingIndex, Array<WGPUBindGroupEntry>& rhiBindGroupEntryList)
{
	for (const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		WGPUBindGroupEntry& rhiBindGroupEntryDesc = rhiBindGroupEntryList.append();
		FindWGPUBindGroupEntry(bindGroupEntry, "", rhiBindGroupEntryDesc);

		if (bindGroupEntry.binding > maxBindingIndex)
		{
			auto it = groupLayoutMap.find(bindGroupEntry.binding);
			if (it)
				rhiBindGroupEntryDesc.binding = *it;
		}
	}
}

void FillWGPUBindGroupEntries(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, Array<WGPUBindGroupEntry>& rhiBindGroupEntryList)
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
				return bindGroupEntry.binding == binding.nameId;
			});

			if (entryIdx == -1)
				continue;

			usedBindingEntries.setTrue(bindingIds[i]);

			const BindGroupDesc::Entry& bindGroupEntry = bindGroupDesc.entries[entryIdx];
			WGPUBindGroupEntry& rhiBindGroupEntryDesc = rhiBindGroupEntryList.append();
			FindWGPUBindGroupEntry(bindGroupEntry, "", rhiBindGroupEntryDesc);

			// store correct index
			rhiBindGroupEntryDesc.binding = binding.index;
		}
	}

	ASSERT_MSG(bindGroupDesc.entries.numElem() >= bindingsToResolve, "Bad binding entry count: %d, expected %d", rhiBindGroupEntryList.numElem(), bindingsToResolve);
	ASSERT_MSG(rhiBindGroupEntryList.numElem() == bindingsToResolve, "Incorrect binding ids, resolved: %d, expected %d", rhiBindGroupEntryList.numElem(), bindingsToResolve);
}

CWGPUBindingLayout::~CWGPUBindingLayout()
{
	if(m_rhiPipelineLayout)
		wgpuPipelineLayoutRelease(m_rhiPipelineLayout);

	for (WGPUBindGroupLayout layout : m_rhiBindGroupLayout)
	{
		if(layout)
			wgpuBindGroupLayoutRelease(layout);
	}
}

//--------------------------------------------

CWGPURenderPipeline::~CWGPURenderPipeline()
{
	wgpuRenderPipelineRelease(m_rhiRenderPipeline);

	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Decrement(stats.pipelines);
}

CWGPURenderPipeline::CWGPURenderPipeline()
{
	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Increment(stats.pipelines);
}

//--------------------------------------------

CWGPUComputePipeline::~CWGPUComputePipeline()
{
	wgpuComputePipelineRelease(m_rhiComputePipeline);

	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Decrement(stats.pipelines);
}

CWGPUComputePipeline::CWGPUComputePipeline()
{
	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Increment(stats.pipelines);
}

//--------------------------------------------

CWGPUBindGroup::~CWGPUBindGroup()
{
	wgpuBindGroupRelease(m_rhiBindGroup);

	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Decrement(stats.bindGroups);
}

CWGPUBindGroup::CWGPUBindGroup()
{
	ShaderAPIStats& stats = CWGPURenderAPI::Instance.GetStatsMutable();
	Atomic::Increment(stats.bindGroups);
}

//--------------------------------------------

CWGPUCommandBuffer::~CWGPUCommandBuffer()
{
	wgpuCommandBufferRelease(m_rhiCommandBuffer);
}