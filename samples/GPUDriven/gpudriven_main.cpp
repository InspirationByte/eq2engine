#include <imgui.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"
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


DECLARE_CVAR(lod_override, "-1", nullptr, 0);
DECLARE_CVAR(cam_speed, "100", nullptr, 0);
DECLARE_CVAR(cam_fov, "90", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_count, "10000", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR(inst_update_once, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(obj_rotate_interval, "100", 1.0, 1000, nullptr, 0);
DECLARE_CVAR(obj_rotate, "1", nullptr, 0);
DECLARE_CVAR(obj_draw, "1", nullptr, 0);

static CStaticAutoPtr<CState_GpuDrivenDemo> g_State_Demo;
static IVertexFormatPtr s_gameObjectVF = nullptr;

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
static DemoRenderState			s_storedRenderState;

CState_GpuDrivenDemo::CState_GpuDrivenDemo()
{

}

// when changed to this state
// @from - used to transfer data
void CState_GpuDrivenDemo::OnEnter(CAppStateBase* from)
{
	g_inputCommandBinder->AddBinding("W", "forward", [](void* _this, const Vector3D& value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_FORWARD, value.x > 0);
	}, this);

	g_inputCommandBinder->AddBinding("S", "backward", [](void* _this, const Vector3D& value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_BACKWARD, value.x > 0);
	}, this);

	g_inputCommandBinder->AddBinding("A", "strafeleft", [](void* _this, const Vector3D& value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_SIDE_LEFT, value.x > 0);
	}, this);

	g_inputCommandBinder->AddBinding("D", "straferight", [](void* _this, const Vector3D& value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);
		bitsSet(this_->m_cameraButtons, CAM_SIDE_RIGHT, value.x > 0);
	}, this);

	g_inputCommandBinder->AddBinding("R", "reset", [](void* _this, const Vector3D& value) {
		CState_GpuDrivenDemo* this_ = reinterpret_cast<CState_GpuDrivenDemo*>(_this);

		if (value.x <= 0)
			this_->InitGame();
	}, this);

	// go heavy.
	constexpr const int INST_RESERVE = 5000000;

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
			const GRIMArchetype archetypeId = DemoGRIMRenderer::Get().CreateStudioDrawArchetype(geom, s_gameObjectVF, 3);

			// TODO: body group lookup
			s_modelIdToArchetypeId.insert(geom->GetCacheId(), archetypeId);
		}
	}

	DemoGRIMRenderer::GetAllocator().Initialize("InstanceUtils", INST_RESERVE);
	DemoGRIMRenderer::Get().Init();
	InitGame();
}

// when the state changes to something
// @to - used to transfer data
void CState_GpuDrivenDemo::OnLeave(CAppStateBase* to)
{
	DemoGRIMRenderer::Get().Shutdown();
	DemoGRIMRenderer::GetAllocator().Shutdown();

	s_modelIdToArchetypeId.clear(true);
	s_storedRenderState = {};
	s_gameObjectVF = nullptr;

	g_inputCommandBinder->UnbindCommandByName("forward");
	g_inputCommandBinder->UnbindCommandByName("backward");
	g_inputCommandBinder->UnbindCommandByName("strafeleft");
	g_inputCommandBinder->UnbindCommandByName("straferight");

	g_studioCache->ReleaseCache();
}

void CState_GpuDrivenDemo::InitGame()
{
	inst_update.SetBool(true);

	// distribute instances randomly
	const int modelCount = g_studioCache->GetModelCount();

	DemoGRIMRenderer::GetAllocator().FreeAll(false, true);
	s_objects.clear();

	for (int i = 0; i < inst_count.GetInt(); ++i)
	{
		const int rndModelIdx = (i % (modelCount - 1)) + 1;

		CEqStudioGeom* geom = g_studioCache->GetModel(rndModelIdx);
		auto it = s_modelIdToArchetypeId.find(geom->GetCacheId());
		if (it.atEnd())
		{
			ASSERT_FAIL("Can't get archetype for model idx = %d", rndModelIdx);
			continue;
		}

		geom->QueueMaterialsLoading();

		Object& obj = s_objects.append();
		obj.instId = DemoGRIMRenderer::GetAllocator().AddInstance<InstTransform>(*it);
		obj.trs.t = Vector3D(RandomFloat(-2900, 2900), RandomFloat(-100, 100), RandomFloat(-2900, 2900));
		obj.trs.r = rotateXYZ(RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F), RandomFloat(-M_PI_2_F, M_PI_2_F));

		obj.trs.t *= 0.25f;

		// update visual
		InstTransform transform;
		transform.position = obj.trs.t;
		transform.orientation = obj.trs.r;
		DemoGRIMRenderer::GetAllocator().Set(obj.instId, transform);
	}
}

