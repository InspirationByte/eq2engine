#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
#include "ds/SlottedArray.h"
#include "math/Random.h"
#include "math/Utility.h"
#include "utils/KeyValues.h"
#include "utils/TextureAtlas.h"
#include "utils/SpiralIterator.h"

#include "gpudriven_main.h"

#include "sys/sys_host.h"

#include "audio/eqSoundEmitterSystem.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/EqParticles.h"
#include "render/GPUInstanceMng.h"
#include "render/IDebugOverlay.h"
#include "render/ViewParams.h"
#include "render/ComputeSort.h"
#include "input/InputCommandBinder.h"
#include "studio/StudioGeom.h"
#include "studio/StudioCache.h"
#include "instancer.h"
#include "sys/sys_in_console.h"

DECLARE_CVAR(cam_speed, "100", nullptr, 0);
DECLARE_CVAR(cam_fov, "90", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_count, "10000", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update_once, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_use_compute, "1", nullptr, CV_ARCHIVE);

static void lod_idx_changed(ConVar* pVar, char const* pszOldValue)
{
	inst_update.SetBool(true);
}

DECLARE_CVAR_CHANGE(lod_idx, "-1", lod_idx_changed, nullptr, 0);
DECLARE_CVAR(obj_rotate_interval, "100", nullptr, 0);
DECLARE_CVAR(obj_rotate, "1", nullptr, 0);
DECLARE_CVAR(obj_draw, "1", nullptr, 0);

static DemoGPUInstanceManager s_instanceMng;
static CStaticAutoPtr<CState_GpuDrivenDemo> g_State_Demo;
static IVertexFormat* s_gameObjectVF = nullptr;

static CViewParams s_currentView;

enum ECameraControls
{
	CAM_FORWARD = (1 << 0),
	CAM_BACKWARD = (1 << 1),
	CAM_SIDE_LEFT = (1 << 2),
	CAM_SIDE_RIGHT = (1 << 3),
};

struct Object
{
	Transform3D trs;
	int instId{ -1 };
};
static Array<Object>	s_objects{ PP_SL };

//--------------------------------------------

static constexpr int MAX_INSTANCE_LODS = 8;

struct GPUIndexedBatch
{
	int		next{ -1 };		// next index in buffer pointing to GPUIndexedBatch

	int		indexCount{ 0 };
	int		firstIndex{ 0 };
	int		cmdIdx{ -1 };
};

struct GPULodInfo
{
	int		next{ -1 };			// next index in buffer pointing to GPULodInfo

	int		firstBatch{ 0 };	//  item index in buffer pointing to GPUIndexedBatch
	float	distance{ 0.0f };
};

struct GPULodList
{
	int		firstLodInfo; // item index in buffer pointing to GPULodInfo
};

// this is only here for software reference impl
struct GPUInstanceBound
{
	int		first{ 0 };
	int		last{ 0 };
	int		archIdx{ -1 };
	int		lodIndex{ -1 };
};

//-------------------------------------------------

struct GPUDrawInfo
{
	IGPUBufferPtr			vertexBuffers[MAX_VERTEXSTREAM - 1];
	IGPUBufferPtr			indexBuffer;
	IMaterialPtr			material;
	MeshInstanceFormatRef	meshInstFormat;
	EPrimTopology			primTopology{ PRIM_TRIANGLES };
	EIndexFormat			indexFormat{ 0 };
	int						batchIdx{ -1 };
};

//-------------------------------------------------

static Map<int, int>					s_modelIdToArchetypeId{ PP_SL };
static SlottedArray<GPUDrawInfo>		s_drawInfos{ PP_SL };	// must be in sync with batchs
static SlottedArray<GPUIndexedBatch>	s_drawBatchs{ PP_SL };
static SlottedArray<GPULodInfo>			s_drawLodInfos{ PP_SL };
static SlottedArray<GPULodList>			s_drawLodsList{ PP_SL };

static IGPUBufferPtr					s_drawBatchsBuffer;
static IGPUBufferPtr					s_drawLodInfosBuffer;
static IGPUBufferPtr					s_drawLodsListBuffer;

static IGPUBufferPtr					s_drawInvocationsBuffer;
static IGPUBufferPtr					s_instanceIdsBuffer;		// sorted ready to draw instances
static ComputeBitonicMergeSortShaderPtr	s_mergeSortShader;

