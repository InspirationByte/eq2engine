//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GRIM � GPU-driven Rendering and Instance Manager
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

using namespace Threading;

DECLARE_CVAR(grim_force_software, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR(grim_stats, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(grim_dbg_onlyMaterial, "", nullptr, CV_CHEAT);
DECLARE_CVAR(grim_dbg_logArchetypes, "0", nullptr, CV_CHEAT);

static constexpr char SHADERNAME_SORT_INSTANCES[] = "InstanceArchetypeSort";
static constexpr char SHADERNAME_CALC_INSTANCE_BOUNDS[] = "InstanceCalcBounds";
static constexpr char SHADERNAME_PREPARE_INDIRECT_INSTANCES[] = "InstancePrepareDrawIndirect";
static constexpr char SHADERNAME_PREPARE_INSTALCE_POOLS[] = "InstancePreparePools";
static constexpr char SHADERNAME_CULL_INSTANCES[] = "InstancesCull";
static constexpr char SHADER_PIPELINE_SORT_INSTANCES[] = "InstanceInfos";

static CEqMutex s_grimRendererMutex;

GRIMBaseRenderer::GRIMBaseRenderer(GRIMBaseInstanceAllocator& allocator)
	: m_instAllocator(allocator)
{
}

void GRIMBaseRenderer::Init()
{
	m_sortShader = CRefPtr_new(ComputeSortShader);
	m_sortShader->AddSortPipeline(SHADER_PIPELINE_SORT_INSTANCES, SHADERNAME_SORT_INSTANCES);
	
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

	m_drawBatchs.SetPipeline(g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_PREPARE_INSTALCE_POOLS)
		.ShaderLayoutId(StringToHash(m_drawBatchs.GetName()))
		.End()
	));
	m_drawLodInfos.SetPipeline(g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_PREPARE_INSTALCE_POOLS)
		.ShaderLayoutId(StringToHash(m_drawLodInfos.GetName()))
		.End()
	));
	m_drawLodsList.SetPipeline(g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_PREPARE_INSTALCE_POOLS)
		.ShaderLayoutId(StringToHash(m_drawLodsList.GetName()))
		.End()
	));
}

void GRIMBaseRenderer::Shutdown()
{
	m_pendingArchetypes.clear(true);
	m_pendingDeletion.clear(true);
	m_drawInfos.clear(true);

	m_drawBatchs.Clear(true);
	m_drawLodInfos.Clear(true);
	m_drawLodsList.Clear(true);

	m_drawBatchs.SetPipeline(nullptr);
	m_drawLodInfos.SetPipeline(nullptr);
	m_drawLodsList.SetPipeline(nullptr);

	m_sortShader = nullptr;
	m_instCalcBoundsPipeline = nullptr;
	m_instPrepareDrawIndirect = nullptr;
	m_updateBindGroup0 = nullptr;
	
	m_cullInstancesPipeline = nullptr;
	m_cullBindGroup0 = nullptr;
}

GRIMArchetype GRIMBaseRenderer::CreateStudioDrawArchetype(const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags, int materialGroupIdx, ArrayCRef<IGPUBufferPtr> extraVertexBuffers)
{
	ASSERT(bodyGroupFlags != 0);
	ASSERT(vertFormat);

	CScopedMutex m(s_grimRendererMutex);

	PendingDesc& pending = m_pendingArchetypes.append();
	pending.egfDesc.geom = geom;
	pending.egfDesc.vertFormat = vertFormat;
	pending.egfDesc.bodyGroupFlags = bodyGroupFlags;
	pending.extraVertexBuffers.append(extraVertexBuffers.ptr(), extraVertexBuffers.numElem());
	pending.type = PendingDesc::TYPE_STUDIO;
	pending.slot = m_drawLodsList.Add(GPULodList{});

	return pending.slot;
}

