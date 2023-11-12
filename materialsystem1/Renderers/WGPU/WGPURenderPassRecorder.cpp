#include <webgpu/webgpu.h>
#include "core/core_common.h"

#include "WGPUBuffer.h"
#include "WGPUStates.h"
#include "WGPURenderPassRecorder.h"
#include "WGPURenderDefs.h"

CWGPURenderPassRecorder::~CWGPURenderPassRecorder()
{
	wgpuCommandEncoderRelease(m_rhiCommandEncoder);
	wgpuRenderPassEncoderRelease(m_rhiRenderPassEncoder);
}

void CWGPURenderPassRecorder::SetPipeline(IGPURenderPipeline* pipeline)
{
	CWGPURenderPipeline* pipelineImpl = static_cast<CWGPURenderPipeline*>(pipeline);
	wgpuRenderPassEncoderSetPipeline(m_rhiRenderPassEncoder, pipelineImpl->m_rhiRenderPipeline);
}

void CWGPURenderPassRecorder::SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets) // TODO: dynamic offsets
{
	CWGPUBindGroup* bindGroupImpl = static_cast<CWGPUBindGroup*>(bindGroup);
	wgpuRenderPassEncoderSetBindGroup(m_rhiRenderPassEncoder, groupIndex, bindGroupImpl->m_rhiBindGroup, dynamicOffsets.numElem(), dynamicOffsets.ptr());
}

void CWGPURenderPassRecorder::SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset, int64 size)
{
	if (size < 0) size = WGPU_WHOLE_SIZE;
	CWGPUBuffer* vertexBufferImpl = static_cast<CWGPUBuffer*>(vertexBuffer);
	ASSERT_MSG(vertexBufferImpl->GetUsageFlags() & WGPUBufferUsage_Vertex, "buffer doesn't have Vertex buffer usage bit");
	wgpuRenderPassEncoderSetVertexBuffer(m_rhiRenderPassEncoder, slot, vertexBufferImpl->GetWGPUBuffer(), offset, size);
}

void CWGPURenderPassRecorder::SetIndexBuffer(IGPUBuffer* indexBuf, EIndexFormat indexFormat, int64 offset, int64 size)
{
	if (size < 0) size = WGPU_WHOLE_SIZE;
	CWGPUBuffer* indexBufferImpl = static_cast<CWGPUBuffer*>(indexBuf);
	ASSERT_MSG(indexBufferImpl->GetUsageFlags() & WGPUBufferUsage_Index, "buffer doesn't have Index buffer usage bit");
	wgpuRenderPassEncoderSetIndexBuffer(m_rhiRenderPassEncoder, indexBufferImpl->GetWGPUBuffer(), g_wgpuIndexFormat[indexFormat], 0, size);
}

void CWGPURenderPassRecorder::SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth)
{
	const Vector2D rectPos = rectangle.GetLeftTop();
	const Vector2D rectSize = rectangle.GetSize();
	wgpuRenderPassEncoderSetViewport(m_rhiRenderPassEncoder, rectPos.x, rectPos.y, rectSize.x, rectSize.y, minDepth, maxDepth);
}

void CWGPURenderPassRecorder::SetScissorRectangle(const IAARectangle& rectangle, float minDepth, float maxDepth)
{
	const IVector2D rectPos = rectangle.GetLeftTop();
	const IVector2D rectSize = rectangle.GetSize();
	wgpuRenderPassEncoderSetScissorRect(m_rhiRenderPassEncoder, rectPos.x, rectPos.y, rectSize.x, rectSize.y);
}

void CWGPURenderPassRecorder::Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance)
{
	wgpuRenderPassEncoderDraw(m_rhiRenderPassEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CWGPURenderPassRecorder::DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex, int firstInstance)
{
	wgpuRenderPassEncoderDrawIndexed(m_rhiRenderPassEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstIndex);
}

void CWGPURenderPassRecorder::DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset)
{
	CWGPUBuffer* indexBufferImpl = static_cast<CWGPUBuffer*>(indirectBuffer);
	wgpuRenderPassEncoderDrawIndexedIndirect(m_rhiRenderPassEncoder, indexBufferImpl->GetWGPUBuffer(), indirectOffset);
}

void CWGPURenderPassRecorder::DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset)
{
	CWGPUBuffer* indexBufferImpl = static_cast<CWGPUBuffer*>(indirectBuffer);
	wgpuRenderPassEncoderDrawIndirect(m_rhiRenderPassEncoder, indexBufferImpl->GetWGPUBuffer(), indirectOffset);
}

// TODO:

// CWGPURenderPassRecorder::BeginOcclusionQuery(uint32_t queryIndex);
// CWGPURenderPassRecorder::EndOcclusionQuery();

// CWGPURenderPassRecorder::ExecuteBundles(size_t bundleCount, WGPURenderBundle const* bundles);
// CWGPURenderPassRecorder::PixelLocalStorageBarrier();

// CWGPURenderPassRecorder::InsertDebugMarker(char const* markerLabel);
// CWGPURenderPassRecorder::PopDebugGroup();
// CWGPURenderPassRecorder::PushDebugGroup(char const* groupLabel);

// CWGPURenderPassRecorder::SetBlendConstant(WGPUColor const* color);
// CWGPURenderPassRecorder::SetStencilReference(uint32_t reference);

IGPUCommandBufferPtr CWGPURenderPassRecorder::End()
{
	wgpuRenderPassEncoderEnd(m_rhiRenderPassEncoder);

	CRefPtr<CWGPUCommandBuffer> commandBuffer = CRefPtr_new(CWGPUCommandBuffer);
	WGPUCommandBuffer rhiCommandBuffer = wgpuCommandEncoderFinish(m_rhiCommandEncoder, nullptr);
	commandBuffer->m_rhiCommandBuffer = rhiCommandBuffer;
	return IGPUCommandBufferPtr(commandBuffer);
}