static IGPUComputePipelinePtr			s_instCalcBoundsPipeline;
static IGPUComputePipelinePtr			s_instPrepareDrawIndirect;
// culling pipeline must be external
static IGPUComputePipelinePtr			s_cullInstancesPipeline;

static IGPUBindGroupPtr					s_cullBindGroup0;
static IGPUBindGroupPtr					s_updateBindGroup0;

static constexpr char SHADERNAME_SORT_INSTANCES[] = "InstanceArchetypeSort";
static constexpr char SHADERNAME_CALC_INSTANCE_BOUNDS[] = "InstanceCalcBounds";
static constexpr char SHADERNAME_PREPARE_INDIRECT_INSTANCES[] = "InstancePrepareDrawIndirect";
static constexpr char SHADERNAME_CULL_INSTANCES[] = "InstancesCull";

static constexpr char SHADER_PIPELINE_SORT_INSTANCES[] = "InstanceInfos";


static void InitIndirectRenderer()
{
	s_mergeSortShader = CRefPtr_new(ComputeSortShader);
	s_mergeSortShader->AddSortPipeline(SHADER_PIPELINE_SORT_INSTANCES, SHADERNAME_SORT_INSTANCES);

	s_drawBatchsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUIndexedBatch), s_drawBatchs.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawBatchs");
	s_drawLodInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPULodInfo), s_drawLodInfos.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodInfos");
	s_drawLodsListBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(s_drawLodsList), s_drawLodsList.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodsList");

	s_drawBatchsBuffer->Update(s_drawBatchs.ptr(), s_drawBatchs.numSlots() * sizeof(s_drawBatchs[0]), 0);
	s_drawLodInfosBuffer->Update(s_drawLodInfos.ptr(), s_drawLodInfos.numSlots() * sizeof(s_drawLodInfos[0]), 0);
	s_drawLodsListBuffer->Update(s_drawLodsList.ptr(), s_drawLodsList.numSlots() * sizeof(s_drawLodsList[0]), 0);

	s_drawInvocationsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUDrawIndexedIndirectCmd), s_drawInfos.numSlots()), BUFFERUSAGE_INDIRECT | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawInvocations");

	s_instCalcBoundsPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_CALC_INSTANCE_BOUNDS)
		.End()
	);

	s_instPrepareDrawIndirect = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_PREPARE_INDIRECT_INSTANCES)
		.End()
	);

	s_cullInstancesPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(SHADERNAME_CULL_INSTANCES)
		.End()
	);

	s_updateBindGroup0 = g_renderAPI->CreateBindGroup(s_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, s_drawBatchsBuffer)
		.Buffer(1, s_drawLodInfosBuffer)
		.Buffer(2, s_drawLodsListBuffer)
		.End()
	);

	s_cullBindGroup0 = g_renderAPI->CreateBindGroup(s_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, s_drawLodInfosBuffer)
		.Buffer(1, s_drawLodsListBuffer)
		.End()
	);
}

static void TermIndirectRenderer()
{
	s_modelIdToArchetypeId.clear(true);
	s_drawBatchs.clear(true);
	s_drawInfos.clear(true);
	s_drawLodInfos.clear(true);
	s_drawLodsList.clear(true);

	s_drawBatchsBuffer = nullptr;
	s_drawLodInfosBuffer = nullptr;
	s_drawLodsListBuffer = nullptr;
	s_drawInvocationsBuffer = nullptr;
	s_instanceIdsBuffer = nullptr;
	s_mergeSortShader = nullptr;
	s_instCalcBoundsPipeline = nullptr;
	s_instPrepareDrawIndirect = nullptr;
	s_updateBindGroup0 = nullptr;

	s_cullInstancesPipeline = nullptr;
	s_cullBindGroup0 = nullptr;
}