GRIMArchetype GRIMBaseRenderer::CreateDrawArchetype(const GRIMArchetypeDesc& desc)
{
	bool hasVertBuffers = false;
	for (IGPUBufferPtr vertBuffer : desc.vertexBuffers)
		hasVertBuffers = hasVertBuffers || vertBuffer;
	ASSERT_MSG(hasVertBuffers, "Cannot create archetype when no vertex buffers specified");
	if (!hasVertBuffers)
		return GRIM_INVALID_ARCHETYPE;

	ASSERT_MSG(desc.lods.numElem() <= GRIM_MAX_INSTANCE_LODS, "Too many lods (%d), max is %d", desc.lods.numElem(), GRIM_MAX_INSTANCE_LODS);

	CScopedMutex m(s_grimRendererMutex);

	PendingDesc& pending = m_pendingArchetypes.append();
	pending.desc = desc;
	pending.type = PendingDesc::TYPE_GRIM;
	pending.slot = m_drawLodsList.Add(GPULodList{});

	return pending.slot;
}

void GRIMBaseRenderer::InitDrawArchetype(GRIMArchetype slot, const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags, int materialGroupIdx, ArrayCRef<IGPUBufferPtr> extraVertexBuffers)
{
	ASSERT_MSG(m_drawLodsList(slot), "Archetype slot %d is not reserved", slot);

	FixedArray<IGPUBufferPtr, MAX_VERTEXSTREAM> vertexBuffers;
	IGPUBufferPtr indexBuffer = geom->GetIndexBuffer();

	MeshInstanceFormatRef instFormat = vertFormat;
	instFormat.usedLayoutBits = 0;
	int instanceStreamId = -1;

	bool skinningSupport = false;
	for (int i = 0; i < instFormat.layout.numElem(); ++i)
	{
		const VertexLayoutDesc& layoutDesc = instFormat.layout[i];

		if (layoutDesc.userId & EGFHwVertex::EGF_FLAG)
		{
			const EGFHwVertex::VertexStreamId vertStreamId = static_cast<EGFHwVertex::VertexStreamId>(layoutDesc.userId & EGFHwVertex::EGF_MASK);
			IGPUBufferPtr vertBuffer = geom->GetVertexBuffer(vertStreamId);
			if(vertBuffer)
			{
				vertexBuffers.append(vertBuffer);

				instFormat.usedLayoutBits |= (1 << i);
				if (vertStreamId == EGFHwVertex::VERT_BONEWEIGHT)
					skinningSupport = true;
			}
		}

		if (instanceStreamId == -1 && layoutDesc.stepMode == VERTEX_STEPMODE_INSTANCE)
		{
			ASSERT_MSG((instFormat.usedLayoutBits & (1 << i)) == 0, "Instance layout id is not valid");
			instFormat.usedLayoutBits |= (1 << i);

			instanceStreamId = vertexBuffers.numElem();
			vertexBuffers.append(nullptr);
		}
	}

	ASSERT_MSG(instanceStreamId != -1, "Vertex format %s is not configured for instanced rendering", vertFormat->GetName());
	vertexBuffers.append(extraVertexBuffers.ptr(), extraVertexBuffers.numElem());

	// TODO: multiple material groups require new archetype
	// also body groups are really are different archetypes for EGF
	ArrayCRef<IMaterialPtr> materials = geom->GetMaterials(materialGroupIdx);
	ArrayCRef<CEqStudioGeom::HWGeomRef> geomRefs = geom->GetHwGeomRefs();
	const studioHdr_t& studio = geom->GetStudioHdr();

	ArchetypeInfo::PTR_T archetypeInfo = CRefPtr_new(ArchetypeInfo);
	archetypeInfo->name = EqString::Format("%s_b%d_m%d", geom->GetName(), bodyGroupFlags, materialGroupIdx);
	archetypeInfo->indexFormat = (EIndexFormat)geom->GetIndexFormat();
	archetypeInfo->meshInstFormat = instFormat;
	archetypeInfo->vertexBuffers.append(vertexBuffers);
	archetypeInfo->instanceStreamId = instanceStreamId;
	archetypeInfo->indexBuffer = indexBuffer;
	archetypeInfo->skinningSupport = skinningSupport;

	if (grim_dbg_logArchetypes.GetBool())
		MsgInfo("GRIM: creating archetype %d (%s)\n", slot, archetypeInfo->name.ToCString());

	int prevLod = -1;
	for (int i = 0; i < studio.numLodParams; i++)
	{
		const studioLodParams_t* lodParam = studio.pLodParams(i);
		GPULodInfo drawLodInfo;
		drawLodInfo.distance = lodParam->distance;

		bool hasBodyGroups = false;
		int prevBatch = -1;
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
				modelDescId = lodModel->modelsIndexes[bodyGroupLOD--];
			} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);
	
			if (modelDescId == EGF_INVALID_IDX)
				continue;
	
			const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);
			for (int k = 0; k < modDesc->numMeshes; ++k)
			{
				const CEqStudioGeom::HWGeomRef::MeshRef& meshRef = geomRefs[modelDescId].meshRefs[k];
				const IMaterialPtr material = materials[meshRef.materialIdx];
				if (material->GetFlags() & MATERIAL_FLAG_INVISIBLE)
					continue;
	
				GPUIndexedBatch drawBatch;
				drawBatch.firstIndex = meshRef.firstIndex;
				drawBatch.indexCount = meshRef.indexCount;

				const int newBatch = m_drawBatchs.Add(drawBatch);
				if (prevBatch != -1)
					m_drawBatchs[prevBatch].next = newBatch;
				else
					drawLodInfo.firstBatch = newBatch;
				prevBatch = newBatch;

				DrawInfo drawInfo;
				drawInfo.archetypeInfo = archetypeInfo;
				drawInfo.primTopology = (EPrimTopology)meshRef.primType;
				drawInfo.ownerArchetype = slot;
				drawInfo.material = material;
				drawInfo.batchIdx = newBatch;
				drawInfo.lodNumber = i;

				m_drawBatchs[newBatch].cmdIdx = m_drawInfos.add(drawInfo);
				hasBodyGroups = true;
			}
		}

		if (!hasBodyGroups)
			continue;

		const int newLod = m_drawLodInfos.Add(drawLodInfo);
		if(prevLod != -1)
		{
			m_drawLodInfos[prevLod].next = newLod;
		}
		else
			m_drawLodsList[slot].firstLodInfo = newLod;
		prevLod = newLod;
	}
}

