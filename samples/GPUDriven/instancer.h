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
	DEFINE_GPU_BUFFER_INSTANCE_COMPONENT(GPUINST_PROP_ID_TRANSFORM, InstTransform);

	Quaternion	orientation{ qidentity };
	Vector3D	position{ vec3_zero };
	float		boundingSphere{ 1.0f };
};

struct InstScale
{
	DEFINE_GPU_BUFFER_INSTANCE_COMPONENT(GPUINST_PROP_ID_SCALE, InstScale);
	Vector3D	scale{ 1.0f };
	float		pad{ 0.0f };
};

struct DemoRenderState : public GRIMRenderState
{
	Vector3D		viewPos;
	Volume			frustum;
};

using DemoGRIMInstanceAllocator = GRIMInstanceAllocator<InstTransform, InstScale>;

class DemoGRIMRenderer : public GRIMBaseRenderer
{
public:
	DemoGRIMRenderer(DemoGRIMInstanceAllocator& instAlloc);

	void	FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const;
	void	GetInstancesBindGroup(int bindGroupIdx, IGPUPipelineLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const;

	void	VisibilityCullInstances_Compute(IntermediateState& intermediate);
	void	VisibilityCullInstances_Software(IntermediateState& intermediate);

	static DemoGRIMInstanceAllocator&	GetAllocator();
	static DemoGRIMRenderer&			Get();
};

const VertexLayoutDesc& GetGPUInstanceVertexLayout();

void DemoInstManagerDebugDrawUI(bool& open);