static int CreateDrawArchetypeEGF(const CEqStudioGeom& geom, uint bodyGroupFlags = 0, int materialGroupIdx = 0)
{
	ASSERT(bodyGroupFlags != 0);

	IVertexFormat* vertFormat = s_gameObjectVF;
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

				const int newBatch = s_drawBatchs.add(drawBatch);
				if (prevBatch != -1)
					s_drawBatchs[prevBatch].next = newBatch;
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

				s_drawBatchs[newBatch].cmdIdx = s_drawInfos.add(drawInfo);
			}
		}

		const int newLod = s_drawLodInfos.add(lodInfo);
		if(prevLod != -1)
			s_drawLodInfos[prevLod].next = newLod;
		else
			lodList.firstLodInfo = newLod;
		prevLod = newLod;
	}

	const int archetypeId = s_drawLodsList.add(lodList);

	// TODO: body group lookup
	s_modelIdToArchetypeId.insert(geom.GetCacheId(), archetypeId);

	// what we are syncing to the GPU:
	// s_drawBatchs
	// s_drawLods
	// s_drawLodsList
	// s_drawArchetypeMaterials

	return archetypeId;
}

struct GPUInstanceInfo
{
	static constexpr int ARCHETYPE_BITS = 24;
	static constexpr int LOD_BITS = 8;

	static constexpr int ARCHETYPE_MASK = (1 << ARCHETYPE_BITS) - 1;
	static constexpr int LOD_MASK = (1 << LOD_BITS) - 1;

	int		instanceId;
	int		packedArchetypeId;
};

//--------------------------------------------------------------------

constexpr int GPUVIS_GROUP_SIZE = 256;
constexpr int GPUVIS_MAX_DIM_GROUPS = 1024;
constexpr int GPUVIS_MAX_DIM_THREADS = (GPUVIS_GROUP_SIZE * GPUVIS_MAX_DIM_GROUPS);

static void VisCalcWorkSize(int length, int& x, int& y, int& z)
{
	if (length <= GPUVIS_MAX_DIM_THREADS)
	{
		x = (length - 1) / GPUVIS_GROUP_SIZE + 1;
		y = z = 1;
	}
	else
	{
		x = GPUVIS_MAX_DIM_GROUPS;
		y = (length - 1) / GPUVIS_MAX_DIM_THREADS + 1;
		z = 1;
	}
}

static void VisibilityCullInstances_Compute(const Vector3D& camPos, const Volume& frustum, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr sortedKeysBuffer, IGPUBufferPtr instanceInfosBuffer)
{
	PROF_EVENT_F();

	struct CullViewParams
	{
		Vector4D	frustumPlanes[6];
		Vector3D	cameraPos;
		int			overrideLodIdx;
		int			maxInstanceIds;
		float		_padding[3];
	};

	CullViewParams cullView;
	memcpy(cullView.frustumPlanes, frustum.GetPlanes().ptr(), sizeof(cullView.frustumPlanes));
	cullView.cameraPos = camPos;
	cullView.overrideLodIdx = lod_idx.GetInt();
	cullView.maxInstanceIds = s_instanceMng.GetInstanceSlotsCount();	// TODO: calculate between frames

	IGPUBufferPtr viewParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(CullViewParams), 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "ViewParamsBuffer");
	cmdRecorder->WriteBuffer(viewParamsBuffer, &cullView, sizeof(cullView), 0);
	cmdRecorder->ClearBuffer(sortedKeysBuffer, 0, sizeof(int));

	IGPUComputePassRecorderPtr computeRecorder = cmdRecorder->BeginComputePass("CullInstances");
	computeRecorder->SetPipeline(s_cullInstancesPipeline);
	computeRecorder->SetBindGroup(0, s_cullBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(s_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, s_instanceMng.GetInstanceArchetypesBuffer())
		.Buffer(1, viewParamsBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(s_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, instanceInfosBuffer)
		.Buffer(1, sortedKeysBuffer)
		.End())
	);

	computeRecorder->SetBindGroup(3, g_renderAPI->CreateBindGroup(s_cullInstancesPipeline, Builder<BindGroupDesc>()
		.GroupIndex(3)
		.Buffer(0, s_instanceMng.GetRootBuffer())
		.Buffer(1, s_instanceMng.GetDataPoolBuffer(InstTransform::COMPONENT_ID))
		.End())
	);

	int x, y, z;
	VisCalcWorkSize(cullView.maxInstanceIds, x, y, z);

	computeRecorder->DispatchWorkgroups(x, y , z);
	computeRecorder->Complete();
}

