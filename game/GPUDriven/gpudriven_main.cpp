#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
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
#include "input/InputCommandBinder.h"
#include "studio/StudioGeom.h"
#include "studio/StudioCache.h"
#include "instancer.h"

DECLARE_CVAR(cam_speed, "100", nullptr, 0);
DECLARE_CVAR(inst_count, "10000", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update, "1", nullptr, 0);
DECLARE_CVAR(lod_idx, "0", nullptr, 0);
DECLARE_CVAR(obj_rotate_interval, "100", nullptr, 0);
DECLARE_CVAR(obj_rotate, "1", nullptr, 0);

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

// NOTE:	draw archetype != instance archetype
//			instance archetype == GPUDrawArchetypeSet index

struct GPUDrawIndirectCmd
{
	uint vertexCount{ 0 };
	uint instanceCount{ 0 };
	uint firstVertex{ 0 };
	uint firstInstance{ 0 };
};

struct GPUDrawIndexedIndirectCmd
{
	uint indexCount{ 0 };
	uint instanceCount{ 0 };
	uint firstIndex{ 0 };
	uint baseVertex{ 0 };
	uint firstInstance{ 0 };
};

struct GPUDrawBatch
{
	IGPUBufferPtr			vertexBuffers[MAX_VERTEXSTREAM - 1];
	IGPUBufferPtr			indexBuffer;
	MeshInstanceFormatRef	meshInstFormat;

	int				indexCount{ 0 };
	int				firstIndex{ 0 };
	int				materialIdx{ -1 };

	EPrimTopology	primTopology{ PRIM_TRIANGLES };
	EIndexFormat	indexFormat{ 0 };
	int				cmdIdx{ -1 };
};

struct GPULodInfo
{
	int				firstBatch{ 0 };
	int				numBatches{ 0 };
	float			distance{ 0.0f };
};

struct GPUDrawArchetype
{
	Array<GPUDrawBatch>	batchs{ PP_SL };
	Array<GPULodInfo>	lods{ PP_SL };
};

static Map<int, int>			s_modelIdToArchetypeId{ PP_SL };
static Array<GPUDrawArchetype>	s_drawArchetypeSets{ PP_SL };
//static Array<GPUDrawBatch>		s_drawBatchs{ PP_SL };
//static Array<GPULodInfo>		s_drawLods{ PP_SL };
//static Array<int>				s_drawFirstLod{ PP_SL };
static Array<IMaterialPtr>		s_drawArchetypeMaterials{ PP_SL };
static IGPUBufferPtr			s_drawInvocationsSrcBuffer;
static IGPUBufferPtr			s_drawInvocationsBuffer;
static IGPUBufferPtr			s_instanceIdsBuffer;

static void TermIndirectRenderer()
{
	s_modelIdToArchetypeId.clear(true);
	s_drawArchetypeSets.clear(true);
	//s_drawBatchs.clear(true);
	//s_drawLods.clear(true);
	//s_drawFirstLod.clear(true);
	s_drawArchetypeMaterials.clear(true);
	s_drawInvocationsBuffer = nullptr;
	s_instanceIdsBuffer = nullptr;
}

static int InitDrawArchetype(const CEqStudioGeom& geom, uint bodyGroupFlags = 0, int materialGroupIdx = 0)
{
	ASSERT(bodyGroupFlags != 0);

	IVertexFormat* vertFormat = s_gameObjectVF;
	IGPUBufferPtr buffer0 = geom.GetVertexBuffer(EGFHwVertex::VERT_POS_UV);
	IGPUBufferPtr buffer1 = geom.GetVertexBuffer(EGFHwVertex::VERT_TBN); 
	IGPUBufferPtr buffer2 = geom.GetVertexBuffer(EGFHwVertex::VERT_BONEWEIGHT);
	IGPUBufferPtr buffer3 = geom.GetVertexBuffer(EGFHwVertex::VERT_COLOR);
	IGPUBufferPtr indexBuffer = geom.GetIndexBuffer();

	const int setIdx = s_drawArchetypeSets.numElem();
	const int firstMaterialIdx = s_drawArchetypeMaterials.numElem();

	// TODO: multiple material groups require new archetype
	// also body groups are really are different archetypes for EGF
	ArrayCRef<IMaterialPtr> materials = geom.GetMaterials(materialGroupIdx);
	for(IMaterialPtr material : materials)
		s_drawArchetypeMaterials.append(material);

	ArrayCRef<CEqStudioGeom::HWGeomRef> geomRefs = geom.GetHwGeomRefs();
	const studioHdr_t& studio = geom.GetStudioHdr();

	GPUDrawArchetype& drawSet = s_drawArchetypeSets.append();
	for (int i = 0; i < studio.numLodParams; i++)
	{
		const studioLodParams_t* lodParam = studio.pLodParams(i);
		GPULodInfo& lodInfo = drawSet.lods.append();
		lodInfo.firstBatch = drawSet.batchs.numElem();
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
	
			const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);
			for (int k = 0; k < modDesc->numMeshes; ++k)
			{
				const CEqStudioGeom::HWGeomRef::MeshRef& meshRef = geomRefs[modelDescId].meshRefs[k];
	
				GPUDrawBatch& drawArch = drawSet.batchs.append();
				drawArch.firstIndex = meshRef.firstIndex;
				drawArch.indexCount = meshRef.indexCount;
				drawArch.primTopology = (EPrimTopology)meshRef.primType;
				drawArch.materialIdx = firstMaterialIdx + meshRef.materialIdx;
				drawArch.indexFormat = (EIndexFormat)geom.GetIndexFormat();

				drawArch.meshInstFormat.name = vertFormat->GetName();
				drawArch.meshInstFormat.formatId = vertFormat->GetNameHash();
				drawArch.meshInstFormat.layout = vertFormat->GetFormatDesc();

				drawArch.vertexBuffers[0] = buffer0;
				drawArch.vertexBuffers[1] = buffer1;
				drawArch.vertexBuffers[2] = buffer2;
				drawArch.vertexBuffers[3] = buffer3;
				drawArch.indexBuffer = indexBuffer;
			}
		}

		lodInfo.numBatches = drawSet.batchs.numElem() - lodInfo.firstBatch;
	}

	// TODO: body group lookup
	s_modelIdToArchetypeId.insert(geom.GetCacheId(), setIdx);
	return setIdx;
}