void GRIMBaseRenderer::InitDrawArchetype(GRIMArchetype slot, const GRIMArchetypeDesc& desc)
{
	ASSERT_MSG(m_drawLodsList(slot), "Archetype slot %d is not reserved", slot);

	FixedArray<IGPUBufferPtr, MAX_VERTEXSTREAM> vertexBuffers;

	MeshInstanceFormatRef instFormat = desc.meshInstanceFormat;
	instFormat.usedLayoutBits = 0;
	int instanceStreamId = -1;

	int vbId = 0;
	for (int i = 0; i < instFormat.layout.numElem(); ++i)
	{
		const VertexLayoutDesc& layoutDesc = instFormat.layout[i];
		instFormat.usedLayoutBits |= (1 << i);

		if (instanceStreamId == -1 && layoutDesc.stepMode == VERTEX_STEPMODE_INSTANCE)
		{
			instanceStreamId = vertexBuffers.numElem();
			vertexBuffers.append(nullptr);
		}
		else
			vertexBuffers.append(desc.vertexBuffers[vbId++]);
	}
	ASSERT_MSG(instanceStreamId != -1, "Vertex format %s is not configured for instanced rendering", instFormat.name);
	instFormat.usedLayoutBits |= (1 << instanceStreamId);

	ArchetypeInfo::PTR_T archetypeInfo = CRefPtr_new(ArchetypeInfo);
	archetypeInfo->name = desc.name;
	archetypeInfo->indexFormat = desc.indexFormat;
	archetypeInfo->meshInstFormat = instFormat;
	archetypeInfo->vertexBuffers.append(vertexBuffers);
	archetypeInfo->instanceStreamId = instanceStreamId;
	archetypeInfo->indexBuffer = desc.indexBuffer;

	if (grim_dbg_logArchetypes.GetBool())
		MsgInfo("GRIM: creating archetype %d (%s)\n", slot, archetypeInfo->name.ToCString());

	int prevLod = -1;
	int numLods = 0;
	for (const GRIMArchetypeDesc::LodInfo& lodInfo : desc.lods)
	{
		GPULodInfo drawLodInfo;
		drawLodInfo.distance = lodInfo.distance;

		bool hasBodyGroups = false;
		int prevBatch = -1;
		for (int i = lodInfo.firstBatch; i < lodInfo.firstBatch + lodInfo.batchCount; ++i)
		{
			const GRIMArchetypeDesc::Batch& batch = desc.batches[i];
			if (batch.material->GetFlags() & MATERIAL_FLAG_INVISIBLE)
				continue;

			GPUIndexedBatch drawBatch;
			drawBatch.firstIndex = batch.firstIndex;
			drawBatch.indexCount = batch.indexCount;

			const int newBatch = m_drawBatchs.Add(drawBatch);
			if (prevBatch != -1)
				m_drawBatchs[prevBatch].next = newBatch;
			else
				drawLodInfo.firstBatch = newBatch;
			prevBatch = newBatch;

			DrawInfo drawInfo;
			drawInfo.archetypeInfo = archetypeInfo;
			drawInfo.primTopology = batch.primTopology;
			drawInfo.ownerArchetype = slot;
			drawInfo.material = batch.material;
			drawInfo.batchIdx = newBatch;
			drawInfo.lodNumber = numLods;

			m_drawBatchs[newBatch].cmdIdx = m_drawInfos.add(drawInfo);
			hasBodyGroups = true;
		}

		if (!hasBodyGroups)
			continue;

		const int newLod = m_drawLodInfos.Add(drawLodInfo);
		if (prevLod != -1)
			m_drawLodInfos[prevLod].next = newLod;
		else
			m_drawLodsList[slot].firstLodInfo = newLod;
		prevLod = newLod;

		++numLods;
	}
}

