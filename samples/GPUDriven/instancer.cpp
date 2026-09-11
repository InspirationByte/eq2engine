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
		.Attribute(StringIdConst24("_instanceId"), 0, ATTRIBUTEFORMAT_UINT8, 4)
		.End();

	return s_gpuInstIdVertexLayoutDesc;
}

DemoGRIMRenderer::DemoGRIMRenderer(DemoGRIMInstanceAllocator& instAlloc)
	: GRIMBaseRenderer(instAlloc)
{
}

void DemoGRIMRenderer::Init()
{
	PROF_EVENT("GRIM World Renderer Init");

	BindingLayoutDesc cullLayoutDesc = Builder<BindingLayoutDesc>()
		.Group(Builder<BindGroupLayoutDesc>()
			.Buffer(StringIdConst24("drawLodInfos"), 0, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE_READONLY)
			.Buffer(StringIdConst24("drawLodsList"), 1, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE_READONLY)
			.End())
		.Group(Builder<BindGroupLayoutDesc>()
			.Buffer(StringIdConst24("viewOccluders"), 0, SHADERKIND_COMPUTE, BUFFERBIND_UNIFORM)
			.Buffer(StringIdConst24("instanceInfos"), 1, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE_READONLY)
			.Buffer(StringIdConst24("instanceInfosCount"), 2, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE_READONLY)
			.End())
		.Group(Builder<BindGroupLayoutDesc>()
			.Buffer(StringIdConst24("culledInstances"), 0, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE)
			.Buffer(StringIdConst24("culledInstanceCount"), 1, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE)
			.End())
		.End();
	FillBindGroupLayoutDesc(cullLayoutDesc.bindGroups.append());

	m_cullInstancesBindingLayout = g_renderAPI->CreateBindingLayout(cullLayoutDesc);

	GRIMBaseRenderer::Init();
}

void DemoGRIMRenderer::FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const
{
	Builder<BindGroupLayoutDesc>(bindGroupLayout)
		.Buffer(StringIdConst24("instRoots"), 0, SHADERKIND_COMPUTE | SHADERKIND_VERTEX, BUFFERBIND_STORAGE_READONLY)
		.Buffer(StringIdConst24("instTransforms"), 1, SHADERKIND_COMPUTE | SHADERKIND_VERTEX, BUFFERBIND_STORAGE_READONLY)
		.Buffer(StringIdConst24("instScales"), 2, SHADERKIND_COMPUTE | SHADERKIND_VERTEX, BUFFERBIND_STORAGE_READONLY);
}

void DemoGRIMRenderer::GetInstancesBindGroup(int bindGroupIdx, IGPUBindingLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const
{
	const uint updateToken = m_instAllocator.GetBufferUpdateToken();
	if (outBindGroup && lastUpdateToken == updateToken)
		return;

	lastUpdateToken = updateToken;
	BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
		.GroupIndex(bindGroupIdx)
		.Buffer(StringIdConst24("instRoots"), m_instAllocator.GetRootBuffer())
		.Buffer(StringIdConst24("instTransforms"), GetAllocator().GetComponentPool<InstTransform>().GetBuffer())
		.Buffer(StringIdConst24("instScales"), GetAllocator().GetComponentPool<InstScale>().GetBuffer())
		.End();

	if(pipelineLayout)
		outBindGroup = g_renderAPI->CreateSharedBindGroup(pipelineLayout, bindGroupDesc);
	else
		outBindGroup = g_renderAPI->CreateBindGroup(m_cullInstancesPipeline, bindGroupDesc);
}

void DemoGRIMRenderer::VisibilityCullInstances_Compute(IntermediateState& intermediate)
{
	CGPUScopedDbgGroup g("CullInstances", intermediate.cmdRecorder);
	PROF_EVENT_F();

	struct CullViewParams
	{
		Vector4D	frustumPlanes[6];
		Vector4D	viewPos;
	};

	DemoRenderState& renderState = static_cast<DemoRenderState&>(intermediate.renderState);

	IGPUBufferPtr viewParamsBuffer = renderState.viewParamsBuffer;
	if(!viewParamsBuffer)
	{
		viewParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(CullViewParams), 1), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "ViewParamsBuffer");
		renderState.viewParamsBuffer = viewParamsBuffer;
	}

	CullViewParams cullView;
	memcpy(cullView.frustumPlanes, renderState.frustum.GetPlanes().ptr(), sizeof(cullView.frustumPlanes));
	cullView.viewPos = Vector4D(renderState.viewPos, 1.0f);
	intermediate.cmdRecorder->WriteBuffer(viewParamsBuffer, &cullView, sizeof(cullView), 0);

	intermediate.cmdRecorder->ClearBuffer(renderState.sortedInstanceIdsBuffer, 0, sizeof(int));

	IGPUComputePassRecorderPtr computeRecorder = intermediate.cmdRecorder->BeginComputePass("CullInstances");
	computeRecorder->SetPipeline(m_cullInstancesPipeline);
	computeRecorder->SetBindGroup(0, m_cullBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(StringIdConst24("viewOccluders"), viewParamsBuffer)
		.Buffer(StringIdConst24("instanceInfos"), intermediate.filteredInstanceInfosBuffer)
		.Buffer(StringIdConst24("instanceInfosCount"), intermediate.filteredInstanceCountBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(StringIdConst24("culledInstances"), renderState.culledInstanceInfosBuffer)
		.Buffer(StringIdConst24("culledInstanceCount"), renderState.sortedInstanceIdsBuffer)
		.End())
	);

	GetInstancesBindGroup(3, m_cullInstancesBindingLayout, renderState.instBindGroup, renderState.instBindGroupUpdateToken);
	computeRecorder->SetBindGroup(3, renderState.instBindGroup);

	const int instanceCount = m_instAllocator.GetInstanceCount();
	const IVector2D workGroups = VisCalcWorkSize(instanceCount);

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
	ArrayRef<GPUInstanceBound> drawInstanceBounds = intermediate.drawInstanceBounds;

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

		// count instances and put their counts per archetypes
		const int boundIdx = archetypeId * GRIM_MAX_INSTANCE_LODS + drawLod;
		++drawInstanceBounds[boundIdx].last;
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