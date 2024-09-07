//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GRIM – GPU-driven Rendering and Instance Manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"

#include "ds/SlottedArray.h"

#include "studio/StudioGeom.h"
#include "studio/StudioCache.h"
#include "render/IDebugOverlay.h"
#include "GrimBaseRenderer.h"
#include "GrimInstanceAllocator.h"
#include "materialsystem1/IMaterialSystem.h"

DECLARE_CVAR(grim_force_software, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR(grim_stats, "0", nullptr, CV_CHEAT);

static constexpr int MAX_INSTANCE_LODS = 8;

static constexpr char SHADERNAME_SORT_INSTANCES[] = "InstanceArchetypeSort";
static constexpr char SHADERNAME_CALC_INSTANCE_BOUNDS[] = "InstanceCalcBounds";
static constexpr char SHADERNAME_PREPARE_INDIRECT_INSTANCES[] = "InstancePrepareDrawIndirect";
static constexpr char SHADERNAME_CULL_INSTANCES[] = "InstancesCull";

static constexpr char SHADER_PIPELINE_SORT_INSTANCES[] = "InstanceInfos";

GRIMBaseRenderer::GRIMBaseRenderer(GRIMBaseInstanceAllocator& allocator)
	: m_instAllocator(allocator)
{
}

void GRIMBaseRenderer::Init()
{
	m_sortShader = CRefPtr_new(ComputeSortShader);
	m_sortShader->AddSortPipeline(SHADER_PIPELINE_SORT_INSTANCES, SHADERNAME_SORT_INSTANCES);
	
	m_drawBatchsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUIndexedBatch), m_drawBatchs.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawBatchs");
	m_drawLodInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPULodInfo), m_drawLodInfos.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodInfos");
	m_drawLodsListBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(m_drawLodsList), m_drawLodsList.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodsList");
	
	m_drawBatchsBuffer->Update(m_drawBatchs.ptr(), m_drawBatchs.numSlots() * sizeof(m_drawBatchs[0]), 0);
	m_drawLodInfosBuffer->Update(m_drawLodInfos.ptr(), m_drawLodInfos.numSlots() * sizeof(m_drawLodInfos[0]), 0);
	m_drawLodsListBuffer->Update(m_drawLodsList.ptr(), m_drawLodsList.numSlots() * sizeof(m_drawLodsList[0]), 0);

	m_instCalcBoundsPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_CALC_INSTANCE_BOUNDS)
		.End()
	);

	m_instPrepareDrawIndirect = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_PREPARE_INDIRECT_INSTANCES)
		.End()
	);

	m_cullInstancesPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_CULL_INSTANCES)
		.End()
	);

	m_updateBindGroup0 = g_renderAPI->CreateBindGroup(m_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, m_drawBatchsBuffer)
		.Buffer(1, m_drawLodInfosBuffer)
		.Buffer(2, m_drawLodsListBuffer)
		.End()
	);

	m_cullBindGroup0 = g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, m_drawLodInfosBuffer)
		.Buffer(1, m_drawLodsListBuffer)
		.End()
	);
}

void GRIMBaseRenderer::Shutdown()
{
	m_drawBatchs.clear(true);
	m_drawInfos.clear(true);
	m_drawLodInfos.clear(true);
	m_drawLodsList.clear(true);
	
	m_drawBatchsBuffer = nullptr;
	m_drawLodInfosBuffer = nullptr;
	m_drawLodsListBuffer = nullptr;
	m_sortShader = nullptr;
	m_instCalcBoundsPipeline = nullptr;
	m_instPrepareDrawIndirect = nullptr;
	m_updateBindGroup0 = nullptr;
	
	m_cullInstancesPipeline = nullptr;
	m_cullBindGroup0 = nullptr;
}

