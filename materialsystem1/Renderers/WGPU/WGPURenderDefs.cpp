#include "core/core_common.h"
#include "WGPURenderDefs.h"
#include "WGPUTexture.h"

void FillWGPUSamplerDescriptor(const SamplerStateParams& samplerParams, WGPUSamplerDescriptor& rhiSamplerDesc)
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
		rhiColorAttachment.depthSlice = colorTarget.depthSlice >= 0 ? colorTarget.depthSlice : WGPU_DEPTH_SLICE_UNDEFINED;
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