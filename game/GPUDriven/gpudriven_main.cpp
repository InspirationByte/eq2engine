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
#include "render/ComputeBitonicMergeSort.h"
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
DECLARE_CVAR(inst_use_compute, "0", nullptr, 0);
DECLARE_CVAR(inst_use_gpu_sort, "0", nullptr, 0);
DECLARE_CVAR(inst_use_gpu_starts, "1", nullptr, 0);

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

struct DrawParams
{
	int		lodIdxOverride;
	float	lodDistanceBias;
};

static Map<int, int>					s_modelIdToArchetypeId{ PP_SL };
static SlottedArray<GPUDrawInfo>		s_drawInfos{ PP_SL };	// must be in sync with batchs
static SlottedArray<GPUIndexedBatch>	s_drawBatchs{ PP_SL };
static SlottedArray<GPULodInfo>			s_drawLodInfos{ PP_SL };
static SlottedArray<GPULodList>			s_drawLodsList{ PP_SL };

static IGPUBufferPtr					s_drawBatchsBuffer;
static IGPUBufferPtr					s_drawLodInfosBuffer;
static IGPUBufferPtr					s_drawLodsListBuffer;
static IGPUBufferPtr					s_drawParamsBuffer;

static IGPUBufferPtr					s_drawInvocationsBuffer;
static IGPUBufferPtr					s_instanceIdsBuffer;		// sorted ready to draw instances
static ComputeBitonicMergeSortShaderPtr	s_mergeSortShader;
static IGPUComputePipelinePtr			s_calcArchetypeIdStartsPipeline;
static IGPUComputePipelinePtr			s_updateIndirectInstancesPipeline;

static IGPUBindGroupPtr		s_updateBindGroup0;

static constexpr EqStringRef INSTANCE_ARCHETYPE_SORT_SHADERNAME = "ComputeInstanceArchetypeSort";
static constexpr EqStringRef UPDATE_INDIRECT_INSTANCES_SHADERNAME = "ComputePrepareDrawIndirectInstances";

static void InitIndirectRenderer()
{
	s_mergeSortShader = CRefPtr_new(ComputeBitonicMergeSortShader);
	s_mergeSortShader->AddSortPipeline("SortInstanceInfos", INSTANCE_ARCHETYPE_SORT_SHADERNAME);

	s_drawBatchsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUIndexedBatch), s_drawBatchs.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawBatchs");
	s_drawLodInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPULodInfo), s_drawLodInfos.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodInfos");
	s_drawLodsListBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(s_drawLodsList), s_drawLodsList.numSlots()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawLodsList");
	s_drawParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(DrawParams), 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawParams");

	s_drawBatchsBuffer->Update(s_drawBatchs.ptr(), s_drawBatchs.numSlots() * sizeof(s_drawBatchs[0]), 0);
	s_drawLodInfosBuffer->Update(s_drawLodInfos.ptr(), s_drawLodInfos.numSlots() * sizeof(s_drawLodInfos[0]), 0);
	s_drawLodsListBuffer->Update(s_drawLodsList.ptr(), s_drawLodsList.numSlots() * sizeof(s_drawLodsList[0]), 0);

	s_drawInvocationsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUDrawIndexedIndirectCmd), s_drawInfos.numSlots()), BUFFERUSAGE_INDIRECT | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "DrawInvocations");
	s_calcArchetypeIdStartsPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderLayoutId(StringToHashConst("CalcArchetypeIdStarts"))
		.ShaderName(INSTANCE_ARCHETYPE_SORT_SHADERNAME)
		.End()
	);

	s_updateIndirectInstancesPipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName(UPDATE_INDIRECT_INSTANCES_SHADERNAME)
		.End()
	);

	s_updateBindGroup0 = g_renderAPI->CreateBindGroup(s_updateIndirectInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, s_drawBatchsBuffer)
		.Buffer(1, s_drawLodInfosBuffer)
		.Buffer(2, s_drawLodsListBuffer)
		.Buffer(3, s_drawParamsBuffer)
		.End());
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
	s_drawParamsBuffer = nullptr;
	s_drawInvocationsBuffer = nullptr;
	s_instanceIdsBuffer = nullptr;
	s_mergeSortShader = nullptr;
	s_calcArchetypeIdStartsPipeline = nullptr;
	s_updateIndirectInstancesPipeline = nullptr;
	s_updateBindGroup0 = nullptr;
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