void GRIMBaseRenderer::DestroyDrawArchetype(GRIMArchetype archetype)
{
	CScopedMutex m(s_grimRendererMutex);
	if (!m_drawLodsList(archetype))
		return;

	m_pendingDeletion.append(archetype);
}

void GRIMBaseRenderer::DbgGetArchetypeNames(Array<EqStringRef>& archetypeNames) const
{
	CScopedMutex m(s_grimRendererMutex);
	archetypeNames.setNum(m_drawLodsList.NumSlots());
	for(int i = 0; i < m_drawLodsList.NumSlots(); ++i)
	{
		if(!m_drawLodsList(i))
			continue;

		const int firstLodInfo = m_drawLodsList[i].firstLodInfo;
		if(firstLodInfo == -1)
		{
			//ASSERT_FAIL("No lods for archetype %d\n", i);
			continue;
		}

		const int firstBatch = m_drawLodInfos[firstLodInfo].firstBatch;
		if(firstBatch == -1)
		{
			ASSERT_FAIL("No batchs for archetype %d\n", i);
			continue;
		}

		const int cmdIdx = m_drawBatchs[firstBatch].cmdIdx;
		if(cmdIdx == -1)
		{
			ASSERT_FAIL("No cmd for lod %d batch %d of archetype %d\n", firstLodInfo, firstBatch, i);
			continue;
		}

		if(!m_drawInfos[cmdIdx].archetypeInfo)
		{
			ASSERT_FAIL("Missing archetypeInfo lod %d batch %d of archetype %d\n", firstLodInfo, firstBatch, i);
			continue;
		}

		archetypeNames[i] = m_drawInfos[cmdIdx].archetypeInfo->name;
	}
}