static void SortInstances_Compute( IGPUBufferPtr instanceInfosBuffer, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr sortedKeysBuffer)
{
	PROF_EVENT_F();
	const int maxInstancesCount = s_instanceMng.GetInstanceSlotsCount();
	s_mergeSortShader->InitKeys(cmdRecorder, sortedKeysBuffer, maxInstancesCount);
	s_mergeSortShader->SortKeys(StringToHashConst(SHADER_PIPELINE_SORT_INSTANCES), cmdRecorder, sortedKeysBuffer, maxInstancesCount, instanceInfosBuffer);
}

static void UpdateInstanceBounds_Compute(IGPUBufferPtr sortedKeysBuffer, IGPUBufferPtr instanceInfosBuffer, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr instanceIdsBuffer, IGPUBufferPtr instanceBoundsBuffer)
{
	PROF_EVENT_F();

	IGPUComputePassRecorderPtr computeRecorder = cmdRecorder->BeginComputePass("CalcInstanceBounds");
	computeRecorder->SetPipeline(s_instCalcBoundsPipeline);
	computeRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(s_instCalcBoundsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, instanceInfosBuffer)
		.Buffer(1, sortedKeysBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(s_instCalcBoundsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, instanceBoundsBuffer)
		.Buffer(1, instanceIdsBuffer)
		.End())
	);

	constexpr int GROUP_SIZE = 128;

	// TODO: DispatchWorkgroupsIndirect (use as result from VisibilityCullInstances)
	computeRecorder->DispatchWorkgroups(s_objects.numElem() / GROUP_SIZE + 1);
	computeRecorder->Complete();
}

static void UpdateIndirectInstances_Compute(IGPUBufferPtr drawInstanceBoundsBuffer, IGPUCommandRecorder* cmdRecorder)
{
	PROF_EVENT_F();

	IGPUComputePassRecorderPtr computeRecorder = cmdRecorder->BeginComputePass("UpdateIndirectInstances");
	computeRecorder->SetPipeline(s_instPrepareDrawIndirect);
	computeRecorder->SetBindGroup(0, s_updateBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(s_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, drawInstanceBoundsBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(s_instPrepareDrawIndirect,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, s_drawInvocationsBuffer)
		.End())
	);

	constexpr int GROUP_SIZE = 32;
	const int numBounds = s_drawLodsList.numSlots() * MAX_INSTANCE_LODS;
	computeRecorder->DispatchWorkgroups(numBounds / GROUP_SIZE + 1);
	computeRecorder->Complete();
}

//--------------------------------------------------------------------

static void VisibilityCullInstances_Software(const Vector3D& camPos, const Volume& frustum, Array<GPUInstanceInfo>& instanceInfos)
{
	PROF_EVENT_F();

	// COMPUTE SHADER REFERENCE: VisibilityCullInstances
	// Input:
	//		instanceIds		: buffer<int[]>
	// Output:
	//		instanceInfos	: buffer<GPUInstanceInfo[]>

	instanceInfos.reserve(s_objects.numElem());

	for (const Object& obj : s_objects)
	{
		const int archetypeId = s_instanceMng.GetInstanceArchetypeId(obj.instId);
		if (archetypeId == -1)
			continue;

		if (!frustum.IsSphereInside(obj.trs.t, 4.0f))
			continue;

		const GPULodList& lodList = s_drawLodsList[archetypeId];
		const float distFromCamera = distanceSqr(camPos, obj.trs.t);

		// find suitable lod idx
		int drawLod = lod_idx.GetInt();
		if (drawLod == -1)
		{
			for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = s_drawLodInfos[lodIdx].next)
			{
				if (distFromCamera < sqr(s_drawLodInfos[lodIdx].distance))
					break;
				drawLod++;
			}
		}

		instanceInfos.append({ obj.instId, archetypeId | (drawLod << GPUInstanceInfo::ARCHETYPE_BITS) });
	}
}

static void SortInstances_Software(Array<GPUInstanceInfo>& instanceInfos)
{
	PROF_EVENT_F();

	// COMPUTE SHADER REFERENCE: SortInstanceArchetypes
	// Input / Output:
	//		instanceInfos		: buffer<GPUInstanceInfo[]>

	// sort instances by archetype
	// as we need them to be linear for each draw call
	arraySort(instanceInfos, [](const GPUInstanceInfo& a, const GPUInstanceInfo& b) {
		return a.packedArchetypeId - b.packedArchetypeId;
	});
}

