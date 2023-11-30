#pragma once
#include "renderers/IShaderAPI.h"

class CWGPUComputePassRecorder : public IGPUComputePassRecorder
{
public:
	~CWGPUComputePassRecorder();

	void*					GetUserData() const { return m_userData; }

	void					SetPipeline(IGPUComputePipeline* pipeline);
	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets);

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

	WGPUCommandEncoder		m_rhiCommandEncoder{ nullptr };
	WGPUComputePassEncoder	m_rhiComputePassEncoder{ nullptr };
	void*					m_userData{ nullptr };
};