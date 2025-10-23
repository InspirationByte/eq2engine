#include <nvrhi/nvrhi.h>
#include "core/core_common.h"
#include "NVRHIComputePassRecorder.h"
#include "NVRHIStates.h"
#include "NVRHIBuffer.h"
#include "NVRHIRenderAPI.h"
#include "../RenderWorker.h"

CNVRHIComputePassRecorder::CNVRHIComputePassRecorder(nvrhi::ICommandList* cmdList, int cmdListIdx, void* userData, const char* label)
	: m_rhiCommandList(cmdList)
	, m_userData(userData)
	, m_cmdListIdx(cmdListIdx)
	, m_dbgName(label)
{
}

void CNVRHIComputePassRecorder::DbgPopGroup() const
{
	m_rhiCommandList->endMarker();
}

void CNVRHIComputePassRecorder::DbgPushGroup(const char* groupLabel) const
{
	m_rhiCommandList->beginMarker(groupLabel);
}

void CNVRHIComputePassRecorder::DbgAddMarker(const char* label) const
{
	m_rhiCommandList->beginMarker(label);
	m_rhiCommandList->endMarker();
}

void CNVRHIComputePassRecorder::CommitComputeState(nvrhi::IBuffer* indirectBuffer)
{
	if (!m_computeStateDirty)
		return;
	m_computeStateDirty = false;

	nvrhi::IDevice* nvrhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	CNVRHIComputePipeline* pipelineImpl = static_cast<CNVRHIComputePipeline*>(m_pipeline.Ptr());
	ASSERT(pipelineImpl);
	
	auto rhiComputeState = nvrhi::ComputeState()
		.setPipeline(pipelineImpl->m_rhiComputePipeline)
		.setIndirectParams(indirectBuffer);

	if (indirectBuffer)
		m_rhiCommandList->setBufferState(indirectBuffer, nvrhi::ResourceStates::IndirectArgument);

	const int shaderModuleIdxs[] = { pipelineImpl->m_computeShaderModuleIdx };
	nvrhiFillBindingSets(*pipelineImpl->m_shaderInfo, shaderModuleIdxs, m_bindings, pipelineImpl->m_pipelineId, pipelineImpl->m_rhiBindingLayout, rhiComputeState.bindings);

	m_rhiCommandList->setComputeState(rhiComputeState);
}

void CNVRHIComputePassRecorder::SetPipeline(IGPUComputePipeline* pipeline)
{
	m_pipeline.Assign(pipeline);
	m_computeStateDirty = true;
}

void CNVRHIComputePassRecorder::SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup)
{
	m_bindings[groupIndex].Assign(bindGroup);
	m_computeStateDirty = true;
}

void CNVRHIComputePassRecorder::DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY, int32 workgroupCountZ)
{
	CommitComputeState();
	m_rhiCommandList->dispatch(workgroupCountX, workgroupCountY, workgroupCountZ);

	ShaderAPIStats& stats = CNVRHIRenderAPI::Instance.GetStatsMutable();
	Atomic::Increment(stats.dispatchCount);
}

void CNVRHIComputePassRecorder::DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset)
{
	CNVRHIBuffer* indirectBufferImpl = static_cast<CNVRHIBuffer*>(indirectBuffer);
	ASSERT(indirectBufferImpl);
	ASSERT_MSG(indirectBufferImpl->GetUsageFlags() & BUFFERUSAGE_INDIRECT, "buffer doesn't have Indirect buffer usage bit");

	// since indirect buffer is part of state, we need to update it
	m_computeStateDirty = true;

	CommitComputeState(indirectBufferImpl->GetNVRHIBufferHandle());
	m_rhiCommandList->dispatchIndirect(indirectOffset);

	ShaderAPIStats& stats = CNVRHIRenderAPI::Instance.GetStatsMutable();
	Atomic::Increment(stats.indirectDispatchCount);
}

void CNVRHIComputePassRecorder::Complete()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Render pass recorder was already ended");
		return;
	}
	m_rhiCommandList = nullptr;
	m_cmdListIdx = -1;
}

IGPUCommandBufferPtr CNVRHIComputePassRecorder::End()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Compute pass recorder was already ended or is owned by GPUCommandRecorder, use Complete in this case");
		return nullptr;
	}
	m_rhiCommandList->close();

	CRefPtr<CNVRHICommandBuffer> commandBuffer = CNVRHICommandBuffer::Create();
	commandBuffer->m_rhiCommandList = m_rhiCommandList;
	commandBuffer->m_dbgName = std::move(m_dbgName);
	commandBuffer->m_cmdListIdx = m_cmdListIdx;

	m_rhiCommandList = nullptr;
	m_cmdListIdx = -1;

	return IGPUCommandBufferPtr(commandBuffer);
}