void GRIMBaseRenderer::DestroyPendingArchetypes()
{
	if (!m_pendingDeletion.numElem())
		return;

	Array<GRIMArchetype> pendingDeletion(PP_SL);
	{
		CScopedMutex m(s_grimRendererMutex);
		pendingDeletion.swap(m_pendingDeletion);
	}

	struct ItemInfo {
		enum EWhat
		{
			DRAWINFO,
			BATCH,
			LODINFO,
			LODLIST
		};
		int index : 24;
		int what : 8;
	};
	Array<ItemInfo> delItems(PP_SL);
	delItems.reserve(64 * pendingDeletion.numElem());

	for (GRIMArchetype id : pendingDeletion)
	{
		const GPULodList lodList = m_drawLodsList[id];
		for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = m_drawLodInfos[lodIdx].next)
		{
			const GPULodInfo lodInfo = m_drawLodInfos[lodIdx];
			for (int batchIdx = lodInfo.firstBatch; batchIdx != -1; batchIdx = m_drawBatchs[batchIdx].next)
			{
				delItems.appendEmplace(m_drawBatchs[batchIdx].cmdIdx, ItemInfo::DRAWINFO);
				delItems.appendEmplace(batchIdx, ItemInfo::BATCH);
			}
			delItems.appendEmplace(lodIdx, ItemInfo::LODINFO);
		}
		delItems.appendEmplace(id, ItemInfo::LODLIST);
	}

	for (ItemInfo& item : delItems)
	{
		switch (item.what)
		{
		case ItemInfo::DRAWINFO:
			m_drawInfos.remove(item.index);
			break;
		case ItemInfo::BATCH:
			m_drawBatchs.Remove(item.index);
			break;
		case ItemInfo::LODINFO:
			m_drawLodInfos.Remove(item.index);
			break;
		case ItemInfo::LODLIST:
			m_drawLodsList.Remove(item.index);
#ifndef _RETAIL
			if (grim_dbg_logArchetypes.GetBool())
				MsgInfo("GRIM: freed archetype %d\n", item.index);
#endif
			break;
		}
	}
}

