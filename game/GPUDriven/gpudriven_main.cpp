#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
#include "math/Random.h"

#include "gpudriven_main.h"
#include "instancer.h"

#include "sys/sys_host.h"
#include "sys/sys_in_console.h"

#include "audio/eqSoundEmitterSystem.h"
#include "materialsystem1/IMaterialSystem.h"

#include "render/EqParticles.h"
#include "render/IDebugOverlay.h"
#include "render/ViewParams.h"
#include "render/ComputeSort.h"
#include "input/InputCommandBinder.h"
#include "studio/StudioGeom.h"
#include "studio/StudioCache.h"


DECLARE_CVAR(cam_speed, "100", nullptr, 0);
DECLARE_CVAR(cam_fov, "90", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_count, "10000", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update_once, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(obj_rotate_interval, "100", 1.0, 1000, nullptr, 0);
DECLARE_CVAR(obj_rotate, "1", nullptr, 0);
DECLARE_CVAR(obj_draw, "1", nullptr, 0);

DECLARE_CVAR(lod_override, "-1", nullptr, 0);

static DemoGRIMInstanceAllocator	s_instanceAlloc;
static DemoGRIMRenderer				s_grimRenderer(s_instanceAlloc);

struct DemoRenderState : public GRIMRenderState
{
	Vector3D		viewPos;
	Volume			frustum;
};

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

//--------------------------------------------

struct Object
{
	Transform3D trs;
	int instId{ -1 };
};
static Array<Object>			s_objects{ PP_SL };
static Map<int, GRIMArchetype>	s_modelIdToArchetypeId{ PP_SL };
static GRIMRenderState			s_storedRenderState;

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

void DemoGRIMRenderer::VisibilityCullInstances_Compute(IntermediateState& intermediate)
{
	PROF_EVENT_F();

	DemoRenderState& renderState = static_cast<DemoRenderState&>(intermediate.renderState);

	struct CullViewParams
	{
		Vector4D	frustumPlanes[6];
		Vector3D	viewPos;
		int			overrideLodIdx;
		int			maxInstanceIds;
		float		_padding[3];
	};

	CullViewParams cullView;
	memcpy(cullView.frustumPlanes, renderState.frustum.GetPlanes().ptr(), sizeof(cullView.frustumPlanes));
	cullView.viewPos = renderState.viewPos;
	cullView.overrideLodIdx = lod_override.GetInt();
	cullView.maxInstanceIds = m_instAllocator.GetInstanceSlotsCount();	// TODO: calculate between frames

	IGPUBufferPtr viewParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(CullViewParams), 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "ViewParamsBuffer");
	intermediate.cmdRecorder->WriteBuffer(viewParamsBuffer, &cullView, sizeof(cullView), 0);
	intermediate.cmdRecorder->ClearBuffer(intermediate.sortedInstanceIds, 0, sizeof(int));

	IGPUComputePassRecorderPtr computeRecorder = intermediate.cmdRecorder->BeginComputePass("CullInstances");
	computeRecorder->SetPipeline(m_cullInstancesPipeline);
	computeRecorder->SetBindGroup(0, m_cullBindGroup0);
	computeRecorder->SetBindGroup(1, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Buffer(0, m_instAllocator.GetInstanceArchetypesBuffer())
		.Buffer(1, viewParamsBuffer)
		.End())
	);
	computeRecorder->SetBindGroup(2, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline,
		Builder<BindGroupDesc>()
		.GroupIndex(2)
		.Buffer(0, intermediate.instanceInfosBuffer)
		.Buffer(1, intermediate.sortedInstanceIds)
		.End())
	);

	computeRecorder->SetBindGroup(3, g_renderAPI->CreateBindGroup(m_cullInstancesPipeline, Builder<BindGroupDesc>()
		.GroupIndex(3)
		.Buffer(0, m_instAllocator.GetRootBuffer())
		.Buffer(1, m_instAllocator.GetDataPoolBuffer(InstTransform::COMPONENT_ID))
		.End())
	);

	int x, y, z;
	VisCalcWorkSize(m_instAllocator.GetInstanceSlotsCount(), x, y, z);

	computeRecorder->DispatchWorkgroups(x, y , z);
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

	instanceInfos.reserve(s_objects.numElem());

	for (const Object& obj : s_objects)
	{
		const int archetypeId = m_instAllocator.GetInstanceArchetypeId(obj.instId);
		if (archetypeId == -1)
			continue;

		const GPULodList& lodList = m_drawLodsList[archetypeId];
		if (lodList.firstLodInfo < 0)
			continue;

		if (!frustum.IsSphereInside(obj.trs.t, 4.0f))
			continue;

		const float distFromCamera = distanceSqr(viewPos, obj.trs.t);

		// find suitable lod idx
		int drawLod = lod_override.GetInt();
		if (drawLod == -1)
		{
			for (int lodIdx = lodList.firstLodInfo; lodIdx != -1; lodIdx = m_drawLodInfos[lodIdx].next, ++drawLod)
			{
				if (distFromCamera < sqr(m_drawLodInfos[lodIdx].distance))
					break;
			}
		}

		instanceInfos.append({ obj.instId, archetypeId | (drawLod << GPUInstanceInfo::ARCHETYPE_BITS) });
	}
}

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
			CEqStudioGeom* geom = g_studioCache->GetModel(modelIdx);

			Msg("model %d %s\n", modelIdx, fsFind.GetPath());
			const GRIMArchetype archetypeId = s_grimRenderer.CreateStudioDrawArchetype(geom, s_gameObjectVF, 3);

			// TODO: body group lookup
			s_modelIdToArchetypeId.insert(geom->GetCacheId(), archetypeId);
		}
	}

	s_instanceAlloc.Initialize("InstanceUtils", INST_RESERVE);
	s_grimRenderer.Init();
	InitGame();
}

