#pragma once
#include "renderers/IShaderAPI.h"
#include "ResourcePool.h"

class CNVRHIComputePassRecorder : public IGPUComputePassRecorder
{
public:
	DECLARE_RENDER_RESOURCE(CNVRHIComputePassRecorder);

	CNVRHIComputePassRecorder(nvrhi::ICommandList* cmdList, int cmdListIdx, void* userData, const char* label);
	~CNVRHIComputePassRecorder() = default;

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
	IGPUBuffer*					m_lastIndirectBuffer{ nullptr };
	EqString					m_dbgName;
	void*						m_userData{ nullptr };
	int							m_cmdListIdx{ -1 };

	bool						m_computeStateDirty{ true };
};