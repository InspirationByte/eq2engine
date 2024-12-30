#pragma once
#include "renderers/IShaderAPI.h"

class CWGPUComputePassRecorder : public IGPUComputePassRecorder
{
public:
	~CWGPUComputePassRecorder();

	void					DbgPopGroup() const;
	void					DbgPushGroup(const char* groupLabel) const;
	void					DbgAddMarker(const char* label) const;

	void*					GetUserData() const { return m_userData; }

	void					SetPipeline(IGPUComputePipeline* pipeline);
	IGPUComputePipelinePtr	GetPipeline() const { return m_pipeline; }

	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets = nullptr);

	void					DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY, int32 workgroupCountZ);
	void					DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset);

	// TODO:
	// 
	// InsertDebugMarker(char const* markerLabel);
	// PopDebugGroup(WGPUComputePassEncoder computePassEncoder);
	// PushDebugGroup(char const* groupLabel);

	// WriteTimestamp(WGPUQuerySet querySet, uint32_t queryIndex);

	void					Complete();
	IGPUCommandBufferPtr	End();

	IGPUComputePipelinePtr	m_pipeline;
	WGPUCommandEncoder		m_rhiCommandEncoder{ nullptr };
	WGPUComputePassEncoder	m_rhiComputePassEncoder{ nullptr };
	void*					m_userData{ nullptr };
};