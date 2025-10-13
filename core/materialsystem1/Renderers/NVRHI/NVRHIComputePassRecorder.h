#pragma once
#include "renderers/IShaderAPI.h"

class CNVRHIComputePassRecorder : public IGPUComputePassRecorder
{
public:
	CNVRHIComputePassRecorder(nvrhi::ICommandList* cmdList, void* userData, const char* label)
		: m_rhiCommandList(cmdList)
		, m_userData(userData)
		, m_dbgName(label)
	{
	}

	void					DbgPopGroup() const;
	void					DbgPushGroup(const char* groupLabel) const;
	void					DbgAddMarker(const char* label) const;

	void*					GetUserData() const { return m_userData; }

	void					SetPipeline(IGPUComputePipeline* pipeline);
	IGPUComputePipelinePtr	GetPipeline() const { return m_pipeline; }

	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup);

	void					DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY, int32 workgroupCountZ);
	void					DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset);

	// TODO:
	// 
	// WriteTimestamp(WGPUQuerySet querySet, uint32_t queryIndex);

	void					Complete();
	IGPUCommandBufferPtr	End();

	void					CommitComputeState(nvrhi::IBuffer* indirectBuffer = nullptr);

	IGPUBindGroupPtr			m_bindings[MAX_BINDGROUPS];
	IGPUComputePipelinePtr		m_pipeline;

	nvrhi::CommandListHandle	m_rhiCommandList{ nullptr };
	EqString					m_dbgName;
	void*						m_userData{ nullptr };

	bool						m_computeStateDirty{ true };
};