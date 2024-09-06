#include "core/core_common.h"
#include "instancer.h"

INIT_GPU_INSTANCE_COMPONENT(InstTransform, "InstanceUtils")

const VertexLayoutDesc& GetGPUInstanceVertexLayout()
{
	static const VertexLayoutDesc s_gpuInstIdVertexLayoutDesc = Builder<VertexLayoutDesc>()
		.Stride(sizeof(int))
		.StepMode(VERTEX_STEPMODE_INSTANCE)
		.UserId(StringToHashConst("GPUInstanceID"))
		.Attribute(VERTEXATTRIB_TEXCOORD, "id", 8, 0, ATTRIBUTEFORMAT_UINT8, 4)
		.End();

	return s_gpuInstIdVertexLayoutDesc;
}


void DemoGRIMInstanceAllocator::FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const
{
	Builder<BindGroupLayoutDesc>(bindGroupLayout)
		.Buffer("InstRoot", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_STORAGE_READONLY)
		.Buffer("InstTransform", 1, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_STORAGE_READONLY);
}

void DemoGRIMInstanceAllocator::GetInstancesBindGroup(int bindGroupIdx, IGPUPipelineLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const
{
	if (outBindGroup && lastUpdateToken == m_buffersUpdated)
		return;

	BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
		.GroupIndex(bindGroupIdx)
		.Buffer(0, GetRootBuffer())
		.Buffer(1, GetDataPoolBuffer(InstTransform::COMPONENT_ID))
		.End();
	outBindGroup = g_renderAPI->CreateBindGroup(pipelineLayout, bindGroupDesc);

	lastUpdateToken = m_buffersUpdated;
}