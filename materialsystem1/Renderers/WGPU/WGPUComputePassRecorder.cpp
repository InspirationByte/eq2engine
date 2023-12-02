#include <webgpu/webgpu.h>
#include "core/core_common.h"
#include "WGPUComputePassRecorder.h"
#include "WGPUStates.h"
#include "WGPUBuffer.h"

CWGPUComputePassRecorder::~CWGPUComputePassRecorder()
{
	if(m_rhiComputePassEncoder)
		wgpuComputePassEncoderRelease(m_rhiComputePassEncoder);
	m_rhiComputePassEncoder = nullptr;

	if (m_rhiCommandEncoder)
		wgpuCommandEncoderRelease(m_rhiCommandEncoder);
}

void CWGPUComputePassRecorder::SetPipeline(IGPUComputePipeline* pipeline)
{
	m_pipeline.Assign(pipeline);

	ASSERT(pipeline);
	CWGPUComputePipeline* pipelineImpl = static_cast<CWGPUComputePipeline*>(pipeline);
	if (!pipelineImpl)
		return;
	wgpuComputePassEncoderSetPipeline(m_rhiComputePassEncoder, pipelineImpl->m_rhiComputePipeline);
}

void CWGPUComputePassRecorder::SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets)
{
	CWGPUBindGroup* bindGroupImpl = static_cast<CWGPUBindGroup*>(bindGroup);
	if (bindGroupImpl)
		wgpuComputePassEncoderSetBindGroup(m_rhiComputePassEncoder, groupIndex, bindGroupImpl->m_rhiBindGroup, dynamicOffsets.numElem(), dynamicOffsets.ptr());
	else
		wgpuComputePassEncoderSetBindGroup(m_rhiComputePassEncoder, groupIndex, nullptr, 0, nullptr);
}

void CWGPUComputePassRecorder::DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY, int32 workgroupCountZ)
{
	wgpuComputePassEncoderDispatchWorkgroups(m_rhiComputePassEncoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void CWGPUComputePassRecorder::DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset)
{
	CWGPUBuffer* indirectBufferImpl = static_cast<CWGPUBuffer*>(indirectBuffer);
	wgpuComputePassEncoderDispatchWorkgroupsIndirect(m_rhiComputePassEncoder, indirectBufferImpl->GetWGPUBuffer(), indirectOffset);
}

void CWGPUComputePassRecorder::Complete()
{
	if (!m_rhiComputePassEncoder)
	{
		ASSERT_FAIL("Render pass recorder was already ended");
		return;
	}
	wgpuComputePassEncoderEnd(m_rhiComputePassEncoder);
	wgpuComputePassEncoderRelease(m_rhiComputePassEncoder);
	m_rhiComputePassEncoder = nullptr;
}

IGPUCommandBufferPtr CWGPUComputePassRecorder::End()
{
	Complete();

	if (!m_rhiCommandEncoder)
	{
		ASSERT_FAIL("Compute pass recorder was already ended or is owned by GPUCommandRecorder, use Complete in this case");
		return nullptr;
	}

	CRefPtr<CWGPUCommandBuffer> commandBuffer = CRefPtr_new(CWGPUCommandBuffer);

	WGPUCommandBuffer rhiCommandBuffer = wgpuCommandEncoderFinish(m_rhiCommandEncoder, nullptr);
	wgpuCommandEncoderRelease(m_rhiCommandEncoder);

	commandBuffer->m_rhiCommandBuffer = rhiCommandBuffer;
	m_rhiCommandEncoder = nullptr;

	return IGPUCommandBufferPtr(commandBuffer);
}