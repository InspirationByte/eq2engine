#pragma once

#include "grim/GrimInstanceAllocator.h"
#include "grim/GrimBaseRenderer.h"

enum EGPUInstanceComponentId : int
{
	GPUINST_PROP_ID_TRANSFORM = 0,
	GPUINST_PROP_ID_SCALE,
};

struct InstTransform
{
	DEFINE_GPU_INSTANCE_COMPONENT(GPUINST_PROP_ID_TRANSFORM, InstTransform);

	Quaternion	orientation{ qidentity };
	Vector3D	position{ vec3_zero };
	float		boundingSphere{ 1.0f };
};

using DemoGRIMInstanceAllocator = GRIMInstanceAllocator<InstTransform>;

class DemoGRIMRenderer : public GRIMBaseRenderer
{
public:
	DemoGRIMRenderer(DemoGRIMInstanceAllocator& instAlloc);

	void	FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const;
	void	GetInstancesBindGroup(int bindGroupIdx, IGPUPipelineLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const;

	void	VisibilityCullInstances_Compute(IntermediateState& intermediate);
	void	VisibilityCullInstances_Software(IntermediateState& intermediate);
};

const VertexLayoutDesc& GetGPUInstanceVertexLayout();