static void UpdateInstanceBounds_Software(ArrayCRef<GPUInstanceInfo> instanceInfos, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr instanceIdsBuffer, Array<GPUInstanceBound>& drawInstanceBounds)
{
	PROF_EVENT_F();

	const int instanceCount = instanceInfos.numElem();

	// COMPUTE SHADER REFERENCE: VisibilityCullInstances
	// Input:
	//		instanceInfos			: buffer<GPUInstanceInfo>
	// Output:
	//		instanceIds				: buffer<int[]>
	//		drawInstanceBounds		: buffer<GPUInstanceBound[]>

	Array<int> instanceIds(PP_SL);
	instanceIds.setNum(instanceCount);

	drawInstanceBounds.setNum(s_drawLodsList.numElem() * MAX_INSTANCE_LODS);
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

static void UpdateIndirectInstances_Software(ArrayCRef<GPUInstanceBound> drawInstanceBounds, IGPUCommandRecorder* cmdRecorder)
{
	PROF_EVENT_F();

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

		const GPULodList& lodList = s_drawLodsList[bound.archIdx];

		// fill lod list
		int numLods = 0;
		int lodInfos[MAX_INSTANCE_LODS];
		for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = s_drawLodInfos[lodIdx].next)
			lodInfos[numLods++] = lodIdx;

		// walk over batches
		const int lodIdx = min(numLods - 1, bound.lodIndex);

		const GPULodInfo& lodInfo = s_drawLodInfos[lodInfos[lodIdx]];
		for (int batchIdx = lodInfo.firstBatch; batchIdx != -1; batchIdx = s_drawBatchs[batchIdx].next)
		{
			const GPUIndexedBatch& drawBatch = s_drawBatchs[batchIdx];

			GPUDrawIndexedIndirectCmd drawCmd;
			drawCmd.firstIndex = drawBatch.firstIndex;
			drawCmd.indexCount = drawBatch.indexCount;
			drawCmd.firstInstance = bound.first;
			drawCmd.instanceCount = bound.last - bound.first;

			ASSERT(s_drawInfos(drawBatch.cmdIdx));

			cmdRecorder->WriteBuffer(s_drawInvocationsBuffer, &drawCmd, sizeof(drawCmd), sizeof(GPUDrawIndexedIndirectCmd) * drawBatch.cmdIdx);
		}
	}
}

//--------------------------------------------------------------------

static float memBytesToKB(size_t byteCnt)
{
	return byteCnt / 1024.0f;
}

static void DrawScene(const RenderPassContext& renderPassCtx)
{
	PROF_EVENT_F();

	int numDrawCalls = 0;

	if(obj_draw.GetBool())
	{
		for (int i = 0; i < s_drawInfos.numSlots(); ++i)
		{
			if (!s_drawInfos(i))
			{
				debugoverlay->Text(color_white, " draw info %d not drawn", i);
				continue;
			}

			const GPUDrawInfo& drawInfo = s_drawInfos[i];

			IMaterial* material = drawInfo.material;
			if (g_matSystem->SetupMaterialPipeline(material, nullptr, drawInfo.primTopology, drawInfo.meshInstFormat, renderPassCtx, &s_instanceMng))
			{
				renderPassCtx.recorder->SetVertexBuffer(0, drawInfo.vertexBuffers[0]);
				renderPassCtx.recorder->SetVertexBuffer(1, s_instanceIdsBuffer);
				renderPassCtx.recorder->SetIndexBuffer(drawInfo.indexBuffer, drawInfo.indexFormat);

				renderPassCtx.recorder->DrawIndexedIndirect(s_drawInvocationsBuffer, sizeof(GPUDrawIndexedIndirectCmd) * s_drawBatchs[drawInfo.batchIdx].cmdIdx);
				++numDrawCalls;
			}
		}
	}

	debugoverlay->Text(color_white, "--- instances summary ---");
	debugoverlay->Text(color_white, " %d draw infos: %.2f KB", s_drawInfos.numElem(), memBytesToKB(s_drawInfos.numSlots() * sizeof(s_drawInfos[0])));
	debugoverlay->Text(color_white, " %d batchs: %.2f KB", s_drawBatchs.numElem(), memBytesToKB(s_drawBatchs.numSlots() * sizeof(s_drawBatchs[0])));
	debugoverlay->Text(color_white, " %d lod infos: %.2f KB", s_drawLodInfos.numElem(), memBytesToKB(s_drawLodInfos.numSlots() * sizeof(s_drawLodInfos[0])));
	debugoverlay->Text(color_white, " %d lod lists: %.2f KB", s_drawLodsList.numElem(), memBytesToKB(s_drawLodsList.numSlots() * sizeof(s_drawLodsList[0])));

	debugoverlay->Text(color_white, "Draw calls: %d", numDrawCalls);
}

