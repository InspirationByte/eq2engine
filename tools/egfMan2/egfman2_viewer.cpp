//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - model viewer application state
//////////////////////////////////////////////////////////////////////////////////

#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <imgui.h>

#include "core/core_common.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "math/Utility.h"

#include "sys/sys_host.h"
#include "input/in_keys_ident.h"

#include "studio/StudioCache.h"
#include "studio/StudioGeom.h"
#include "physics/eqPhysics_World.h"
#include "physics/eqPhysics_Body.h"
#include "physics/PhysicsCollisionGroup.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "render/IDebugOverlay.h"
#include "render/ViewParams.h"

#include "CAnimatedModel.h"
#include "grid.h"
#include "egfman2_filedialog.h"
#include "egfman2_physics.h"

#include "egfman2_viewer.h"

static constexpr const float s_bottomPanelHeight = 200.0f;
static constexpr const float s_browserPanelWidth = 260.0f;
static constexpr const char* s_browserRootDir = "models";

static constexpr ImGuiWindowFlags s_panelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
	| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
static constexpr const float s_dragSpring = 10.0f;
static constexpr const float s_dragDamping = 0.5f;

enum EMouseButtonBits
{
	MOUSE_BIT_LEFT		= (1 << 0),
	MOUSE_BIT_RIGHT		= (1 << 1),
	MOUSE_BIT_MIDDLE	= (1 << 2),
};

CStaticAutoPtr<CState_EGFViewer> g_stateViewer;

// exposes pose controllers to the UI
class CViewerModel : public CAnimatedModel
{
public:
	ArrayCRef<AnimPoseController>	GetPoseControllers() const { return m_poseControllers; }
};

static CViewerModel		s_model;
static CViewParams		s_cameraParams(Vector3D(0, 0, -100), vec3_zero, 70);

//-------------------------------------------------------------------------------------------------

// tool output goes to the engine console
static void RunCompilerTool(const EqString& cmdLine)
{
	int exitCode = 0;
	if (!RunProcessToConsole(cmdLine, exitCode))
		ErrorMsg("Failed to run command '%s'", cmdLine.ToCString());
	else if (exitCode != 0)
		MsgError("Command '%s' failed with exit code %d\n", cmdLine.ToCString(), exitCode);
}

static void FlushCache()
{
	g_studioCache->ReleaseCache();
	g_studioCache->PrecacheModel("models/error.egf");
}