static void SortInstances_Compute(ArrayCRef<GPUInstanceInfo> instanceInfos, IGPUBufferPtr instanceInfosBuffer, IGPUBufferPtr archetypeIdStartsBuffer, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr sortedKeysBuffer)
{
	Array<int> sortedKeys(PP_SL);

	const int keysCount = instanceInfos.numElem();
	if (inst_use_gpu_sort.GetBool())
	{
		s_mergeSortShader->InitKeys(cmdRecorder, sortedKeysBuffer, keysCount);
		s_mergeSortShader->SortKeys(StringToHashConst("SortInstanceInfos"), cmdRecorder, sortedKeysBuffer, keysCount, instanceInfosBuffer);
	}
	else
	{
		for (int i = 0; i < keysCount; ++i)
			sortedKeys.append(i);

		arraySort(sortedKeys, [&](const int a, const int b) {
			return instanceInfos[a].packedArchetypeId - instanceInfos[b].packedArchetypeId;
		});
		sortedKeys.insert(keysCount, 0);

		cmdRecorder->WriteBuffer(sortedKeysBuffer, sortedKeys.ptr(), sortedKeys.numElem() * sizeof(sortedKeys[0]), 0);
	}

	IGPUComputePassRecorderPtr computeRecorder = cmdRecorder->BeginComputePass("CalcArchetypeIdStarts");
	computeRecorder->SetPipeline(s_calcArchetypeIdStartsPipeline);
	computeRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(s_calcArchetypeIdStartsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, instanceInfosBuffer)
		.Buffer(1, sortedKeysBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(s_calcArchetypeIdStartsPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, archetypeIdStartsBuffer)
		.End())
	);
	computeRecorder->DispatchWorkgroups(1);
	computeRecorder->Complete();

}

static void UpdateIndirectInstances_Compute(IGPUBufferPtr instanceInfosBuffer, IGPUBufferPtr archetypeIdStartsBuffer, IGPUBufferPtr sortedKeysBuffer, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr indirectDrawBuffer, IGPUBufferPtr instanceIdsBuffer)
{
	DrawParams drawParams{ lod_idx.GetInt(), 1.0f };
	cmdRecorder->WriteBuffer(s_drawParamsBuffer, &drawParams, sizeof(drawParams), 0);

	IGPUComputePassRecorderPtr computeRecorder = cmdRecorder->BeginComputePass("UpdateIndirectInstances");
	computeRecorder->SetPipeline(s_updateIndirectInstancesPipeline);
	computeRecorder->SetBindGroup(0, s_updateBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(s_updateIndirectInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, sortedKeysBuffer)
		.Buffer(1, instanceInfosBuffer)
		.Buffer(2, archetypeIdStartsBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(s_updateIndirectInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, instanceIdsBuffer)
		.Buffer(1, indirectDrawBuffer)
		.End())
	);

	constexpr int GROUP_SIZE = 32;

	computeRecorder->DispatchWorkgroups(s_drawLodsList.numElem() / GROUP_SIZE + 1);
	computeRecorder->Complete();
}

//--------------------------------------------------------------------

static void VisibilityCullInstances_Software(const Vector3D& camPos, const Volume& frustum, Array<GPUInstanceInfo>& instanceInfos)
{
	PROF_EVENT_F();

	// COMPUTE SHADER REFERENCE: VisibilityCullInstances
	// Input:
	//		instanceIds		: buffer<int>
	// Output:
	//		instanceInfos	: buffer<GPUInstanceInfo>

	instanceInfos.reserve(s_objects.numElem());

	for (const Object& obj : s_objects)
	{
		const int archetypeId = s_instanceMng.GetInstanceArchetypeId(obj.instId);
		if (archetypeId == -1)
			continue;

		if (!frustum.IsSphereInside(obj.trs.t, 0.1))
			continue;

		const GPULodList& lodList = s_drawLodsList[archetypeId];
		const float distFromCamera = distanceSqr(camPos, obj.trs.t);

		// find suitable lod idx
		int drawLod = -1;
		for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = s_drawLodInfos[lodIdx].next)
		{
			if (distFromCamera < sqr(s_drawLodInfos[lodIdx].distance))
				break;
			drawLod++;
		}

		instanceInfos.append({ obj.instId, archetypeId | (drawLod << 24) });
	}
}

static void SortInstances_Software(Array<GPUInstanceInfo>& instanceInfos, Array<int>& archetypeIdStarts)
{
	PROF_EVENT_F();

	// COMPUTE SHADER REFERENCE: SortInstanceArchetypes
	// Input / Output:
	//		instanceInfos		: buffer<GPUInstanceInfo[]>
	//		archetypeIdStarts	: buffer<int[]>
	//

	// sort instances by archetype
	// as we need them to be linear for each draw call
	arraySort(instanceInfos, [](const GPUInstanceInfo& a, const GPUInstanceInfo& b) {
		return a.packedArchetypeId - b.packedArchetypeId;
	});

	const int instanceCount = instanceInfos.numElem();

	int lastArchetypeId = -1;
	int lastArchetypeIdStart = 0;
	for (int i = 0; i < instanceCount; ++i)
	{
		if (lastArchetypeId != -1 && lastArchetypeId != instanceInfos[i].packedArchetypeId)
		{
			archetypeIdStarts.append(lastArchetypeIdStart);
			lastArchetypeIdStart = i;
		}
		lastArchetypeId = instanceInfos[i].packedArchetypeId;
	}

	if (lastArchetypeId != -1)
		archetypeIdStarts.append(lastArchetypeIdStart);
}