GRIMArchetype GRIMBaseRenderer::CreateDrawArchetypeEGF(const CEqStudioGeom& geom, IVertexFormat* vertFormat, uint bodyGroupFlags, int materialGroupIdx)
{
	ASSERT(bodyGroupFlags != 0);

	IGPUBufferPtr buffer0 = geom.GetVertexBuffer(EGFHwVertex::VERT_POS_UV);
	IGPUBufferPtr buffer1 = geom.GetVertexBuffer(EGFHwVertex::VERT_TBN); 
	IGPUBufferPtr buffer2 = geom.GetVertexBuffer(EGFHwVertex::VERT_BONEWEIGHT);
	IGPUBufferPtr buffer3 = geom.GetVertexBuffer(EGFHwVertex::VERT_COLOR);
	IGPUBufferPtr indexBuffer = geom.GetIndexBuffer();

	// TODO: multiple material groups require new archetype
	// also body groups are really are different archetypes for EGF
	ArrayCRef<IMaterialPtr> materials = geom.GetMaterials(materialGroupIdx);
	ArrayCRef<CEqStudioGeom::HWGeomRef> geomRefs = geom.GetHwGeomRefs();
	const studioHdr_t& studio = geom.GetStudioHdr();

	GPULodList lodList;
	int prevLod = -1;
	for (int i = 0; i < studio.numLodParams; i++)
	{
		const studioLodParams_t* lodParam = studio.pLodParams(i);
		GPULodInfo lodInfo;
		lodInfo.distance = lodParam->distance;

		for (int j = 0; j < studio.numBodyGroups; ++j)
		{
			if (!(bodyGroupFlags & (1 << j)))
				continue;

			const int bodyGroupLodIndex = studio.pBodyGroups(j)->lodModelIndex;
			const studioLodModel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);
	
			int bodyGroupLOD = i;
			uint8 modelDescId = EGF_INVALID_IDX;
			do
			{
				modelDescId = lodModel->modelsIndexes[bodyGroupLOD];
				bodyGroupLOD--;
			} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);
	
			if (modelDescId == EGF_INVALID_IDX)
				continue;
	
			int prevBatch = -1;
			const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);
			for (int k = 0; k < modDesc->numMeshes; ++k)
			{
				const CEqStudioGeom::HWGeomRef::MeshRef& meshRef = geomRefs[modelDescId].meshRefs[k];
	
				GPUIndexedBatch drawBatch;
				drawBatch.firstIndex = meshRef.firstIndex;
				drawBatch.indexCount = meshRef.indexCount;

				const int newBatch = m_drawBatchs.add(drawBatch);
				if (prevBatch != -1)
					m_drawBatchs[prevBatch].next = newBatch;
				else
					lodInfo.firstBatch = newBatch;
				prevBatch = newBatch;

				GPUDrawInfo drawInfo;
				drawInfo.primTopology = (EPrimTopology)meshRef.primType;
				drawInfo.indexFormat = (EIndexFormat)geom.GetIndexFormat();
				drawInfo.meshInstFormat.name = vertFormat->GetName();
				drawInfo.meshInstFormat.formatId = vertFormat->GetNameHash();
				drawInfo.meshInstFormat.layout = vertFormat->GetFormatDesc();

				// TODO: load vertex buffers according to layout
				drawInfo.vertexBuffers[0] = buffer0;
				drawInfo.vertexBuffers[1] = buffer1;
				drawInfo.vertexBuffers[2] = buffer2;
				drawInfo.vertexBuffers[3] = buffer3;
				drawInfo.indexBuffer = indexBuffer;
				drawInfo.material = materials[meshRef.materialIdx];
				drawInfo.batchIdx = newBatch;

				m_drawBatchs[newBatch].cmdIdx = m_drawInfos.add(drawInfo);
			}
		}

		const int newLod = m_drawLodInfos.add(lodInfo);
		if(prevLod != -1)
			m_drawLodInfos[prevLod].next = newLod;
		else
			lodList.firstLodInfo = newLod;
		prevLod = newLod;
	}

	const GRIMArchetype archetypeId = m_drawLodsList.add(lodList);

	// what we are syncing to the GPU:
	// s_drawBatchs
	// s_drawLods
	// s_drawLodsList
	// s_drawArchetypeMaterials

	return archetypeId;
}

GRIMArchetype GRIMBaseRenderer::CreateDrawArchetype(const GRIMArchetypeCreateDesc& desc)
{
	return GRIM_INVALID_ARCHETYPE;
}

void GRIMBaseRenderer::DestroyDrawArchetype(GRIMArchetype id)
{

}

//--------------------------------------------------------------------

void GRIMBaseRenderer::SortInstances_Compute(IntermediateState& intermediate)
{
	PROF_EVENT_F();
	const int maxInstancesCount = m_instAllocator.GetInstanceSlotsCount();
	m_sortShader->InitKeys(intermediate.cmdRecorder, intermediate.sortedInstanceIds, maxInstancesCount);
	m_sortShader->SortKeys(StringToHashConst(SHADER_PIPELINE_SORT_INSTANCES), intermediate.cmdRecorder, intermediate.sortedInstanceIds, maxInstancesCount, intermediate.instanceInfosBuffer);
}