void GRIMBaseRenderer::SyncArchetypes(IGPUCommandRecorder* cmdRecorder)
{
	Array<PendingDesc> pending(PP_SL);
	if(m_pendingArchetypes.numElem())
	{
		CScopedMutex m(s_grimRendererMutex);
		m_pendingArchetypes.swap(pending);
	}

	// before we send anything to GPU, we need to commit new archetypes
	// destroy pending first so we can place new ones at their slots
	DestroyPendingArchetypes();
	for (PendingDesc& pending : pending)
	{
		if (pending.type == PendingDesc::TYPE_GRIM)
			InitDrawArchetype(pending.slot, pending.desc);
		else if (pending.type == PendingDesc::TYPE_STUDIO)
			InitDrawArchetype(pending.slot, pending.egfDesc.geom, pending.egfDesc.vertFormat, pending.egfDesc.bodyGroupFlags, pending.egfDesc.materialGroupIdx, pending.extraVertexBuffers);
	}

	// we have to sync desc buffers first
	bool buffersUpdated = false;
	if (m_drawBatchs.Sync(cmdRecorder))
		buffersUpdated = true;

	if (m_drawLodInfos.Sync(cmdRecorder))
		buffersUpdated = true;

	if (m_drawLodsList.Sync(cmdRecorder))
		buffersUpdated = true;

	if (!buffersUpdated)
		return;

	m_updateBindGroup0 = g_renderAPI->CreateBindGroup(m_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, m_drawBatchs.GetBuffer())
		.Buffer(1, m_drawLodInfos.GetBuffer())
		.Buffer(2, m_drawLodsList.GetBuffer())
		.End()
	);

	m_cullBindGroup0 = g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, m_drawLodInfos.GetBuffer())
		.Buffer(1, m_drawLodsList.GetBuffer())
		.End()
	);
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
	const int numBounds = m_drawLodsList.NumSlots() * GRIM_MAX_INSTANCE_LODS;
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

	drawInstanceBounds.setNum(m_drawLodsList.NumSlots() * GRIM_MAX_INSTANCE_LODS);
	if (instanceCount > 0)
	{
		const int lastArchetypeId = instanceInfos[instanceCount - 1].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
		const int lastArchLodIndex = (instanceInfos[instanceCount - 1].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
		ASSERT(lastArchLodIndex < GRIM_MAX_INSTANCE_LODS);
		const int lastBoundIdx = lastArchetypeId * GRIM_MAX_INSTANCE_LODS + lastArchLodIndex;

		drawInstanceBounds[lastBoundIdx].last = instanceCount;
	}

	for (int i = 0; i < instanceCount; ++i)
	{
		instanceIds[i] = instanceInfos[i].instanceId;

		if (i == 0 || instanceInfos[i].packedArchetypeId > instanceInfos[i - 1].packedArchetypeId)
		{
			const int archetypeId = instanceInfos[i].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
			const int archLodIndex = (instanceInfos[i].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
			ASSERT(archLodIndex < GRIM_MAX_INSTANCE_LODS);
			const int boundIdx = archetypeId * GRIM_MAX_INSTANCE_LODS + archLodIndex;

			drawInstanceBounds[boundIdx].first = i;
			drawInstanceBounds[boundIdx].archIdx = archetypeId;
			drawInstanceBounds[boundIdx].lodIndex = archLodIndex;
			if (i > 0)
			{
				const int lastArchetypeId = instanceInfos[i - 1].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
				const int lastArchLodIndex = (instanceInfos[i - 1].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;
				const int lastBoundIdx = lastArchetypeId * GRIM_MAX_INSTANCE_LODS + lastArchLodIndex;

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
		int lodInfos[GRIM_MAX_INSTANCE_LODS];
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

	if (maxNumberOfObjects < 0)
		maxNumberOfObjects = m_instAllocator.GetInstanceCount();

	renderState.drawInvocationsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUDrawIndexedIndirectCmd), m_drawInfos.numSlots()), BUFFERUSAGE_INDIRECT | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawInvocations");
	renderState.instanceIdsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), maxNumberOfObjects), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceIds");
	renderState.visibleArchetypes.resize(m_drawLodsList.NumSlots()+1);
	renderState.visibleArchetypes.reset(true);

	IntermediateState intermediate{ renderState };
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

	const int numBounds = m_drawLodsList.NumSlots() * GRIM_MAX_INSTANCE_LODS;
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
	if (renderState.drawInvocationsBuffer == nullptr || renderState.instanceIdsBuffer == nullptr)
		return;

	PROF_EVENT_F();

	struct ListItm
	{
		uint16 id{ USHRT_MAX };
		uint16 next{ USHRT_MAX };
	};

	Array<ListItm> drawInfoLinkList(PP_SL);
	drawInfoLinkList.reserve(m_drawInfos.numSlots());
	drawInfoLinkList.append(ListItm{}); // store end element in this array

	const bool validationOn = false;// TODO g_renderAPI->IsValidationEnabled();

	Map<uint64, int> drawInfosByMaterial(PP_SL);
	for (int i = 0; i < m_drawInfos.numSlots(); ++i)
	{
		if (!m_drawInfos(i))
			continue;
		const DrawInfo& drawInfo = m_drawInfos[i];

		// do not draw anything if no instances referencing this archetype
		if (m_instAllocator.GetInstanceCountByArchetype(drawInfo.ownerArchetype) == 0)
			continue;

		const ArchetypeInfo& archetypeInfo = drawInfo.archetypeInfo.Ref();

		IMaterial* material = drawInfo.material;
		IMaterial* originalMaterial = material;
		if (renderPassCtx.beforeMaterialSetup)
			material = renderPassCtx.beforeMaterialSetup(material);

		if (!material)
			continue;

		const char* onlyMaterialName = grim_dbg_onlyMaterial.GetString();
		if (*onlyMaterialName)
		{
			if (CString::CompareCaseIns(originalMaterial->GetName(), onlyMaterialName))
				continue;
		}

		uint64 materialId = reinterpret_cast<uint64>(material);
		materialId *= 31;
		materialId += archetypeInfo.meshInstFormat.formatId;
		materialId *= 31;
		materialId += archetypeInfo.meshInstFormat.usedLayoutBits;
		materialId *= 31;
		materialId += drawInfo.primTopology;

		auto it = drawInfosByMaterial.find(materialId);
		if (it.atEnd())
			it = drawInfosByMaterial.insert(materialId, 0); // insert list end

		*it = drawInfoLinkList.append(ListItm{ (uint16)i, (uint16)*it }); // link to last element
	}


	renderPassCtx.recorder->DbgPushGroup("GRIMDraw");
	int numDrawCalls = 0;
	for (auto it = drawInfosByMaterial.begin(); !it.atEnd(); ++it)
	{
		ListItm litem = drawInfoLinkList[*it];

		const DrawInfo& setupDrawInfo = m_drawInfos[litem.id];
		const ArchetypeInfo& setupArchetypeInfo = setupDrawInfo.archetypeInfo.Ref();

		if (!g_matSystem->SetupMaterialPipeline(setupDrawInfo.material, nullptr, setupDrawInfo.primTopology, setupArchetypeInfo.meshInstFormat, renderPassCtx, this))
			continue;

		// TODO: if archetypeInfo is matching, use MultiDrawIndirect

		for(; litem.id != USHRT_MAX; litem = drawInfoLinkList[litem.next])
		{
			const DrawInfo& drawInfo = m_drawInfos[litem.id];
			const ArchetypeInfo& archetypeInfo = drawInfo.archetypeInfo.Ref();

			if(validationOn)
				renderPassCtx.recorder->DbgAddMarker(EqString::Format("draw arch %d (mtl %s) (lod %d) (cnt %d)", drawInfo.ownerArchetype, drawInfo.material->GetName(), drawInfo.lodNumber, m_instAllocator.GetInstanceCountByArchetype(drawInfo.ownerArchetype)));

			for (int vsi = 0; vsi < archetypeInfo.vertexBuffers.numElem(); ++vsi)
				renderPassCtx.recorder->SetVertexBuffer(vsi, (archetypeInfo.instanceStreamId == vsi) ? renderState.instanceIdsBuffer : archetypeInfo.vertexBuffers[vsi]);
			renderPassCtx.recorder->SetIndexBuffer(archetypeInfo.indexBuffer, archetypeInfo.indexFormat);

			renderPassCtx.recorder->DrawIndexedIndirect(renderState.drawInvocationsBuffer, sizeof(GPUDrawIndexedIndirectCmd)* m_drawBatchs[drawInfo.batchIdx].cmdIdx);
			++numDrawCalls;
		}
	}
	renderPassCtx.recorder->DbgPopGroup();

	if(grim_stats.GetBool())
	{
		debugoverlay->Text(color_white, "--- GRIM instances summary ---");
		debugoverlay->Text(color_white, "Mode: %s", grim_force_software.GetBool() ? "CPU" : "Compute");
		debugoverlay->Text(color_white, " %d draw infos: %.2f KB", m_drawInfos.numElem(), memBytesToKB(m_drawInfos.numSlots() * sizeof(m_drawInfos[0])));
		debugoverlay->Text(color_white, " %d batchs: %.2f KB", m_drawBatchs.NumElem(), memBytesToKB(m_drawBatchs.NumSlots() * sizeof(m_drawBatchs[0])));
		debugoverlay->Text(color_white, " %d lod infos: %.2f KB", m_drawLodInfos.NumElem(), memBytesToKB(m_drawLodInfos.NumSlots() * sizeof(m_drawLodInfos[0])));
		debugoverlay->Text(color_white, " %d lod lists: %.2f KB", m_drawLodsList.NumElem(), memBytesToKB(m_drawLodsList.NumSlots() * sizeof(m_drawLodsList[0])));

		debugoverlay->Text(color_white, "Draw materials: %d", drawInfosByMaterial.size());
		debugoverlay->Text(color_white, "Draw calls: %d", numDrawCalls);
	}
}