static void UpdateIndirectInstances_Software(ArrayCRef<GPUInstanceInfo> instanceInfos, ArrayCRef<int> archetypeIdStarts, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr indirectDrawBuffer, IGPUBufferPtr instanceIdsBuffer)
{
	PROF_EVENT_F();

	const int instanceCount = instanceInfos.numElem();
	Array<int> instanceIds(PP_SL);

	// COMPUTE SHADER REFERENCE: PrepareDrawIndirectInstances
	// Input:
	//		instanceInfos		: buffer<GPUInstanceInfo[]>
	//		archetypeIdStarts	: buffer<int[]>
	// Output:
	//		instanceIds			: buffer<int[]>
	//		indirectDraws		: buffer<GPUDrawIndexedIndirectCmd[]>
	//
	// Notes:
	//		instanceInfos must be sorted by archetype id prior to executing

	instanceIds.setNum(instanceCount);

	for (int i = 0; i < archetypeIdStarts.numElem(); ++i)
	{
		const int firstInstance = archetypeIdStarts[i];
		const int lastInstance = (i + 1 < archetypeIdStarts.numElem()) ? archetypeIdStarts[i + 1] : instanceCount;

		const int archetypeId = instanceInfos[firstInstance].packedArchetypeId & GPUInstanceInfo::ARCHETYPE_MASK;
		const int archLodIndex = (instanceInfos[firstInstance].packedArchetypeId >> GPUInstanceInfo::ARCHETYPE_BITS) & GPUInstanceInfo::LOD_MASK;

		const GPULodList& lodList = s_drawLodsList[archetypeId];

		// fill lod list
		int numLods = 0;
		int lodInfos[MAX_MODEL_LODS];
		for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = s_drawLodInfos[lodIdx].next)
			lodInfos[numLods++] = lodIdx;

		// walk over batches
		const int lodIdx = min(numLods - 1, lod_idx.GetInt() == -1 ? archLodIndex : lod_idx.GetInt());

		const GPULodInfo& lodInfo = s_drawLodInfos[lodInfos[lodIdx]];
		for (int batchIdx = lodInfo.firstBatch; batchIdx != -1; batchIdx = s_drawBatchs[batchIdx].next)
		{
			const GPUIndexedBatch& drawBatch = s_drawBatchs[batchIdx];

			GPUDrawIndexedIndirectCmd drawCmd;
			drawCmd.firstIndex = drawBatch.firstIndex;
			drawCmd.indexCount = drawBatch.indexCount;
			drawCmd.firstInstance = firstInstance;
			drawCmd.instanceCount = lastInstance - firstInstance;

			// indirectDrawBuffer[drawBatch.cmdIdx] = drawCmd;
			cmdRecorder->WriteBuffer(indirectDrawBuffer, &drawCmd, sizeof(drawCmd), sizeof(GPUDrawIndexedIndirectCmd) * drawBatch.cmdIdx);
		}

		for (int j = firstInstance; j < lastInstance; ++j)
			instanceIds[j] = instanceInfos[j].instanceId;
	}

	cmdRecorder->WriteBuffer(instanceIdsBuffer, instanceIds.ptr(), sizeof(instanceIds[0]) * instanceIds.numElem(), 0);
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
		obj.trs.t = Vector3D(RandomFloat(-900, 900), RandomFloat(-100, 100), RandomFloat(-900, 900));
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

	debugoverlay->Text(color_white, "Object count: % d", s_objects.numElem());
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

			Array<GPUInstanceInfo> instanceInfos(PP_SL);
			VisibilityCullInstances_Software(s_currentView.GetOrigin(), frustum, instanceInfos);

			s_instanceIdsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_objects.numElem()), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceIds");
			cmdRecorder->ClearBuffer(s_drawInvocationsBuffer, 0, s_drawInvocationsBuffer->GetSize());

			if (inst_use_compute.GetBool())
			{
				const int instanceCount = instanceInfos.numElem();
				IGPUBufferPtr instanceInfosBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUInstanceInfo), s_objects.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstanceInfos");
				cmdRecorder->WriteBuffer(instanceInfosBuffer, instanceInfos.ptr(), instanceCount * sizeof(instanceInfos[0]), 0);
				
				IGPUBufferPtr sortedKeysBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_objects.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "SortedKeys");
				cmdRecorder->WriteBuffer(sortedKeysBuffer, &instanceCount, sizeof(int), 0);

				IGPUBufferPtr archetypeIdStartsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_drawLodInfos.numElem() + 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "ArchetypeIdStarts");
				SortInstances_Compute(instanceInfos, instanceInfosBuffer, archetypeIdStartsBuffer, cmdRecorder, sortedKeysBuffer);
				UpdateIndirectInstances_Compute(instanceInfosBuffer, archetypeIdStartsBuffer, sortedKeysBuffer, cmdRecorder, s_drawInvocationsBuffer, s_instanceIdsBuffer);
			}
			else
			{
				Array<int> archetypeIdStarts(PP_SL);
				SortInstances_Software(instanceInfos, archetypeIdStarts);
				UpdateIndirectInstances_Software(instanceInfos, archetypeIdStarts, cmdRecorder, s_drawInvocationsBuffer, s_instanceIdsBuffer);
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