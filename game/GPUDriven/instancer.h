#pragma once

#include "grim/GrimInstanceAllocator.h"
#include "materialsystem1/IMatSysShader.h"

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

class DemoGRIMInstanceAllocator
	: public GRIMInstanceAllocator<InstTransform>
	, public IShaderMeshInstanceProvider
{
public:
	void	FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const;
	void	GetInstancesBindGroup(int bindGroupIdx, IGPUPipelineLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const;
};

const VertexLayoutDesc& GetGPUInstanceVertexLayout();