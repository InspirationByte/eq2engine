#include <nvrhi/nvrhi.h>
#include "core/core_common.h"

#include "NVRHIBuffer.h"
#include "NVRHIStates.h"
#include "NVRHIRenderPassRecorder.h"
#include "NVRHIRenderDefs.h"
#include "NVRHITexture.h"
#include "NVRHIRenderAPI.h"

//-------------------------------------------

void CNVRHIRenderPassRecorder::DbgPopGroup() const
{
	m_rhiCommandList->endMarker();
}

void CNVRHIRenderPassRecorder::DbgPushGroup(const char* groupLabel) const
{
	m_rhiCommandList->beginMarker(groupLabel);
}

void CNVRHIRenderPassRecorder::DbgAddMarker(const char* label) const
{
	m_rhiCommandList->beginMarker(label);
	m_rhiCommandList->endMarker();
}

void CNVRHIRenderPassRecorder::CommitGraphicsState(nvrhi::IBuffer* indirectBuffer)
{
	if (!m_graphicsStateDirty)
		return;
	m_graphicsStateDirty = false;

	nvrhi::IDevice* nvrhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	auto rhiViewportState = nvrhi::ViewportState()
		.addViewport(m_rhiViewport)
		.addScissorRect(m_rhiScissor);

	CNVRHIRenderPipeline* pipelineImpl = static_cast<CNVRHIRenderPipeline*>(m_pipeline.Ptr());
	ASSERT(pipelineImpl);

	const nvrhi::GraphicsPipelineDesc& rhiPipelineDesc = pipelineImpl->m_rhiPipelineDesc;

	// time to create graphics pipeline instance
	// TODO: don't do it, use framebuffer desc in CreatePipeline
	// TODO: Vulkan dynamic rendering ext support in NVRHI
	nvrhi::GraphicsPipelineHandle rhiPipeline = nvrhiDevice->createGraphicsPipeline(rhiPipelineDesc, m_rhiFramebuffer);

	auto rhiGraphicsState = nvrhi::GraphicsState()
		.setPipeline(rhiPipeline)
		.setFramebuffer(m_rhiFramebuffer)
		.setViewport(rhiViewportState)
		.setIndirectParams(indirectBuffer);

	CNVRHIBuffer* indexBufferImpl = static_cast<CNVRHIBuffer*>(m_indexBuffer.buffer.Ptr());
	if (indexBufferImpl)
	{
		ASSERT_MSG(indexBufferImpl->GetUsageFlags() & BUFFERUSAGE_INDEX, "buffer doesn't have Index buffer usage bit");

		auto rhiIndexBuffer = nvrhi::IndexBufferBinding()
			.setBuffer(indexBufferImpl->GetNVRHIBufferHandle())
			.setOffset(m_indexBuffer.offset)
			.setFormat(g_nvrhiIndexFormat[m_indexFormat]);
		rhiGraphicsState.setIndexBuffer(rhiIndexBuffer);
	}

	int slotId = 0;
	for (const GPUBufferView& vertexBindings : m_rhiVertexBuffers)
	{
		if (vertexBindings.buffer)
		{
			CNVRHIBuffer* vertexBufferImpl = static_cast<CNVRHIBuffer*>(vertexBindings.buffer.Ptr());
			ASSERT_MSG(vertexBufferImpl->GetUsageFlags() & BUFFERUSAGE_VERTEX, "buffer at slot %d doesn't have Vertex buffer usage bit", slotId);

			auto rhiVertexBuffer = nvrhi::VertexBufferBinding()
				.setBuffer(vertexBufferImpl->GetNVRHIBufferHandle())
				.setOffset(vertexBindings.offset)
				.setSlot(slotId);

			rhiGraphicsState.addVertexBuffer(rhiVertexBuffer);
		}
		++slotId;
	}	
	
	const int shaderModuleIdxs[] = {
		pipelineImpl->m_vertexShaderModuleIdx,
		pipelineImpl->m_fragmentShaderModuleIdx
	};

	static thread_local Array<nvrhi::BindingSetHandle> createdBindingSets(PP_SL);

	for (IGPUBindGroup* bindGroup : m_bindings)
	{
		CNVRHIBindGroup* bindGroupImpl = static_cast<CNVRHIBindGroup*>(bindGroup);
		if (!bindGroupImpl)
			continue;

		if (bindGroupImpl->m_bindingLayout)
		{
			const BindGroupDesc& bindGroupDesc = bindGroupImpl->m_bindGroupDesc;

			static thread_local NVRHISamplerHandleList rhiSamplers;
			rhiSamplers.clear();

			// we need to create binding set for this shader using provided layout
			auto rhiBindingSetDesc = nvrhi::BindingSetDesc();
			bindGroupImpl->m_bindingLayout->FillBindingSetDescByLayoutMap(bindGroupDesc, *pipelineImpl->m_shaderInfo, shaderModuleIdxs, rhiSamplers, rhiBindingSetDesc);

			nvrhi::BindingSetHandle rhiBindSet = nvrhiDevice->createBindingSet(rhiBindingSetDesc, rhiPipelineDesc.bindingLayouts[bindGroupDesc.groupIdx]);
			if (!rhiBindSet)
			{
				ASSERT_FAIL("Failed to create bind group %s\n", bindGroupDesc.name.ToCString());
				continue;
			}

			createdBindingSets.append(rhiBindSet);
			rhiGraphicsState.addBindingSet(rhiBindSet);
		}
		else
		{
			rhiGraphicsState.addBindingSet(bindGroupImpl->m_rhiBindingSet);
		}
	}

	m_rhiCommandList->setGraphicsState(rhiGraphicsState);
	createdBindingSets.clear();
}