static int MouseButtonBit(int button)
{
	if (button == MOU_B1)
		return MOUSE_BIT_LEFT;
	if (button == MOU_B2)
		return MOUSE_BIT_RIGHT;
	if (button == MOU_B3)
		return MOUSE_BIT_MIDDLE;
	return 0;
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::OnEnter(CAppStateBase* from)
{
	PhysicsSceneInit();

	FogInfo fog;
	fog.enableFog = true;
	fog.fogColor = ColorRGB(0.25f, 0.25f, 0.25f);
	fog.fogDensity = 1.0f;
	fog.fogFar = 14250;
	fog.fogNear = -2750;
	g_matSystem->SetFogInfo(fog);

	g_matSystem->GetConfiguration().editormode = true;
	g_matSystem->GetConfiguration().enableShadows = false;

	g_studioCache->PrecacheModel("models/error.egf");

	const int modelArgIdx = g_cmdLine->Find("-model");
	if (modelArgIdx != -1)
	{
		const EqString modelPath(g_cmdLine->GetArgumentsOf(modelArgIdx));
		LoadModel(modelPath.ToCString());
		SetOptimalCameraDistance();
	}

	RefreshGUI();
}

void CState_EGFViewer::OnLeave(CAppStateBase* to)
{
	m_dragObj = nullptr;
	s_model.SetModel(nullptr);
	PhysicsSceneShutdown();
	m_viewport.ReleaseTargets();
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::LoadModel(const char* modelPath)
{
	m_dragObj = nullptr;
	FlushCache();

	const int cacheIndex = g_studioCache->PrecacheModel(modelPath);
	if (cacheIndex == STUDIOCACHE_INVALID_IDX)
		ErrorMsg("Can't open %s\n", modelPath);

	s_model.SetModel(g_studioCache->GetModel(cacheIndex));
}

void CState_EGFViewer::ReloadModel()
{
	CEqStudioGeom* model = s_model.m_model;
	if (!model)
		return;

	LoadModel(model->GetName());
	RefreshGUI();
}

void CState_EGFViewer::OpenModelDialog()
{
	const EqString modelsPath(g_fileSystem->GetAbsoluteFullPath(SP_MOD, "models"));

	EqString modelPath;
	if (FileDialogOpen(modelPath, "Open EGF model", "Entity Geometry File", "egf", modelsPath))
		LoadModel(modelPath.ToCString());

	SetOptimalCameraDistance();
	RefreshGUI();
}

void CState_EGFViewer::CompileScriptsDialog()
{
	Array<EqString> paths(PP_SL);
	if (!FileDialogOpenMultiple(paths, "Select script files to compile", "Script", "esc,asc", nullptr))
		return;

	const char* devAddonDir = g_fileSystem->GetCurrentGameDirectory();

	for (const EqString& fileName : paths)
	{
		const EqString ext(fnmPathExtractExt(fileName));

		if (ext == "asc")
		{
			const EqString cmdLine(EqString::Format("animca.exe -devAddon \"%s\" \"%s\"", devAddonDir, fileName.ToCString()));

			Msg("***Starting animCa: %s\n", cmdLine.ToCString());
			RunCompilerTool(cmdLine);

			continue;
		}

		EqString modelPath;

		KVSection scriptSec;
		if (KV_LoadFromFile(fileName, -1, scriptSec))
		{
			const KVSection* modelFileNameKey = scriptSec.FindSection("modelfilename");
			if (modelFileNameKey)
			{
				modelPath = KV_GetValueString(modelFileNameKey);
				const EqString cmdLine(EqString::Format("egfca.exe -devAddon \"%s\" \"%s\"", devAddonDir, fileName.ToCString()));

				Msg("***Starting egfCa: '%s'\n", cmdLine.ToCString());
				RunCompilerTool(cmdLine);
			}
			else
			{
				ErrorMsg("ERROR! 'modelfilename' is not specified in the script!");
			}
		}

		// show the compiled model only when a single script was selected
		if (paths.numElem() == 1 && modelPath.Length())
		{
			LoadModel(modelPath.ToCString());
			SetOptimalCameraDistance();
			g_matSystem->PreloadNewMaterials();
		}
	}

	RefreshGUI();
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::ResetView()
{
	m_cameraRotation = Vector3D(25, 225, 0);
	m_cameraTarget = vec3_zero;
	SetOptimalCameraDistance();
}

void CState_EGFViewer::SetOptimalCameraDistance()
{
	if (!s_model.m_model)
		return;

	const BoundingBox& bbox = s_model.m_model->GetBoundingBox();
	m_cameraDistance = length(bbox.GetSize()) * 2.0f;
}

void CState_EGFViewer::RefreshGUI()
{
	m_bodyGroupNames.clear();
	m_motionItems.clear();
	m_motionSelection = -1;
	m_poseControllerSelection = -1;
	s_model.m_bodyGroupFlags = 0xFFFFFFFF;

	if (!s_model.m_model)
		return;

	const studioHdr_t& studio = s_model.m_model->GetStudioHdr();
	for (int i = 0; i < studio.numBodyGroups; i++)
		m_bodyGroupNames.append(studio.pBodyGroups(i)->name);

	if (m_animMode == 0)
	{
		for (const AnimSequence* seq : s_model.m_sequencesList)
		{
			MotionItem& item = m_motionItems.append();
			item.name = EqString::Format("%s.%s", seq->motionData->name, seq->desc->name);
			item.sequenceId = s_model.FindSequence(item.name);
		}
	}
	// TODO: animation mode is not supported yet
}

void CState_EGFViewer::OnSequenceSelected()
{
	if (m_motionSelection == -1)
		return;

	s_model.SetSequence(m_motionItems[m_motionSelection].sequenceId, 0);
	s_model.ResetSequenceTime(0);
	s_model.PlaySequence(0);

	const AnimSequence* seq = s_model.m_sequencesList[m_motionSelection];
	CString::PrintF(m_animFramerate, sizeof(m_animFramerate), "%g", seq->desc->framerate);

	m_timelineMax = s_model.GetCurrentAnimationDurationInFrames() - 1;
	s_model.SetPlaybackSpeedScale(1.0f, 0);
}

void CState_EGFViewer::OnPoseControllerSelected()
{
	if (m_poseControllerSelection == -1)
		return;

	s_model.GetPoseControllerRange(m_poseControllerSelection, m_poseValueMin, m_poseValueMax);
	m_poseValue = s_model.GetPoseControllerValue(m_poseControllerSelection);
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::ShowFPS(float frameTime)
{
	static float s_accTime = 0.1f;
	static int s_fps = 0;
	static int s_numFrames = 0;

	if (s_accTime > 0.1f)
	{
		s_fps = static_cast<int>(s_numFrames / s_accTime + 0.5f);
		s_numFrames = 0;
		s_accTime = 0;
	}

	s_accTime += frameTime;
	s_numFrames++;

	ColorRGBA col(1, 0, 0, 1);
	if (s_fps > 25)
	{
		col = ColorRGBA(1, 1, 0, 1);
		if (s_fps > 45)
			col = ColorRGBA(0, 1, 0, 1);
	}

	debugoverlay->Text(col, "FPS: %d (%g s)", s_fps, frameTime);
}

void CState_EGFViewer::RenderFloor(IGPURenderPassRecorder* rendPassRecorder)
{
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.cullMode = CULL_FRONT;
	defaultRenderPass.drawColor = MColor(1.0f, 1.0f, 0.0f, 0.15f);

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
	meshBuilder.TexturedQuad3(
		Vector3D(-64, 0, -64),
		Vector3D(64, 0, -64),
		Vector3D(-64, 0, 64),
		Vector3D(64, 0, 64),
		vec2_zero, vec2_zero, vec2_zero, vec2_zero);

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

bool CState_EGFViewer::Update(float fDt)
{
	const IVector2D screenSize = m_viewport.GetSize();

	Vector3D forward, right;
	AngleVectors(m_cameraRotation, forward, right);

	s_cameraParams.SetAngles(m_cameraRotation);
	s_cameraParams.SetOrigin(m_cameraTarget - forward * m_cameraDistance);

	ShowFPS(fDt);

	FogInfo fog;
	g_matSystem->GetFogInfo(fog);
	fog.viewPos = s_cameraParams.GetOrigin();
	g_matSystem->SetFogInfo(fog);

	m_projMat = perspectiveMatrixY(DEG2RAD(s_cameraParams.GetFOV()), screenSize.x, screenSize.y, 0.25f, 2500.0f);

	m_viewMat = rotateZXY4(DEG2RAD(-s_cameraParams.GetAngles().x), DEG2RAD(-s_cameraParams.GetAngles().y), DEG2RAD(-s_cameraParams.GetAngles().z));
	m_viewMat.translate(-s_cameraParams.GetOrigin());

	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, m_projMat);
	g_matSystem->SetMatrix(MATRIXMODE_VIEW, m_viewMat);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);
	debugoverlay->SetMatrices(m_projMat, m_viewMat);

	if (s_model.IsPhysicsEnabled() && m_dragObj)
	{
		const Vector3D worldDragPoint = transformPoint(m_dragObjLocalPoint, m_dragObj->GetTransform());

		// spring with damping towards the target point
		const Vector3D targetVelocity = (m_dragTargetPoint - worldDragPoint) * s_dragSpring;
		const Vector3D dragImpulse = (targetVelocity - m_dragObj->GetVelocityAtWorldPoint(worldDragPoint)) * m_dragObj->GetMass() * s_dragDamping;

		DbgSphere().Position(worldDragPoint).Fill(true).Radius(0.05f);
		DbgLine().Start(worldDragPoint).End(m_dragTargetPoint);

		m_dragObj->Wake();
		m_dragObj->ApplyWorldImpulse(worldDragPoint, dragImpulse);
	}

	PhysicsSimulate(fDt);
	s_model.Update(fDt);

	if (s_model.IsSequencePlaying() && m_motionSelection != -1)
	{
		const AnimSequence* seq = s_model.m_sequencesList[m_motionSelection];

		const float setFrameRate = atoi(m_animFramerate);
		if (setFrameRate > 0)
			s_model.SetPlaybackSpeedScale(setFrameRate / seq->desc->framerate, 0);

		m_timelineFrame = s_model.GetCurrentAnimationFrame();
	}

	int renderFlags = 0;
	if (m_drawPhysModel)
		renderFlags |= RFLAG_PHYSICS;
	if (m_drawBones)
		renderFlags |= RFLAG_BONES;
	if (m_drawAttachments)
		renderFlags |= RFLAG_ATTACHMENTS;

	// host resets wireframe mode from r_wireframe every frame
	if (m_wireframe)
		g_matSystem->GetConfiguration().wireframeMode = true;

	IGPURenderPassRecorderPtr modelDrawRenderPass = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(m_viewport.GetColorTarget(), true, ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f))
		.DepthStencilTarget(m_viewport.GetDepthTarget())
		.DepthClear()
		.End()
	);

	s_model.Render(renderFlags, m_cameraDistance, m_lod, m_lodOverride, fDt, modelDrawRenderPass);

	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	// floor 1x1 meters
	if (m_drawFloor)
		RenderFloor(modelDrawRenderPass);

	if (m_drawGrid)
	{
		DrawGrid(1.0f, 8, vec3_zero, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), true, modelDrawRenderPass);
		debugoverlay->Line3D(vec3_zero, vec3_right, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
		debugoverlay->Line3D(vec3_zero, vec3_up, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
		debugoverlay->Line3D(vec3_zero, vec3_forward, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
	}

	g_matSystem->QueueCommandBuffer(modelDrawRenderPass->End());

	return true;
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::GetMouseCursorProperties(bool& visible, bool& centered)
{
	visible = true;
	centered = false;
}

void CState_EGFViewer::HandleKeyPress(int key, bool down)
{
	if (!down)
		return;

	const bool ctrlDown = (SDL_GetModState() & KMOD_CTRL) != 0;

	if (ctrlDown)
	{
		if (key == KEY_O)
			OpenModelDialog();
		return;
	}

	if (key == KEY_R)
		ResetView();
	else if (key == KEY_P)
		m_drawPhysModel = !m_drawPhysModel;
	else if (key == KEY_B)
		m_drawBones = !m_drawBones;
	else if (key == KEY_W)
		m_wireframe = !m_wireframe;
}

void CState_EGFViewer::HandleMouseClick(int x, int y, int buttons, bool down)
{
	const int buttonBit = MouseButtonBit(buttons);
	if (down)
		m_mouseButtons |= buttonBit;
	else
		m_mouseButtons &= ~buttonBit;

	ProcessMouse(x, y, vec2_zero);
}

void CState_EGFViewer::HandleMouseMove(int x, int y, float deltaX, float deltaY)
{
	// warping cursor produces motion event with a jump to the opposite edge
	if (m_cursorWrapPending && x == m_cursorWrapPos.x && y == m_cursorWrapPos.y)
	{
		m_cursorWrapPending = false;
		return;
	}

	// host gives deltas as -0.05 * (dx, -dy) * m_sensitivity, convert them back to pixels
	ProcessMouse(x, y, Vector2D(deltaX, -deltaY) * -20.0f);
}

void CState_EGFViewer::WrapCursor(int x, int y)
{
	// remaining motion events before the warp one still have old positions
	if (m_cursorWrapPending)
		return;

	const IVector2D screenSize = m_viewport.GetSize();
	IVector2D wrapPos(x, y);

	if (x <= 0)
		wrapPos.x = screenSize.x - 2;
	else if (x >= screenSize.x - 1)
		wrapPos.x = 1;

	if (y <= 0)
		wrapPos.y = screenSize.y - 2;
	else if (y >= screenSize.y - 1)
		wrapPos.y = 1;

	if (wrapPos.x == x && wrapPos.y == y)
		return;

	m_cursorWrapPos = wrapPos;
	m_cursorWrapPending = true;
	g_pHost->SetCursorPosition(wrapPos.x, wrapPos.y);
}

void CState_EGFViewer::ProcessMouse(int x, int y, const Vector2D& moveDelta)
{
	// button release can be consumed by ImGui, so drop the buttons that are no longer held
	const uint32 sdlButtons = SDL_GetMouseState(nullptr, nullptr);
	if (!(sdlButtons & SDL_BUTTON_LMASK))
		m_mouseButtons &= ~MOUSE_BIT_LEFT;
	if (!(sdlButtons & SDL_BUTTON_RMASK))
		m_mouseButtons &= ~MOUSE_BIT_RIGHT;
	if (!(sdlButtons & SDL_BUTTON_MMASK))
		m_mouseButtons &= ~MOUSE_BIT_MIDDLE;

	if (!m_mouseButtons)
		m_cursorWrapPending = false;

	const IVector2D screenSize = m_viewport.GetSize();
	const int w = screenSize.x;
	const int h = screenSize.y;

	Vector3D camAngles = m_cameraRotation;
	Vector3D camPos = m_cameraTarget;

	const float fov = s_cameraParams.GetFOV();

	// height is used because width has aspect
	const float cameraMoveFactor = -(fov / h) * 2;

	Vector3D forward, right, up;
	AngleVectors(camAngles, forward, right, up);

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		if (m_mouseButtons & MOUSE_BIT_LEFT)
		{
			camAngles.x += moveDelta.y * 0.1f;
			camAngles.y -= moveDelta.x * 0.1f;
		}

		if (m_mouseButtons & MOUSE_BIT_RIGHT)
		{
			m_cameraDistance += moveDelta.y * 0.05f;
			if (m_cameraDistance < 0.1f)
				m_cameraDistance = 0.1f;
		}

		if (m_mouseButtons & MOUSE_BIT_MIDDLE)
		{
			camPos += right * moveDelta.x * cameraMoveFactor * m_cameraDistance * 0.01f;
			camPos -= up * moveDelta.y * cameraMoveFactor * m_cameraDistance * 0.01f;
		}

		if (m_mouseButtons)
			WrapCursor(x, y);
	}
	else if (m_mouseButtons & MOUSE_BIT_LEFT)
	{
		Vector3D rayStart, rayDir;
		ScreenToDirection(Vector2D(w - x, y), Vector2D(w, h), s_cameraParams.GetOrigin(), m_projMat, m_viewMat, false, rayStart, rayDir);

		if (s_model.IsPhysicsEnabled() && m_dragObj == nullptr)
		{
			eqCollisionInfo coll;
			const FVector3D rayEnd = rayStart + rayDir * 1000.0f;
			if (PhysicsGetWorld().TestLineCollision(rayStart, rayEnd, coll, COLLISION_GROUP_OBJECTS | COLLISION_GROUP_DEBRIS) && coll.hitobject->IsDynamic())
			{
				m_dragObj = static_cast<CEqRigidBody*>(coll.hitobject);
				m_dragTargetPoint = m_dragStartPoint = coll.position;
				m_dragObjLocalPoint = transformPointInverse(Vector3D(coll.position), m_dragObj->GetTransform());
			}
		}

		if (m_dragObj)
		{
			const Vector3D dragForward = normalize(m_dragStartPoint - s_cameraParams.GetOrigin());

			// drag plane faces the camera
			const Plane pl(dragForward.x, dragForward.y, dragForward.z, -dot(dragForward, m_dragStartPoint));

			Vector3D intersection;
			if (pl.GetIntersectionWithRay(rayStart, normalize(rayDir), intersection))
				m_dragTargetPoint = intersection;
		}
	}
	else
	{
		m_dragObj = nullptr;
	}

	m_cameraRotation = camAngles;
	m_cameraTarget = clamp(camPos, Vector3D(-F_INFINITY), Vector3D(F_INFINITY));
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::DrawUI(bool& visible)
{
	DrawMenuBar();

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float topPanelsHeight = viewport->WorkSize.y - s_bottomPanelHeight;
	if (m_showBrowser)
		DrawModelBrowser(viewport->WorkPos.y, topPanelsHeight);

	const float browserWidth = m_showBrowser ? s_browserPanelWidth : 0.0f;
	DrawSceneViewport(ImVec2(viewport->WorkPos.x + browserWidth, viewport->WorkPos.y), ImVec2(viewport->WorkSize.x - browserWidth, topPanelsHeight));

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - s_bottomPanelHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, s_bottomPanelHeight));

	if (ImGui::Begin("##egfman2Panel", nullptr, s_panelFlags))
	{
		if (ImGui::BeginTabBar("##egfman2Tabs"))
		{
			if (ImGui::BeginTabItem("Model"))
			{
				DrawModelTab();
				ImGui::EndTabItem();
			}

			const ImGuiTabItemFlags motionTabFlags = m_selectMotionTab ? ImGuiTabItemFlags_SetSelected : 0;
			m_selectMotionTab = false;
			if (ImGui::BeginTabItem("Motion", nullptr, motionTabFlags))
			{
				DrawMotionTab();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Physics"))
			{
				DrawPhysicsTab();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Inverse Kinematics"))
			{
				DrawIKTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void CState_EGFViewer::DrawSceneViewport(const ImVec2& pos, const ImVec2& size)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	if (ImGui::Begin("##egfman2Scene", nullptr, s_panelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		m_viewport.DrawImGui();
	ImGui::End();
	ImGui::PopStyleVar(2);
}

void CState_EGFViewer::DrawModelBrowser(float panelTop, float panelHeight)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, panelTop));
	ImGui::SetNextWindowSize(ImVec2(s_browserPanelWidth, panelHeight));

	if (ImGui::Begin("##egfman2Browser", nullptr, s_panelFlags))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float refreshWidth = ImGui::CalcTextSize("Refresh").x + style.FramePadding.x * 2.0f;

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - refreshWidth - style.ItemSpacing.x);
		ImGui::InputTextWithHint("##browserFilter", "Filter", m_browserFilter, sizeof(m_browserFilter));
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			m_browserDirs.clear();

		ImGui::Separator();

		if (ImGui::BeginChild("##browserTree"))
		{
			// filter needs all directories, they are scanned once and cached
			if (m_browserFilter[0])
				DrawBrowserFiltered(s_browserRootDir);
			else
				DrawBrowserDir(s_browserRootDir);
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void CState_EGFViewer::DrawBrowserDir(const EqString& dirPath)
{
	// directory map items are not moved by inserts made in nested directories
	const BrowserDir& dir = GetBrowserDir(dirPath);

	for (const EqString& name : dir.dirs)
	{
		if (!ImGui::TreeNodeEx(name.ToCString(), ImGuiTreeNodeFlags_SpanAvailWidth))
			continue;

		DrawBrowserDir(EqString::Format("%s/%s", dirPath.ToCString(), name.ToCString()));
		ImGui::TreePop();
	}

	for (const EqString& name : dir.files)
		DrawBrowserFile(dirPath, name, name);
}

void CState_EGFViewer::DrawBrowserFiltered(const EqString& dirPath)
{
	const BrowserDir& dir = GetBrowserDir(dirPath);

	for (const EqString& name : dir.dirs)
		DrawBrowserFiltered(EqString::Format("%s/%s", dirPath.ToCString(), name.ToCString()));

	// label is the path relative to the browser root
	const int rootLength = CString::Length(s_browserRootDir);
	const char* relativeDir = dirPath.ToCString() + min(rootLength + 1, dirPath.Length());

	for (const EqString& name : dir.files)
	{
		if (name.Find(m_browserFilter) == -1)
			continue;

		EqString label(name);
		if (*relativeDir)
			label = EqString::Format("%s/%s", relativeDir, name.ToCString());

		DrawBrowserFile(dirPath, name, label);
	}
}

void CState_EGFViewer::DrawBrowserFile(const EqString& dirPath, const EqString& name, const EqString& label)
{
	const EqString filePath = EqString::Format("%s/%s", dirPath.ToCString(), name.ToCString());

	bool selected = false;
	if (s_model.m_model)
	{
		EqString currentModel = s_model.m_model->GetName();
		EqString cmpFilePath = filePath;
		fnmPathFixSeparators(currentModel);
		fnmPathFixSeparators(cmpFilePath);
		selected = !cmpFilePath.CompareCaseIns(currentModel);
	}

	if (!ImGui::Selectable(label.ToCString(), selected) || selected)
		return;

	LoadModel(filePath);
	SetOptimalCameraDistance();
	RefreshGUI();
}

const CState_EGFViewer::BrowserDir& CState_EGFViewer::GetBrowserDir(const EqString& dirPath)
{
	const uint dirId = StringId(dirPath, true);
	const auto it = m_browserDirs.find(dirId);
	if (!it.atEnd())
		return *it;

	BrowserDir& dir = m_browserDirs[dirId];

	// same entries may come from several search paths
	CFileSystemFind fsFind(EqString::Format("%s/*", dirPath.ToCString()), SP_MOD);
	while (fsFind.Next())
	{
		const EqStringRef name = fsFind.GetPath();
		if (name[0] == '.')
			continue;

		const bool isDirectory = fsFind.IsDirectory();
		if (!isDirectory && fnmPathExtractExt(name) != "egf")
			continue;

		Array<EqString>& entries = isDirectory ? dir.dirs : dir.files;
		const int foundIdx = arrayFindIndexF(entries, [&](const EqString& entry) {
			return !entry.CompareCaseIns(name);
		});

		if (foundIdx == -1)
			entries.append(name);
	}

	const auto compareNames = [](const EqString& a, const EqString& b) {
		return a.CompareCaseIns(b);
	};
	arraySort(dir.dirs, compareNames);
	arraySort(dir.files, compareNames);

	return dir;
}

void CState_EGFViewer::DrawMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Open", "Ctrl+O"))
			OpenModelDialog();

		ImGui::Separator();

		if (ImGui::MenuItem("Reload model"))
			ReloadModel();

		// TODO: save physics model and sequence properties
		ImGui::MenuItem("Save");

		ImGui::Separator();

		if (ImGui::MenuItem("Compile scripts..."))
			CompileScriptsDialog();

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Reset View", "R"))
			ResetView();

		ImGui::MenuItem("Model browser", nullptr, &m_showBrowser);
		ImGui::Separator();
		ImGui::MenuItem("Show physics objects", "P", &m_drawPhysModel);
		ImGui::MenuItem("Show bones", "B", &m_drawBones);
		ImGui::MenuItem("Show attachments", nullptr, &m_drawAttachments);
		ImGui::MenuItem("Show ground", nullptr, &m_drawFloor);
		ImGui::MenuItem("Show grid", nullptr, &m_drawGrid);
		ImGui::MenuItem("Wireframe", "W", &m_wireframe);

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void CState_EGFViewer::DrawModelTab()
{
	ImGui::TextUnformatted("Body groups");
	ImGui::SameLine();

	if (ImGui::BeginListBox("##bodyGroups", ImVec2(250.0f, -FLT_MIN)))
	{
		for (int i = 0; i < m_bodyGroupNames.numElem(); ++i)
		{
			bool enabled = (s_model.m_bodyGroupFlags & (1 << i)) != 0;

			ImGui::PushID(i);
			if (ImGui::Checkbox(m_bodyGroupNames[i].ToCString(), &enabled))
			{
				if (enabled)
					s_model.m_bodyGroupFlags |= (1 << i);
				else
					s_model.m_bodyGroupFlags &= ~(1 << i);
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}

	ImGui::SameLine();
	ImGui::TextUnformatted("LODs");
	ImGui::SameLine();

	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::InputInt("##lod", &m_lod))
		m_lod = clamp(m_lod, 0, 10);

	ImGui::SameLine();
	ImGui::Checkbox("override", &m_lodOverride);
}

void CState_EGFViewer::DrawMotionTab()
{
	if (ImGui::BeginTable("##motionTable", 4, ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Anim/Act:");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(250.0f);
		const char* motionPreview = m_motionSelection != -1 ? m_motionItems[m_motionSelection].name.ToCString() : "select animation...";
		if (ImGui::BeginCombo("##motion", motionPreview))
		{
			for (int i = 0; i < m_motionItems.numElem(); ++i)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(m_motionItems[i].name.ToCString(), i == m_motionSelection))
				{
					m_motionSelection = i;
					OnSequenceSelected();
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("FPS:");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(50.0f);
		ImGui::InputText("##fps", m_animFramerate, sizeof(m_animFramerate), ImGuiInputTextFlags_CharsDecimal);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Timeline:");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(250.0f);
		if (ImGui::SliderInt("##timeline", &m_timelineFrame, 0, max(m_timelineMax, 0)))
			s_model.SetSequenceTime(m_timelineFrame, 0);

		ImGui::TableNextColumn();
		if (ImGui::Button("Play"))
		{
			s_model.ResetSequenceTime(0);
			s_model.PlaySequence(0);
		}

		ImGui::TableNextColumn();
		if (ImGui::Button("Stop"))
			s_model.StopSequence(0);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Pose controller");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(250.0f);
		const char* posePreview = m_poseControllerSelection != -1 ? s_model.GetPoseControllers()[m_poseControllerSelection].desc->name : "select controller...";
		if (ImGui::BeginCombo("##poseController", posePreview))
		{
			for (int i = 0; i < s_model.GetPoseControllers().numElem(); ++i)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(s_model.GetPoseControllers()[i].desc->name, i == m_poseControllerSelection))
				{
					m_poseControllerSelection = i;
					OnPoseControllerSelected();
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Control");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(250.0f);
		if (ImGui::SliderFloat("##poseValue", &m_poseValue, m_poseValueMin, m_poseValueMax) && m_poseControllerSelection != -1)
			s_model.SetPoseControllerValue(m_poseControllerSelection, m_poseValue);

		ImGui::EndTable();
	}

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::TextUnformatted("Mode");
	bool modeChanged = ImGui::RadioButton("Sequence Activity", &m_animMode, 0);
	modeChanged = ImGui::RadioButton("Animation", &m_animMode, 1) || modeChanged;
	ImGui::EndGroup();

	if (modeChanged)
		RefreshGUI();
}

void CState_EGFViewer::DrawPhysicsTab()
{
	constexpr float fieldWidth = 80.0f;

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Object");
	if (ImGui::BeginTable("##physObject", 2, ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Mass");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(fieldWidth);
		ImGui::InputText("##mass", m_objMass, sizeof(m_objMass));

		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Surfaceprops");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(fieldWidth);
		ImGui::InputText("##surfProps", m_objSurfProps, sizeof(m_objSurfProps));

		ImGui::EndTable();
	}
	ImGui::EndGroup();

	ImGui::SameLine(0.0f, 20.0f);

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Joint limits");
	if (ImGui::BeginTable("##physJointLimits", 3, ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Joint");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(fieldWidth);
		if (ImGui::BeginCombo("##joint", ""))
			ImGui::EndCombo();
		ImGui::TableNextColumn();

		static const char* s_axisNames[] = { "X", "Y", "Z" };
		for (int axis = 0; axis < 3; ++axis)
		{
			ImGui::PushID(axis);
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(s_axisNames[axis]);

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(fieldWidth);
			ImGui::InputText("##min", m_jointLimits[axis * 2], sizeof(m_jointLimits[0]));

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(fieldWidth);
			ImGui::InputText("##max", m_jointLimits[axis * 2 + 1], sizeof(m_jointLimits[0]));
			ImGui::PopID();
		}

		ImGui::EndTable();
	}
	ImGui::EndGroup();

	ImGui::SameLine(0.0f, 20.0f);

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Simulation");
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Timescale");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(fieldWidth);
	ImGui::InputText("##timescale", m_simTimescale, sizeof(m_simTimescale));

	if (ImGui::Button("Run/Pause"))
		s_model.TogglePhysicsState();

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
		s_model.ResetPhysics();
	ImGui::EndGroup();
}

void CState_EGFViewer::DrawIKTab()
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Link");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##ikLink", ""))
		ImGui::EndCombo();
}