constexpr int GPUBOUNDS_GROUP_SIZE = 256;
constexpr int GPUBOUNDS_MAX_DIM_GROUPS = 1024;
constexpr int GPUBOUNDS_MAX_DIM_THREADS = (GPUBOUNDS_GROUP_SIZE * GPUBOUNDS_MAX_DIM_GROUPS);

static void CalcBoundsWorkSize(int length, int& x, int& y, int& z)
{
	if (length <= GPUBOUNDS_MAX_DIM_THREADS)
	{
		x = (length - 1) / GPUBOUNDS_GROUP_SIZE + 1;
		y = z = 1;
	}
	else
	{
		x = GPUBOUNDS_MAX_DIM_GROUPS;
		y = (length - 1) / GPUBOUNDS_MAX_DIM_THREADS + 1;
		z = 1;
	}
}

void GRIMBaseRenderer::UpdateInstanceBounds_Compute(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	IGPUComputePassRecorderPtr computeRecorder = intermediate.cmdRecorder->BeginComputePass("CalcInstanceBounds");
	computeRecorder->SetPipeline(m_instCalcBoundsPipeline);
	computeRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(m_instCalcBoundsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, intermediate.instanceInfosBuffer)
		.Buffer(1, intermediate.sortedInstanceIds)
		.End())
	);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(m_instCalcBoundsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, intermediate.drawInstanceBoundsBuffer)
		.Buffer(1, intermediate.renderState.instanceIdsBuffer)
		.End())
	);

	// TODO: DispatchWorkgroupsIndirect (use as result from VisibilityCullInstances)
	int x, y, z;
	CalcBoundsWorkSize(intermediate.maxNumberOfObjects, x, y ,z);
	computeRecorder->DispatchWorkgroups(x, y, z);
	computeRecorder->Complete();
}

void GRIMBaseRenderer::UpdateIndirectInstances_Compute(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	IGPUComputePassRecorderPtr computeRecorder = intermediate.cmdRecorder->BeginComputePass("UpdateIndirectInstances");
	computeRecorder->SetPipeline(m_instPrepareDrawIndirect);
	computeRecorder->SetBindGroup(0, m_updateBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(m_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, intermediate.drawInstanceBoundsBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(m_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, intermediate.renderState.drawInvocationsBuffer)
		.End())
	);

	constexpr int GROUP_SIZE = 32;
	const int numBounds = m_drawLodsList.numSlots() * MAX_INSTANCE_LODS;
	computeRecorder->DispatchWorkgroups(numBounds / GROUP_SIZE + 1);
	computeRecorder->Complete();
}

//--------------------------------------------------------------------

void GRIMBaseRenderer::SortInstances_Software(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	ArrayRef<GPUInstanceInfo> instanceInfos = intermediate.instanceInfos;

	// COMPUTE SHADER REFERENCE: SortInstanceArchetypes
	// Input / Output:
	//		instanceInfos		: buffer<GPUInstanceInfo[]>

	// sort instances by archetype
	// as we need them to be linear for each draw call
	arraySort(instanceInfos, [](const GPUInstanceInfo& a, const GPUInstanceInfo& b) {
		return a.packedArchetypeId - b.packedArchetypeId;
	});
}

