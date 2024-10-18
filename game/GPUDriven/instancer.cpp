#include "core/core_common.h"
#include "instancer.h"

INIT_GPU_INSTANCE_COMPONENT(InstTransform, "InstanceUtils")

const VertexLayoutDesc& GetGPUInstanceVertexLayout()
{
	static const VertexLayoutDesc s_gpuInstIdVertexLayoutDesc = Builder<VertexLayoutDesc>()
		.Stride(sizeof(int))
		.StepMode(VERTEX_STEPMODE_INSTANCE)
		.UserId(StringIdConst24("GPUInstanceID"))
		.Attribute(VERTEXATTRIB_TEXCOORD, "id", 8, 0, ATTRIBUTEFORMAT_UINT8, 4)
		.End();

	return s_gpuInstIdVertexLayoutDesc;
}

DemoGRIMRenderer::DemoGRIMRenderer(DemoGRIMInstanceAllocator& instAlloc)
	: GRIMBaseRenderer(instAlloc)
{
}

void DemoGRIMRenderer::FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const
{
	Builder<BindGroupLayoutDesc>(bindGroupLayout)
		.Buffer("InstRoot", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_STORAGE_READONLY)
		.Buffer("InstTransform", 1, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_STORAGE_READONLY);
}

void DemoGRIMRenderer::GetInstancesBindGroup(int bindGroupIdx, IGPUPipelineLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const
{
	const uint updateToken = m_instAllocator.GetBufferUpdateToken();
	if (outBindGroup && lastUpdateToken == updateToken)
		return;

	lastUpdateToken = updateToken;
	BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
		.GroupIndex(bindGroupIdx)
		.Buffer(0, m_instAllocator.GetRootBuffer())
		.Buffer(1, m_instAllocator.GetDataPoolBuffer(InstTransform::COMPONENT_ID))
		.End();
	outBindGroup = g_renderAPI->CreateBindGroup(pipelineLayout, bindGroupDesc);
}