void CNVRHIRenderPassRecorder::SetPipeline(IGPURenderPipeline* pipeline)
{
	for (int i = 0; i < MAX_BINDGROUPS; ++i)
		m_bindings[i] = nullptr;

	m_pipeline.Assign(pipeline);
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup)
{
	m_bindings[groupIndex].Assign(bindGroup);
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset, int64 size)
{
	m_rhiVertexBuffers[slot] = GPUBufferView(vertexBuffer, offset, size);
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::SetIndexBuffer(IGPUBuffer* indexBuf, EIndexFormat indexFormat, int64 offset, int64 size)
{
	m_indexBuffer = GPUBufferView(indexBuf, offset, size);
	m_indexFormat = indexFormat;
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth)
{
	const Vector2D rectLT = rectangle.GetLeftTop();
	const Vector2D rectRB = rectangle.GetRightBottom();
	m_rhiViewport = nvrhi::Viewport(rectLT.x, rectRB.x, rectLT.y, rectRB.y, minDepth, maxDepth);
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::SetScissorRectangle(const IAARectangle& rectangle)
{
	// clamp scissor to screen size
	const IAARectangle screenRect(IVector2D(0, 0), GetRenderTargetDimensions());
	IAARectangle rect;
	rect.leftTop = clamp(rectangle.leftTop, screenRect.leftTop, screenRect.rightBottom);
	rect.rightBottom = clamp(rectangle.rightBottom, screenRect.leftTop, screenRect.rightBottom);

	const Vector2D rectLT = screenRect.GetLeftTop();
	const Vector2D rectRB = screenRect.GetRightBottom();
	m_rhiScissor = nvrhi::Rect(rectLT.x, rectRB.y, rectLT.y, rectRB.y);
	m_graphicsStateDirty = true;
}

void CNVRHIRenderPassRecorder::Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance)
{
	CommitGraphicsState();

	m_rhiCommandList->draw(nvrhi::DrawArguments()
		.setVertexCount(vertexCount)
		.setStartVertexLocation(firstVertex)
		.setInstanceCount(instanceCount)
		.setStartInstanceLocation(firstInstance)
	);
}

void CNVRHIRenderPassRecorder::DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex, int firstInstance)
{
	CommitGraphicsState();

	m_rhiCommandList->drawIndexed(nvrhi::DrawArguments()
		.setVertexCount(indexCount)
		.setStartIndexLocation(firstIndex)
		.setStartVertexLocation(baseVertex)
		.setInstanceCount(instanceCount)
		.setStartInstanceLocation(firstInstance)
	);
}