void GRIMBaseRenderer::UpdateInstanceBounds_Software(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	ArrayCRef<GPUInstanceInfo> instanceInfos = intermediate.instanceInfos;
	IGPUCommandRecorder* cmdRecorder = intermediate.cmdRecorder;
	IGPUBufferPtr instanceIdsBuffer = intermediate.renderState.instanceIdsBuffer;
	Array<GPUInstanceBound>& drawInstanceBounds = intermediate.drawInstanceBounds;

	const int instanceCount = instanceInfos.numElem();

	// COMPUTE SHADER REFERENCE: VisibilityCullInstances
	// Input:
	//		instanceInfos			: buffer<GPUInstanceInfo>
	// Output:
	//		instanceIds				: buffer<int[]>
	//		drawInstanceBounds		: buffer<GPUInstanceBound[]>

	Array<int> instanceIds(PP_SL);
	instanceIds.setNum(instanceCount);

	drawInstanceBounds.setNum(m_drawLodsList.numElem() * MAX_INSTANCE_LODS);
	if (instanceCount > 0)
	{
		const int lastArchetypeId = instanceInfos[instanceCount - 1].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
		const int lastArchLodIndex = (instanceInfos[instanceCount - 1].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
		const int lastBoundIdx = lastArchetypeId * MAX_INSTANCE_LODS + lastArchLodIndex;

		drawInstanceBounds[lastBoundIdx].last = instanceCount;
	}

	for (int i = 0; i < instanceCount; ++i)
	{
		instanceIds[i] = instanceInfos[i].instanceId;

		if (i == 0 || instanceInfos[i].packedArchetypeId > instanceInfos[i - 1].packedArchetypeId)
		{
			const int archetypeId = instanceInfos[i].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
			const int archLodIndex = (instanceInfos[i].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
			const int boundIdx = archetypeId * MAX_INSTANCE_LODS + archLodIndex;

			drawInstanceBounds[boundIdx].first = i;
			drawInstanceBounds[boundIdx].archIdx = archetypeId;
			drawInstanceBounds[boundIdx].lodIndex = archLodIndex;
			if (i > 0)
			{
				const int lastArchetypeId = instanceInfos[i - 1].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
				const int lastArchLodIndex = (instanceInfos[i - 1].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
				const int lastBoundIdx = lastArchetypeId * MAX_INSTANCE_LODS + lastArchLodIndex;

				drawInstanceBounds[lastBoundIdx].last = i;
			}
		}
	}
	cmdRecorder->WriteBuffer(instanceIdsBuffer, instanceIds.ptr(), sizeof(instanceIds[0]) * instanceIds.numElem(), 0);
}

void GRIMBaseRenderer::UpdateIndirectInstances_Software(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	ArrayCRef<GPUInstanceBound> drawInstanceBounds = intermediate.drawInstanceBounds;
	IGPUCommandRecorder* cmdRecorder = intermediate.cmdRecorder;
	IGPUBufferPtr drawInvocationsBuffer = intermediate.renderState.drawInvocationsBuffer;

	// COMPUTE SHADER REFERENCE: PrepareDrawIndirectInstances
	// Input:
	//		instanceInfos		: buffer<GPUInstanceInfo[]>
	//		drawInstanceBounds	: buffer<GPUInstanceBound[]>
	// Output:
	//		indirectDraws		: buffer<GPUDrawIndexedIndirectCmd[]>
	//
	// Notes:
	//		instanceInfos must be sorted by archetype id prior to executing

	for (const GPUInstanceBound& bound : drawInstanceBounds)
	{
		if (bound.archIdx == -1)
			continue;

		const GPULodList& lodList = m_drawLodsList[bound.archIdx];

		// fill lod list
		int numLods = 0;
		int lodInfos[MAX_INSTANCE_LODS];
		for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = m_drawLodInfos[lodIdx].next)
			lodInfos[numLods++] = lodIdx;

		// walk over batches
		const int lodIdx = min(numLods - 1, bound.lodIndex);

		const GPULodInfo& lodInfo = m_drawLodInfos[lodInfos[lodIdx]];
		for (int batchIdx = lodInfo.firstBatch; batchIdx != -1; batchIdx = m_drawBatchs[batchIdx].next)
		{
			const GPUIndexedBatch& drawBatch = m_drawBatchs[batchIdx];

			GPUDrawIndexedIndirectCmd drawCmd;
			drawCmd.firstIndex = drawBatch.firstIndex;
			drawCmd.indexCount = drawBatch.indexCount;
			drawCmd.firstInstance = bound.first;
			drawCmd.instanceCount = bound.last - bound.first;

			ASSERT(m_drawInfos(drawBatch.cmdIdx));

			cmdRecorder->WriteBuffer(drawInvocationsBuffer, &drawCmd, sizeof(drawCmd), sizeof(GPUDrawIndexedIndirectCmd) * drawBatch.cmdIdx);
		}
	}
}

static float memBytesToKB(size_t byteCnt)
{
	return byteCnt / 1024.0f;
}

void GRIMBaseRenderer::PrepareDraw(IGPUCommandRecorder* cmdRecorder, GRIMRenderState& renderState, int maxNumberOfObjects)
{
	PROF_EVENT_F();

	renderState.drawInvocationsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUDrawIndexedIndirectCmd), m_drawInfos.numSlots()), BUFFERUSAGE_INDIRECT | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawInvocations");
	renderState.instanceIdsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), maxNumberOfObjects), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceIds");

	IntermediateState intermediate;
	intermediate.renderState = renderState;
	intermediate.cmdRecorder.Assign(cmdRecorder);	// FIXME: create new and return cmd buffer only?
	intermediate.maxNumberOfObjects = maxNumberOfObjects;

	cmdRecorder->ClearBuffer(intermediate.renderState.drawInvocationsBuffer, 0, intermediate.renderState.drawInvocationsBuffer->GetSize());

	if (grim_force_software.GetBool())
	{
		VisibilityCullInstances_Software(intermediate);
		SortInstances_Software(intermediate);
		UpdateInstanceBounds_Software(intermediate);
		UpdateIndirectInstances_Software(intermediate);
		return;
	}

	cmdRecorder->DbgPushGroup("GRIMPrepareDraw");

	const int numBounds = m_drawLodsList.numSlots() * MAX_INSTANCE_LODS;
	intermediate.sortedInstanceIds = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), intermediate.maxNumberOfObjects + 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "SortedKeys");
	intermediate.instanceInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUInstanceInfo), intermediate.maxNumberOfObjects), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceInfos");
	intermediate.drawInstanceBoundsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUInstanceBound), numBounds + 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceBounds");

	cmdRecorder->ClearBuffer(intermediate.drawInstanceBoundsBuffer, 0, intermediate.drawInstanceBoundsBuffer->GetSize());
	cmdRecorder->WriteBuffer(intermediate.drawInstanceBoundsBuffer, &numBounds, sizeof(int), 0);

	cmdRecorder->DbgPushGroup("CullInstances");
	VisibilityCullInstances_Compute(intermediate);
	cmdRecorder->DbgPopGroup();

	cmdRecorder->DbgPushGroup("SortInstances");
	SortInstances_Compute(intermediate);
	cmdRecorder->DbgPopGroup();

	cmdRecorder->DbgPushGroup("CalcBounds");
	UpdateInstanceBounds_Compute(intermediate);
	cmdRecorder->DbgPopGroup();

	cmdRecorder->DbgPushGroup("UpdateIndirect");
	UpdateIndirectInstances_Compute(intermediate);
	cmdRecorder->DbgPopGroup();

	cmdRecorder->DbgPopGroup();
}