//--------------------------------------------

CState_GpuDrivenDemo::CState_GpuDrivenDemo()
{

}

// when changed to this state
// @from - used to transfer data
void CState_GpuDrivenDemo::OnEnter(CAppStateBase* from)
{
	g_inputCommandBinder->AddBinding("W", "forward", [](void* _this, short value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_FORWARD, value > 0);
	}, this);

	g_inputCommandBinder->AddBinding("S", "backward", [](void* _this, short value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_BACKWARD, value > 0);
	}, this);

	g_inputCommandBinder->AddBinding("A", "strafeleft", [](void* _this, short value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_SIDE_LEFT, value > 0);
	}, this);

	g_inputCommandBinder->AddBinding("D", "straferight", [](void* _this, short value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_SIDE_RIGHT, value > 0);
	}, this);

	g_inputCommandBinder->AddBinding("R", "reset", [](void* _this, short value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);

		if (value <= 0)
			this_->InitGame();
	}, this);

	// go heavy.
	constexpr const int INST_RESERVE = 5000000;

	g_pfxRender->Init();
	s_instanceMng.Initialize("InstanceUtils", INST_RESERVE);
	s_objects.reserve(INST_RESERVE);

	{
		const VertexLayoutDesc& egfPosUvsDesc = EGFHwVertex::PositionUV::GetVertexLayoutDesc();
		//const VertexLayoutDesc& egfTbnDesc = EGFHwVertex::TBN::GetVertexLayoutDesc();
		//const VertexLayoutDesc& egfBoneWeightDesc = EGFHwVertex::BoneWeights::GetVertexLayoutDesc();
		//const VertexLayoutDesc& egfColorDesc = EGFHwVertex::Color::GetVertexLayoutDesc();

		const VertexLayoutDesc& gpuInstDesc = GetGPUInstanceVertexLayout();

		FixedArray<VertexLayoutDesc, 5> gameObjInstEgfLayout;
		gameObjInstEgfLayout.append(egfPosUvsDesc);
		//gameObjInstEgfLayout.append(egfTbnDesc);
		//gameObjInstEgfLayout.append(egfBoneWeightDesc);
		//gameObjInstEgfLayout.append(egfColorDesc);
		gameObjInstEgfLayout.append(gpuInstDesc);

		// disable tangent, binormal
		//gameObjInstEgfLayout[1].attributes[0].format = ATTRIBUTEFORMAT_NONE;
		//gameObjInstEgfLayout[1].attributes[1].format = ATTRIBUTEFORMAT_NONE;
		//
		//// disable color
		//gameObjInstEgfLayout[2].attributes[0].format = ATTRIBUTEFORMAT_NONE;

		s_gameObjectVF = g_renderAPI->CreateVertexFormat("EGFVertexGameObj", gameObjInstEgfLayout);
	}

	{
		g_studioCache->PrecacheModel("models/error.egf");
		CFileSystemFind fsFind("models/*.egf", SP_MOD);
		while (fsFind.Next())
		{
			const int modelIdx = g_studioCache->PrecacheModel(EqString::Format("models/%s", fsFind.GetPath()));
			Msg("model %d %s\n", modelIdx, fsFind.GetPath());
			CreateDrawArchetypeEGF(*g_studioCache->GetModel(modelIdx), 1);
		}
	}

	InitIndirectRenderer();

	InitGame();
}