void CNVRHIRenderPassRecorder::DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset)
{
	CNVRHIBuffer* indirectBufferImpl = static_cast<CNVRHIBuffer*>(indirectBuffer);
	ASSERT(indirectBufferImpl);
	ASSERT_MSG(indirectBufferImpl->GetUsageFlags() & BUFFERUSAGE_INDIRECT, "buffer doesn't have Indirect buffer usage bit");

	// since indirect buffer is part of state, we need to update it
	m_graphicsStateDirty = true;

	CommitGraphicsState(indirectBufferImpl->GetNVRHIBufferHandle());

	m_rhiCommandList->drawIndexedIndirect(indirectOffset);
}

void CNVRHIRenderPassRecorder::DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset)
{
	CNVRHIBuffer* indirectBufferImpl = static_cast<CNVRHIBuffer*>(indirectBuffer);
	ASSERT(indirectBufferImpl);
	ASSERT_MSG(indirectBufferImpl->GetUsageFlags() & BUFFERUSAGE_INDIRECT, "buffer doesn't have Indirect buffer usage bit");
	CommitGraphicsState(indirectBufferImpl->GetNVRHIBufferHandle());

	m_rhiCommandList->drawIndirect(indirectOffset);
}


void CNVRHIRenderPassRecorder::MultiDrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset, int maxDrawCount, IGPUBuffer* drawCountBuffer, int drawCountBufferOffset)
{
	ASSERT_FAIL("Unsupported on this RHI");
}

void CNVRHIRenderPassRecorder::MultiDrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset, int maxDrawCount, IGPUBuffer* drawCountBuffer, int drawCountBufferOffset)
{
	ASSERT_FAIL("Unsupported on this RHI");
}

static void NVRHIBeginRenderPass(const RenderPassDesc& renderPassDesc, nvrhi::CommandListHandle rhiCmdList, nvrhi::FramebufferDesc& rhiFramebufferDesc)
{
	for (const RenderPassDesc::ColorTargetDesc& colorTarget : renderPassDesc.colorTargets)
	{
		const CNVRHITexture* targetTexture = static_cast<CNVRHITexture*>(colorTarget.target.texture.Ptr());
		const CNVRHITexture* resolveTargetTexture = static_cast<CNVRHITexture*>(colorTarget.resolveTarget.texture.Ptr());

		rhiFramebufferDesc.addColorAttachment(targetTexture->GetNVRHITextureHandle(), targetTexture->GetNVRHITextureView(colorTarget.target.arraySlice));
		if (colorTarget.loadOp == LOADFUNC_CLEAR)
		{
			rhiCmdList->clearTextureFloat(targetTexture->GetNVRHITextureHandle(), targetTexture->GetNVRHITextureView(colorTarget.target.arraySlice),
				nvrhi::Color{ colorTarget.clearColor.r, colorTarget.clearColor.g, colorTarget.clearColor.b, colorTarget.clearColor.a });
		}

		// TODO: on Complete()
		//if (resolveTargetTexture)
		//	rhiCmdList->resolveTexture();
	}

	if (renderPassDesc.depthStencil)
	{
		const CNVRHITexture* depthTexture = static_cast<CNVRHITexture*>(renderPassDesc.depthStencil.texture.Ptr());

		auto rhiDepthAttachment = nvrhi::FramebufferAttachment()
			.setSubresources(depthTexture->GetNVRHITextureView(renderPassDesc.depthStencil.arraySlice))
			.setTexture(depthTexture->GetNVRHITextureHandle())
			.setReadOnly(renderPassDesc.depthReadOnly);

		rhiFramebufferDesc.setDepthAttachment(depthTexture->GetNVRHITextureHandle());

		const bool clearDepth = !renderPassDesc.depthReadOnly && renderPassDesc.depthLoadOp == LOADFUNC_CLEAR;
		const bool clearStencil = !renderPassDesc.stencilReadOnly && IsStencilFormat(renderPassDesc.depthStencil.texture->GetFormat()) && renderPassDesc.stencilLoadOp == LOADFUNC_CLEAR;
		rhiCmdList->clearDepthStencilTexture(depthTexture->GetNVRHITextureHandle(), depthTexture->GetNVRHITextureView(renderPassDesc.depthStencil.arraySlice),
			clearDepth, renderPassDesc.depthClearValue, clearStencil, (uint8)renderPassDesc.stencilClearValue);

		// TODO
		// stencilStoreOp
	}
}

