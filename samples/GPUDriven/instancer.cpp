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
			.Buffer(StringIdConst24("drawInstanceBounds"), 2, SHADERKIND_COMPUTE, BUFFERBIND_STORAGE)
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

	intermediate.cmdRecorder->ClearBuffer(renderState.culledInstanceCountBuffer, 0, sizeof(int));

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
		.Buffer(StringIdConst24("culledInstanceCount"), renderState.culledInstanceCountBuffer)
		.Buffer(StringIdConst24("drawInstanceBounds"), renderState.drawInstanceBoundsBuffer)
		.End())
	);

	GetInstancesBindGroup(3, m_cullInstancesBindingLayout, renderState.instBindGroup, renderState.instBindGroupUpdateToken);
	computeRecorder->SetBindGroup(3, renderState.instBindGroup);

	const int instanceCount = m_instAllocator.GetInstanceCount();
	const IVector2D workGroups = VisCalcWorkSize(instanceCount);

	computeRecorder->DispatchWorkgroups(workGroups.x, workGroups.y);
	computeRecorder->Complete();
}

bool DemoGRIMRenderer::CullInstance_Software(const GRIMRenderState& renderState, const GPUInstanceInfo& instInfo, float& outViewDistanceSqr) const
{
	const DemoGRIMInstanceAllocator& instAlloc = DemoGRIMRenderer::GetAllocator();
	const DemoRenderState& demoRenderState = static_cast<const DemoRenderState&>(renderState);

	const Vector3D& viewPos = demoRenderState.viewPos;
	const Volume& frustum = demoRenderState.frustum;

	const int trsIdx = instAlloc.GetInstanceComponentIdx(instInfo.instanceId, InstTransform::COMPONENT_ID);
	const InstTransform& trs = instAlloc.GetComponentPool<InstTransform>().GetDataPool()[trsIdx];

	if (!frustum.IsSphereInside(trs.position, trs.boundingSphere))
		return false;

	outViewDistanceSqr = distanceSqr(viewPos, trs.position);

	return true;
}

void DemoGRIMRenderer::VisibilityCullInstances_Software(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	GRIMRenderState& renderState = intermediate.renderState;
	Array<GPUInstanceInfo>& instanceInfos = intermediate.instanceInfos;
	ArrayRef<GPUInstanceBound> drawInstanceBounds = intermediate.drawInstanceBounds;

	// compute potentially visible archetypes and store states as bitarray
	renderState.visibleArchetypes.reset();

	for (int i = 0; i < instanceInfos.numElem(); ++i)
	{
		GPUInstanceInfo& instInfo = instanceInfos[i];

		const GRIMArchetype archetypeId = instInfo.packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
		const GPULodList& lodList = m_drawLodsList[archetypeId];
		if (lodList.firstLodInfo < 0)
		{
			instanceInfos.fastRemoveIndex(i--);
			continue;
		}

		float distFromCameraSqr = F_UNDEF;
		if(!CullInstance_Software(renderState, instanceInfos[i], distFromCameraSqr))
		{
			instanceInfos.fastRemoveIndex(i--);
			continue;
		}

		// find suitable lod idx
		int lodIndex = (instInfo.packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
		lodIndex = (lodIndex == GPUInstanceInfo::LOD_MASK) ? -1 : lodIndex;
		if (lodIndex == -1)
		{
			int lodIdx = lodList.firstLodInfo;
			for (int i = 0; i < GRIM_MAX_INSTANCE_LODS; ++i)
			{
				if (lodIdx == -1 || distFromCameraSqr < sqr(m_drawLodInfos[lodIdx].distance))
					break;

				++lodIndex;
				lodIdx = m_drawLodInfos[lodIdx].next;
			}
		}

		// update instance
		instInfo.packedArchetypeId = archetypeId | (lodIndex << GPUInstanceInfo::ARCHETYPE_BITS);

		// account instance in bound
		const int boundIdx = archetypeId * GRIM_MAX_INSTANCE_LODS + lodIndex;
		++drawInstanceBounds[boundIdx].last;

		// set visible
		renderState.visibleArchetypes.setTrue(archetypeId);
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