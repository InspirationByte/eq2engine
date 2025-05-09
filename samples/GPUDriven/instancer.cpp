#include <imgui.h>

#include "core/core_common.h"
#include "instancer.h"

DEFINE_SHADER_NOFACTORY(InstanceUtils)
INIT_GPU_INSTANCE_COMPONENT(InstTransform, "InstanceUtils")
INIT_GPU_INSTANCE_COMPONENT(InstScale, "InstanceUtils")

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
		.Buffer("InstTransform", 1, SHADERKIND_VERTEX, BUFFERBIND_STORAGE_READONLY)
		.Buffer("InstScale", 2, SHADERKIND_VERTEX, BUFFERBIND_STORAGE_READONLY);
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
		.Buffer(1, static_cast<DemoGRIMInstanceAllocator&>(m_instAllocator).GetComponentPool<InstTransform>().GetBuffer())
		.Buffer(2, static_cast<DemoGRIMInstanceAllocator&>(m_instAllocator).GetComponentPool<InstScale>().GetBuffer())
		.End();
	outBindGroup = g_renderAPI->CreateBindGroup(pipelineLayout, bindGroupDesc);
}

void DemoGRIMRenderer::VisibilityCullInstances_Compute(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	DemoRenderState& renderState = static_cast<DemoRenderState&>(intermediate.renderState);

	struct CullViewParams
	{
		Vector4D	frustumPlanes[6];
		Vector4D	viewPos;
	};

	CullViewParams cullView;
	memcpy(cullView.frustumPlanes, renderState.frustum.GetPlanes().ptr(), sizeof(cullView.frustumPlanes));
	cullView.viewPos = Vector4D(renderState.viewPos, 1.0f);

	IGPUBufferPtr viewParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(CullViewParams), 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "ViewParamsBuffer");
	intermediate.cmdRecorder->WriteBuffer(viewParamsBuffer, &cullView, sizeof(cullView), 0);
	intermediate.cmdRecorder->ClearBuffer(renderState.sortedInstanceIdsBuffer, 0, sizeof(int));

	IGPUComputePassRecorderPtr computeRecorder = intermediate.cmdRecorder->BeginComputePass("CullInstances");
	computeRecorder->SetPipeline(m_cullInstancesPipeline);
	computeRecorder->SetBindGroup(0, m_cullBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, viewParamsBuffer)
		.Buffer(1, intermediate.filteredInstanceInfosBuffer)
		.Buffer(2, intermediate.filteredInstanceCountBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, renderState.culledInstanceInfosBuffer)
		.Buffer(1, renderState.sortedInstanceIdsBuffer)
		.End())
	);

	computeRecorder->SetBindGroup(3, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline, Builder<BindGroupDesc>()
		.GroupIndex(3)
		.Buffer(0, m_instAllocator.GetRootBuffer())
		.Buffer(1, DemoGRIMRenderer::GetAllocator().GetComponentPool<InstTransform>().GetBuffer())
		.End())
	);

	IVector2D workGroups = VisCalcWorkSize(intermediate.maxNumberOfObjects);

	computeRecorder->DispatchWorkgroups(workGroups.x, workGroups.y);
	computeRecorder->Complete();
}

void DemoGRIMRenderer::VisibilityCullInstances_Software(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	// COMPUTE SHADER REFERENCE: VisibilityCullInstances
	// Input:
	//		instanceIds		: buffer<int[]>
	// Output:
	//		instanceInfos	: buffer<GPUInstanceInfo[]>

	DemoRenderState& renderState = static_cast<DemoRenderState&>(intermediate.renderState);

	const Vector3D& viewPos = renderState.viewPos;
	const Volume& frustum = renderState.frustum;
	Array<GPUInstanceInfo>& instanceInfos = intermediate.instanceInfos;

	// compute potentially visible archetypes and store states as bitarray
	renderState.visibleArchetypes.reset();

	for (int i = 0; i < instanceInfos.numElem(); ++i)
	{
		GPUInstanceInfo& instInfo = instanceInfos[i];

		const GRIMArchetype archetypeId = instInfo.packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
		const int lodIndex = (instInfo.packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;

		const GPULodList& lodList = m_drawLodsList[archetypeId];
		if (lodList.firstLodInfo < 0)
		{
			instanceInfos.fastRemoveIndex(i--);
			continue;
		}

		const int trsIdx = DemoGRIMRenderer::GetAllocator().GetInstanceComponentIdx(instInfo.instanceId, InstTransform::COMPONENT_ID);
		const InstTransform& trs = DemoGRIMRenderer::GetAllocator().GetComponentPool<InstTransform>().GetDataPool()[trsIdx];

		if (!frustum.IsSphereInside(trs.position, trs.boundingSphere))
		{
			instanceInfos.fastRemoveIndex(i--);
			continue;
		}

		renderState.visibleArchetypes.setTrue(archetypeId);

		const float distFromCamera = distanceSqr(viewPos, trs.position);

		// find suitable lod idx
		int drawLod = lodIndex;
		if (drawLod == GPUInstanceInfo::LOD_MASK)
		{
			drawLod = -1;
			for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = m_drawLodInfos[lodIdx].next, ++drawLod)
			{
				if (distFromCamera < sqr(m_drawLodInfos[lodIdx].distance))
					break;
			}
		}

		// update instance
		instInfo.packedArchetypeId = archetypeId | (drawLod << GPUInstanceInfo::ARCHETYPE_BITS);
	}
}

static DemoGRIMInstanceAllocator s_instanceAlloc;
static DemoGRIMRenderer s_grimRenderer(s_instanceAlloc);

DemoGRIMInstanceAllocator& DemoGRIMRenderer::GetAllocator()
{
	return s_instanceAlloc;
}

DemoGRIMRenderer& DemoGRIMRenderer::Get()
{
	return s_grimRenderer;
}


void DemoInstManagerDebugDrawUI(bool& open)
{
#ifdef IMGUI_ENABLED
	ImGui::SetNextWindowSize(ImVec2(512, 256), ImGuiCond_FirstUseEver);
	if (open && ImGui::Begin("GRIM Debug", &open))
	{
		GRIMInstanceDebug::DrawUI(s_grimRenderer);
		ImGui::End();
	}
#endif // IMGUI_ENABLED
}