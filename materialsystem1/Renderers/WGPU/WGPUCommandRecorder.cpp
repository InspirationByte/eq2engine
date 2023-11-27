#include <webgpu/webgpu.h>
#include "core/core_common.h"

#include "WGPUBuffer.h"
#include "WGPUStates.h"
#include "WGPUCommandRecorder.h"
#include "WGPURenderPassRecorder.h"
#include "WGPURenderDefs.h"
#include "WGPUTexture.h"

CWGPUCommandRecorder::~CWGPUCommandRecorder()
{
	if(m_rhiCommandEncoder)
		wgpuCommandEncoderRelease(m_rhiCommandEncoder);
}

void CWGPUCommandRecorder::WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const
{
	CWGPUBuffer* bufferImpl = static_cast<CWGPUBuffer*>(buffer);
	if (!bufferImpl)
		return;

	const int64 writeDataSize = (size + 3) & ~3;
	if (writeDataSize <= 0)
		return;

	wgpuCommandEncoderWriteBuffer(m_rhiCommandEncoder, bufferImpl->GetWGPUBuffer(), offset, reinterpret_cast<const uint8_t*>(data), writeDataSize);
	m_hasCommands = true;
}

IGPUCommandBufferPtr CWGPUCommandRecorder::End()
{
	if (!m_rhiCommandEncoder)
	{
		ASSERT_FAIL("Command recorder was already ended");
		return nullptr;
	}

	if (!m_hasCommands)
		return nullptr;

	CRefPtr<CWGPUCommandBuffer> commandBuffer = CRefPtr_new(CWGPUCommandBuffer);

	WGPUCommandBuffer rhiCommandBuffer = wgpuCommandEncoderFinish(m_rhiCommandEncoder, nullptr);
	wgpuCommandEncoderRelease(m_rhiCommandEncoder);
	m_rhiCommandEncoder = nullptr;

	commandBuffer->m_rhiCommandBuffer = rhiCommandBuffer;
	return IGPUCommandBufferPtr(commandBuffer);
}

IGPURenderPassRecorderPtr CWGPUCommandRecorder::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	WGPURenderPassDescriptor rhiRenderPassDesc = {};
	rhiRenderPassDesc.label = renderPassDesc.name.Length() ? renderPassDesc.name.ToCString() : nullptr;

	FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS> rhiColorAttachmentList;

	IVector2D renderTargetDims = 0;

	for (const RenderPassDesc::ColorTargetDesc& colorTarget : renderPassDesc.colorTargets)
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

	if (renderPassDesc.depthStencil)
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

	WGPURenderPassEncoder rhiRenderPassEncoder = wgpuCommandEncoderBeginRenderPass(m_rhiCommandEncoder, &rhiRenderPassDesc);
	if (!rhiRenderPassEncoder)
		return nullptr;

	CRefPtr<CWGPURenderPassRecorder> renderPass = CRefPtr_new(CWGPURenderPassRecorder);
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target->GetFormat() : FORMAT_NONE;
	}
	renderPass->m_depthTargetFormat = renderPassDesc.depthStencil ? renderPassDesc.depthStencil->GetFormat() : FORMAT_NONE;
	renderPass->m_rhiCommandEncoder = m_rhiCommandEncoder;
	renderPass->m_rhiRenderPassEncoder = rhiRenderPassEncoder;
	renderPass->m_renderTargetDims = renderTargetDims;
	renderPass->m_userData = userData;

	m_hasCommands = true;

	return IGPURenderPassRecorderPtr(renderPass);
}