// when the state changes to something
// @to - used to transfer data
void CState_GpuDrivenDemo::OnLeave(CAppStateBase* to)
{
	TermIndirectRenderer();

	g_renderAPI->DestroyVertexFormat(s_gameObjectVF);
	s_gameObjectVF = nullptr;

	g_inputCommandBinder->UnbindCommandByName("forward");
	g_inputCommandBinder->UnbindCommandByName("backward");
	g_inputCommandBinder->UnbindCommandByName("strafeleft");
	g_inputCommandBinder->UnbindCommandByName("straferight");

	s_instanceMng.Shutdown();
	g_pfxRender->Shutdown();

	g_studioCache->ReleaseCache();
}

void CState_GpuDrivenDemo::InitGame()
{
	inst_update.SetBool(true);

	// distribute instances randomly
	const int modelCount = g_studioCache->GetModelCount();

	s_instanceMng.FreeAll(false, true);
	s_objects.clear();

	for (int i = 0; i < inst_count.GetInt(); ++i)
	{
		const int rndModelIdx = (i % (modelCount - 1)) + 1;

		auto it = s_modelIdToArchetypeId.find(g_studioCache->GetModel(rndModelIdx)->GetCacheId());
		if (it.atEnd())
		{
			ASSERT_FAIL("Can't get archetype for model idx = %d", rndModelIdx);
			continue;
		}

		Object& obj = s_objects.append();
		obj.instId = s_instanceMng.AddInstance<InstTransform>(*it);
		obj.trs.t = Vector3D(RandomFloat(-2900, 2900), RandomFloat(-100, 100), RandomFloat(-2900, 2900));
		obj.trs.r = rotateXYZ(RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F));

		obj.trs.t *= 0.25f;

		// update visual
		InstTransform transform;
		transform.position = obj.trs.t;
		transform.orientation = obj.trs.r;
		s_instanceMng.Set(obj.instId, transform);
	}
}

void CState_GpuDrivenDemo::StepGame(float fDt)
{
	PROF_EVENT_F();

	Vector3D forward, right;
	AngleVectors(s_currentView.GetAngles(), &forward, &right);

	Vector3D moveVec = vec3_zero;

	if (m_cameraButtons & CAM_FORWARD)
		moveVec += forward;
	if(m_cameraButtons & CAM_BACKWARD)
		moveVec -= forward;
	if(m_cameraButtons & CAM_SIDE_LEFT)
		moveVec -= right;
	if(m_cameraButtons & CAM_SIDE_RIGHT)
		moveVec += right;

	if(length(moveVec) > F_EPS)
		moveVec = normalize(moveVec);

	s_currentView.SetOrigin(s_currentView.GetOrigin() + moveVec * cam_speed.GetFloat() * fDt);
	s_currentView.SetFOV(cam_fov.GetFloat());

	debugoverlay->Text(color_white, "Mode: %s", inst_use_compute.GetBool() ? "Compute" : "CPU");
	debugoverlay->Text(color_white, "Object count: %d", s_objects.numElem());
	if(obj_rotate.GetBool())
	{
		// rotate
		for (int i = 0; i < s_objects.numElem(); i += obj_rotate_interval.GetInt())
		{
			Object& obj = s_objects[i];

			obj.trs.r = obj.trs.r * rotateXYZ(DEG2RAD(35) * fDt, DEG2RAD(25) * fDt, DEG2RAD(5) * fDt);

			InstTransform transform;
			transform.position = obj.trs.t;
			transform.orientation = obj.trs.r;
			s_instanceMng.Set(obj.instId, transform);
		}
	}
}