void CNVRHIRenderPassRecorder::InternalBeginRenderPass(const RenderPassDesc& renderPassDesc)
{
	auto rhiFramebufferDesc = nvrhi::FramebufferDesc();
	NVRHIBeginRenderPass(renderPassDesc, m_rhiCommandList, rhiFramebufferDesc);

	m_rhiFramebuffer = CNVRHIRenderAPI::Instance.GetNVRHIDevice()->createFramebuffer(rhiFramebufferDesc);
	m_rhiCommandList->setResourceStatesForFramebuffer(m_rhiFramebuffer);

	IVector2D renderTargetDims = 0;
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		if (colorTarget.target.texture)
		{
			renderTargetDims = IVector2D(colorTarget.target.texture->GetWidth(), colorTarget.target.texture->GetHeight());
			m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target.texture->GetFormat() : FORMAT_NONE;

			if (colorTarget.target)
				m_renderTargetMSAASamples = colorTarget.target.texture->GetSampleCount();
		}
	}

	if (renderPassDesc.depthStencil)
	{
		renderTargetDims = IVector2D(renderPassDesc.depthStencil.texture->GetWidth(), renderPassDesc.depthStencil.texture->GetHeight());
		m_depthTargetFormat = renderPassDesc.depthStencil.texture->GetFormat();
	}
	else
		m_depthTargetFormat = FORMAT_NONE;

	m_depthReadOnly = renderPassDesc.depthReadOnly;
	m_stencilReadOnly = renderPassDesc.stencilReadOnly;
	m_renderTargetDims = renderTargetDims;
	m_dbgName = renderPassDesc.name;

	const AARectangle defaultViewportRectangle(vec2_zero, Vector2D(renderTargetDims));
	SetViewport(defaultViewportRectangle, 0.0f, 1.0f);
	SetScissorRectangle(defaultViewportRectangle);
}

// TODO:

// CNVRHIRenderPassRecorder::BeginOcclusionQuery(uint32_t queryIndex);
// CNVRHIRenderPassRecorder::EndOcclusionQuery();

// CNVRHIRenderPassRecorder::ExecuteBundles(size_t bundleCount, WGPURenderBundle const* bundles);
// CNVRHIRenderPassRecorder::PixelLocalStorageBarrier();

// CNVRHIRenderPassRecorder::InsertDebugMarker(char const* markerLabel);
// CNVRHIRenderPassRecorder::PopDebugGroup();
// CNVRHIRenderPassRecorder::PushDebugGroup(char const* groupLabel);

// CNVRHIRenderPassRecorder::SetBlendConstant(WGPUColor const* color);
// CNVRHIRenderPassRecorder::SetStencilReference(uint32_t reference);

void CNVRHIRenderPassRecorder::Complete()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Render pass recorder was already ended");
		return;
	}
	m_rhiCommandList = nullptr;
}

IGPUCommandBufferPtr CNVRHIRenderPassRecorder::End()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Render pass recorder was already ended or is owned by GPUCommandRecorder, use Complete in this case");
		return nullptr;
	}
	m_rhiCommandList->commitBarriers();
	m_rhiCommandList->close();

	CRefPtr<CNVRHICommandBuffer> commandBuffer = CRefPtr_new(CNVRHICommandBuffer);
	commandBuffer->m_rhiCommandList = m_rhiCommandList;
	commandBuffer->m_dbgName = std::move(m_dbgName);
	m_rhiCommandList = nullptr;

	return IGPUCommandBufferPtr(commandBuffer);
}