void GRIMBaseRenderer::Draw(const GRIMRenderState& renderState, const RenderPassContext& renderPassCtx)
{
	PROF_EVENT_F();

	renderPassCtx.recorder->DbgPushGroup("GRIMDraw");
	int numDrawCalls = 0;
	for (int i = 0; i < m_drawInfos.numSlots(); ++i)
	{
		if (!m_drawInfos(i))
		{
			debugoverlay->Text(color_white, " draw info %d not drawn", i);
			continue;
		}

		const GPUDrawInfo& drawInfo = m_drawInfos[i];

		IMaterial* material = drawInfo.material;
		if (g_matSystem->SetupMaterialPipeline(material, nullptr, drawInfo.primTopology, drawInfo.meshInstFormat, renderPassCtx, this))
		{
			renderPassCtx.recorder->SetVertexBuffer(0, drawInfo.vertexBuffers[0]);
			renderPassCtx.recorder->SetVertexBuffer(1, renderState.instanceIdsBuffer);
			renderPassCtx.recorder->SetIndexBuffer(drawInfo.indexBuffer, drawInfo.indexFormat);

			renderPassCtx.recorder->DrawIndexedIndirect(renderState.drawInvocationsBuffer, sizeof(GPUDrawIndexedIndirectCmd) * m_drawBatchs[drawInfo.batchIdx].cmdIdx);
			++numDrawCalls;
		}
	}
	renderPassCtx.recorder->DbgPopGroup();

	if(grim_stats.GetBool())
	{
		debugoverlay->Text(color_white, "--- GRIM instances summary ---");
		debugoverlay->Text(color_white, "Mode: %s", grim_force_software.GetBool() ? "CPU" : "Compute");
		debugoverlay->Text(color_white, " %d draw infos: %.2f KB", m_drawInfos.numElem(), memBytesToKB(m_drawInfos.numSlots() * sizeof(m_drawInfos[0])));
		debugoverlay->Text(color_white, " %d batchs: %.2f KB", m_drawBatchs.numElem(), memBytesToKB(m_drawBatchs.numSlots() * sizeof(m_drawBatchs[0])));
		debugoverlay->Text(color_white, " %d lod infos: %.2f KB", m_drawLodInfos.numElem(), memBytesToKB(m_drawLodInfos.numSlots() * sizeof(m_drawLodInfos[0])));
		debugoverlay->Text(color_white, " %d lod lists: %.2f KB", m_drawLodsList.numElem(), memBytesToKB(m_drawLodsList.numSlots() * sizeof(m_drawLodsList[0])));

		debugoverlay->Text(color_white, "Draw calls: %d", numDrawCalls);
	}
}