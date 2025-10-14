#include <nvrhi/nvrhi.h>
#include "core/core_common.h"
#include "NVRHIComputePassRecorder.h"
#include "NVRHIStates.h"
#include "NVRHIBuffer.h"
#include "NVRHIRenderAPI.h"

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
			bindGroupImpl->m_bindingLayout->FillBindingSetDescByLayoutMap(bindGroupDesc, *pipelineImpl->m_shaderInfo, ArrayCRef(&pipelineImpl->m_computeShaderModuleIdx, 1), rhiSamplers, rhiBindingSetDesc);

			nvrhi::BindingSetHandle rhiBindSet = nvrhiDevice->createBindingSet(rhiBindingSetDesc, pipelineImpl->m_rhiBindingLayout[bindGroupDesc.groupIdx]);
			if (!rhiBindSet)
			{
				ASSERT_FAIL("Failed to create bind group %s\n", bindGroupDesc.name.ToCString());
				continue;
			}

			rhiComputeState.addBindingSet(rhiBindSet);
		}
		else
		{
			rhiComputeState.addBindingSet(bindGroupImpl->m_rhiBindingSet);
		}
	}

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
}

void CNVRHIComputePassRecorder::Complete()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Render pass recorder was already ended");
		return;
	}
	m_rhiCommandList = nullptr;
}

IGPUCommandBufferPtr CNVRHIComputePassRecorder::End()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Compute pass recorder was already ended or is owned by GPUCommandRecorder, use Complete in this case");
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