void CState_GpuDrivenDemo::StepGame(float fDt)
{
	PROF_EVENT_F();

	Vector3D forward, right;
	AngleVectors(s_currentView.GetAngles(), forward, right);

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
			DemoGRIMRenderer::GetAllocator().Set(obj.instId, transform);
		}
	}

#ifdef IMGUI_ENABLED
	if (ImGui::Begin("GPUDriven Demo"))
	{
		if (ImGui::Button("Respawn Objects"))
			InitGame();

		{
			int newCount = inst_count.GetInt();
			if (ImGui::SliderInt("Object count", &newCount, 10, 1000000))
				inst_count.SetInt(newCount);
		}

		{
			int rotatingInterval = obj_rotate_interval.GetInt();
			if (ImGui::SliderInt("Rotation Objects interval", &rotatingInterval, 1, 100))
				obj_rotate_interval.SetInt(rotatingInterval);
		}

		{
			HOOK_TO_CVAR(lod_override);
			int lodOverride = lod_override->GetInt();
			if (ImGui::SliderInt("LOD override", &lodOverride, -1, GRIM_MAX_INSTANCE_LODS-1))
			{
				lod_override->SetInt(lodOverride);
			}
		}

		ImGui::End();
	}
#endif

	GRIMDrawSettings drawSettings = DemoGRIMRenderer::Get().GetDrawSettings();
	drawSettings.overrideLodIdx = lod_override.GetInt();
	DemoGRIMRenderer::Get().SetDrawSettings(drawSettings);
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

	{
		IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder();
		DemoGRIMRenderer::GetAllocator().SyncInstances(cmdRecorder);
		DemoGRIMRenderer::Get().SyncArchetypes(cmdRecorder);

		if (inst_update.GetBool())
		{
			const Matrix4x4 viewProjMat = projMat * viewMat;
			s_storedRenderState.frustum.LoadAsFrustum(viewProjMat);
			s_storedRenderState.viewPos = s_currentView.GetOrigin();

			DemoGRIMRenderer::Get().PrepareDraw(cmdRecorder, s_storedRenderState, s_objects.numElem());

			if (inst_update_once.GetBool())
				inst_update.SetBool(false);

			g_matSystem->QueueCommandBuffer(cmdRecorder->End());
			Future<bool> future = g_matSystem->SubmitQueuedCommandsAwaitable();
			future.AddCallback([&](const FutureResult<bool>& result) {
				DemoGRIMRenderer::Get().PostPrepareDraw(s_storedRenderState);
			});

			while (!future.Wait(0))
				g_renderAPI->Flush();
		}
		else
		{
			g_matSystem->QueueCommandBuffer(cmdRecorder->End());
		}
	}

	{
		IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder();
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
				DemoGRIMRenderer::Get().Draw(s_storedRenderState, rendPassCtx);
			}

			rendPassRecorder->Complete();
		}
		g_matSystem->QueueCommandBuffer(cmdRecorder->End());
	}
}

void CState_GpuDrivenDemo::HandleKeyPress(int key, bool down)
{
	g_inputCommandBinder->OnPressEvent(key, down);
}

void CState_GpuDrivenDemo::HandleMouseClick(int x, int y, int buttons, bool down)
{
	if (g_consoleInput->IsVisible())
		return;
	g_inputCommandBinder->OnPressEvent(buttons, down);
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
}

void CState_GpuDrivenDemo::HandleJoyAxis(short axis, short value)
{
}

void CState_GpuDrivenDemo::GetMouseCursorProperties(bool& visible, bool& centered) 
{
	visible = false; 
	centered = true; 
}