// when 'false' returned the next state goes on
bool CState_GpuDrivenDemo::Update(float fDt)
{
	const IVector2D screenSize = g_pHost->GetWindowSize();

	Matrix4x4 projMat, viewMat;
	s_currentView.GetMatrices(projMat, viewMat, screenSize.x, screenSize.y, 0.1f, 10000.0f);

	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, projMat);
	g_matSystem->SetMatrix(MATRIXMODE_VIEW, viewMat);
	debugoverlay->SetMatrices(projMat, viewMat);

	DbgLine()
		.Start(vec3_zero)
		.End(vec3_right)
		.Color(color_red);

	DbgLine()
		.Start(vec3_zero)
		.End(vec3_up)
		.Color(color_green);

	DbgLine()
		.Start(vec3_zero)
		.End(vec3_forward)
		.Color(color_blue);

	DbgSphere()
		.Position(vec3_zero)
		.Radius(7000.0f)
		.Color(color_white);

	StepGame(fDt);

	// RenderGame
	{
		IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder();
		g_pfxRender->UpdateBuffers(cmdRecorder);

		s_instanceMng.SyncInstances(cmdRecorder);
		if (inst_update.GetBool())
		{
			const Matrix4x4 viewProj = projMat * viewMat;
			Volume frustum;
			frustum.LoadAsFrustum(viewProj);

			s_instanceIdsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_objects.numElem()), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceIds");

			if (inst_use_compute.GetBool())
			{
				const int numBounds = s_drawLodsList.numSlots() * MAX_INSTANCE_LODS;
				IGPUBufferPtr sortedKeysBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_objects.numElem() + 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "SortedKeys");
				IGPUBufferPtr instanceInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUInstanceInfo), s_objects.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceInfos");
				IGPUBufferPtr drawInstanceBoundsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUInstanceBound), numBounds + 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceBounds");

				cmdRecorder->ClearBuffer(drawInstanceBoundsBuffer, 0, drawInstanceBoundsBuffer->GetSize());
				cmdRecorder->WriteBuffer(drawInstanceBoundsBuffer, &numBounds, sizeof(int), 0);

				VisibilityCullInstances_Compute(s_currentView.GetOrigin(), frustum, cmdRecorder, sortedKeysBuffer, instanceInfosBuffer);
				SortInstances_Compute(instanceInfosBuffer, cmdRecorder, sortedKeysBuffer);
				UpdateInstanceBounds_Compute(sortedKeysBuffer, instanceInfosBuffer, cmdRecorder, s_instanceIdsBuffer, drawInstanceBoundsBuffer);
				UpdateIndirectInstances_Compute(drawInstanceBoundsBuffer, cmdRecorder);
			}
			else
			{
				Array<GPUInstanceInfo> instanceInfos(PP_SL);
				VisibilityCullInstances_Software(s_currentView.GetOrigin(), frustum, instanceInfos);
				SortInstances_Software(instanceInfos);

				Array<GPUInstanceBound> drawInstanceBounds(PP_SL);
				UpdateInstanceBounds_Software(instanceInfos, cmdRecorder, s_instanceIdsBuffer, drawInstanceBounds);
				UpdateIndirectInstances_Software(drawInstanceBounds, cmdRecorder);
			}

			if(inst_update_once.GetBool())
				inst_update.SetBool(false);
		}

		{
			IGPURenderPassRecorderPtr rendPassRecorder = cmdRecorder->BeginRenderPass(
				Builder<RenderPassDesc>()
				.ColorTarget(g_matSystem->GetCurrentBackbuffer())
				.DepthStencilTarget(g_matSystem->GetDefaultDepthBuffer())
				.DepthClear()
				.End()
			);

			const RenderPassContext rendPassCtx(rendPassRecorder, nullptr);
			DrawScene(rendPassCtx);
			g_pfxRender->Render(rendPassCtx, nullptr);

			rendPassRecorder->Complete();
		}
		g_matSystem->QueueCommandBuffer(cmdRecorder->End());
	}

	return true;
}

void CState_GpuDrivenDemo::HandleKeyPress(int key, bool down)
{
	g_inputCommandBinder->OnKeyEvent(key, down);
}

void CState_GpuDrivenDemo::HandleMouseClick(int x, int y, int buttons, bool down)
{
	if (g_consoleInput->IsVisible())
		return;
	g_inputCommandBinder->OnMouseEvent(buttons, down);
}

void CState_GpuDrivenDemo::HandleMouseMove(int x, int y, float deltaX, float deltaY)
{
	if (g_consoleInput->IsVisible())
		return;
	const Vector3D camAngles = s_currentView.GetAngles();
	s_currentView.SetAngles(camAngles + Vector3D(deltaY, deltaX, 0.0f));
}

void CState_GpuDrivenDemo::HandleMouseWheel(int x, int y, int scroll)
{
	g_inputCommandBinder->OnMouseWheel(scroll);
}

void CState_GpuDrivenDemo::HandleJoyAxis(short axis, short value)
{
	g_inputCommandBinder->OnJoyAxisEvent(axis, value);
}

void CState_GpuDrivenDemo::GetMouseCursorProperties(bool& visible, bool& centered) 
{
	visible = false; 
	centered = true; 
}