static void UpdateIndirectInstances(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr& indirectDrawBuffer, IGPUBufferPtr& instanceIdsBuffer)
{
	PROF_EVENT_F();

	int numAllDrawInvocations = 0;
	for (const GPUDrawArchetype& drawSet : s_drawArchetypeSets)
	{
		for (GPUDrawBatch& drawArch : drawSet.batchs)
			drawArch.cmdIdx = numAllDrawInvocations++;
	}
	
	indirectDrawBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GPUDrawIndexedIndirectCmd), numAllDrawInvocations), BUFFERUSAGE_INDIRECT | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST);
	instanceIdsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), s_objects.numElem()), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST);
	cmdRecorder->ClearBuffer(indirectDrawBuffer, 0, indirectDrawBuffer->GetSize());

	// BEGIN COMPUTE IMPL REFERENCE

	// sort instances by archetype
	struct InstanceInfo
	{
		int instanceId;
		int archetypeId;
	};

	// collect visible instances
	Array<InstanceInfo> instanceInfos(PP_SL);
	for (const Object& obj : s_objects)
	{
		const int archetypeId = s_instanceMng.GetInstanceArchetypeId(obj.instId);
		if (archetypeId == -1)
			continue;
		instanceInfos.append({ obj.instId, archetypeId });
	}

	// sort instances by archetype id
	arraySort(instanceInfos, [](const InstanceInfo& a, const InstanceInfo& b) {
		return a.archetypeId - b.archetypeId;
	});

	Array<int> instanceIds(PP_SL);
	int lastArchetypeId = -1;
	int lastArchetypeIdStart = 0;


	for (int i = 0; i < instanceInfos.numElem(); ++i)
	{
		const InstanceInfo& instInfo = instanceInfos[i];
		if (lastArchetypeId != instInfo.archetypeId && lastArchetypeId != -1)
		{
			const GPUDrawArchetype& drawSet = s_drawArchetypeSets[instInfo.archetypeId];

			const int lodIdx = min(drawSet.lods.numElem() - 1, lod_idx.GetInt());
			const GPULodInfo& lodInfo = drawSet.lods[lodIdx];

			for (const GPUDrawBatch& drawArch : ArrayCRef(drawSet.batchs.ptr() + lodInfo.firstBatch, lodInfo.numBatches))
			{
				GPUDrawIndexedIndirectCmd drawCmd;
				drawCmd.firstIndex = drawArch.firstIndex;
				drawCmd.indexCount = drawArch.indexCount;
				drawCmd.firstInstance = instanceIds.numElem();
				drawCmd.instanceCount = instanceIds.numElem() - lastArchetypeIdStart;
				
				ASSERT(drawCmd.instanceCount > 1);

				cmdRecorder->WriteBuffer(indirectDrawBuffer, &drawCmd, sizeof(drawCmd), sizeof(GPUDrawIndexedIndirectCmd) * drawArch.cmdIdx);
			}

			lastArchetypeIdStart = instanceIds.numElem();
		}
		lastArchetypeId = instInfo.archetypeId;
		instanceIds.append(instInfo.instanceId);
	}

	cmdRecorder->WriteBuffer(instanceIdsBuffer, instanceIds.ptr(), sizeof(instanceIds[0]) * instanceIds.numElem(), 0);
}

static void DrawScene(const RenderPassContext& renderPassCtx)
{
	PROF_EVENT_F();

	int numDrawCalls = 0;
	for (const GPUDrawArchetype& drawSet : s_drawArchetypeSets)
	{
		for (const GPUDrawBatch& drawArch : drawSet.batchs)
		{
			IMaterial* material = s_drawArchetypeMaterials[drawArch.materialIdx];
			if (!g_matSystem->SetupMaterialPipeline(material, nullptr, drawArch.primTopology, drawArch.meshInstFormat, renderPassCtx, &s_instanceMng))
				continue;

			renderPassCtx.recorder->SetVertexBuffer(0, drawArch.vertexBuffers[0]);
			renderPassCtx.recorder->SetVertexBuffer(1, s_instanceIdsBuffer);
			renderPassCtx.recorder->SetIndexBuffer(drawArch.indexBuffer, drawArch.indexFormat);

			renderPassCtx.recorder->DrawIndexedIndirect(s_drawInvocationsBuffer, sizeof(GPUDrawIndexedIndirectCmd) * drawArch.cmdIdx);
			++numDrawCalls;
		}
	}

	debugoverlay->Text(color_white, "--- instances summary ---");
	debugoverlay->Text(color_white, "Archetypes: %d", s_drawArchetypeSets.numElem());
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
			InitDrawArchetype(*g_studioCache->GetModel(modelIdx), 1);
		}
	}

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
			UpdateIndirectInstances(cmdRecorder, s_drawInvocationsBuffer, s_instanceIdsBuffer);
			//inst_update.SetBool(false);
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
	g_inputCommandBinder->OnMouseEvent(buttons, down);
}

void CState_GpuDrivenDemo::HandleMouseMove(int x, int y, float deltaX, float deltaY)
{
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