// when the state changes to something
// @to - used to transfer data
void CState_GpuDrivenDemo::OnLeave(CAppStateBase* to)
{
	s_grimRenderer.Shutdown();
	s_instanceAlloc.Shutdown();

	s_modelIdToArchetypeId.clear(true);
	s_storedRenderState = {};

	g_renderAPI->DestroyVertexFormat(s_gameObjectVF);
	s_gameObjectVF = nullptr;

	g_inputCommandBinder->UnbindCommandByName("forward");
	g_inputCommandBinder->UnbindCommandByName("backward");
	g_inputCommandBinder->UnbindCommandByName("strafeleft");
	g_inputCommandBinder->UnbindCommandByName("straferight");

	g_pfxRender->Shutdown();

	g_studioCache->ReleaseCache();
}

void CState_GpuDrivenDemo::InitGame()
{
	inst_update.SetBool(true);

	// distribute instances randomly
	const int modelCount = g_studioCache->GetModelCount();

	s_instanceAlloc.FreeAll(false, true);
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
		obj.instId = s_instanceAlloc.AddInstance<InstTransform>(*it);
		obj.trs.t = Vector3D(RandomFloat(-2900, 2900), RandomFloat(-100, 100), RandomFloat(-2900, 2900));
		obj.trs.r = rotateXYZ(RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F));

		obj.trs.t *= 0.25f;

		// update visual
		InstTransform transform;
		transform.position = obj.trs.t;
		transform.orientation = obj.trs.r;
		s_instanceAlloc.Set(obj.instId, transform);
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
			s_instanceAlloc.Set(obj.instId, transform);
		}
	}
}

// when 'false' returned the next state goes on
bool CState_GpuDrivenDemo::Update(float fDt)
{
	StepGame(fDt);
	RenderGame();

	return true;
}

void CState_GpuDrivenDemo::RenderGame()
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

	IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder();
	g_pfxRender->UpdateBuffers(cmdRecorder);

	s_instanceAlloc.SyncInstances(cmdRecorder);
	s_grimRenderer.SyncArchetypes(cmdRecorder);

	if (inst_update.GetBool())
	{
		DemoRenderState	renderState;
		const Matrix4x4 viewProjMat = projMat * viewMat;
		renderState.frustum.LoadAsFrustum(viewProjMat);
		renderState.viewPos = s_currentView.GetOrigin();

		s_grimRenderer.PrepareDraw(cmdRecorder, renderState, s_objects.numElem());
		s_storedRenderState = renderState;

		if (inst_update_once.GetBool())
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

		if (obj_draw.GetBool())
		{
			s_grimRenderer.Draw(s_storedRenderState, rendPassCtx);
		}

		g_pfxRender->Render(rendPassCtx, nullptr);

		rendPassRecorder->Complete();
	}
	g_matSystem->QueueCommandBuffer(cmdRecorder->End());
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