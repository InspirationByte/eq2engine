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

static constexpr const float s_browserPanelWidth = 260.0f;
static constexpr const float s_inspectorPanelWidth = 420.0f;
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

CAnimatedModel			g_viewerModel;
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
	g_viewerModel.SetModel(nullptr);
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

	g_viewerModel.SetModel(g_studioCache->GetModel(cacheIndex));
}

void CState_EGFViewer::ReloadModel()
{
	CEqStudioGeom* model = g_viewerModel.m_model;
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
	if (!g_viewerModel.m_model)
		return;

	const BoundingBox& bbox = g_viewerModel.m_model->GetBoundingBox();
	m_cameraDistance = length(bbox.GetSize()) * 2.0f;
}

void CState_EGFViewer::RefreshGUI()
{
	m_motionItems.clear();
	m_motionSelection = -1;
	m_poseControllerSelection = -1;
	m_selectedMeshGroup = -1;
	m_selectedMesh = -1;
	g_viewerModel.m_bodyGroupFlags = 0xFFFFFFFF;
	g_viewerModel.m_skinIdx = 0;
	g_viewerModel.m_selectedPhysObject = -1;
	g_viewerModel.m_selectedBone = -1;
	g_viewerModel.m_selectedAttachment = -1;

	if (!g_viewerModel.m_model)
		return;

	if (m_animMode == 0)
	{
		for (const AnimSequence* seq : g_viewerModel.m_sequencesList)
		{
			MotionItem& item = m_motionItems.append();
			item.name = EqString::Format("%s.%s", seq->motionData->name, seq->desc->name);
			item.sequenceId = g_viewerModel.FindSequence(item.name);
		}
	}
	// TODO: animation mode is not supported yet
}

void CState_EGFViewer::OnSequenceSelected()
{
	if (m_motionSelection == -1)
		return;

	g_viewerModel.SetSequence(m_motionItems[m_motionSelection].sequenceId, 0);
	g_viewerModel.ResetSequenceTime(0);
	g_viewerModel.PlaySequence(0);

	const AnimSequence* seq = g_viewerModel.m_sequencesList[m_motionSelection];
	CString::PrintF(m_animFramerate, sizeof(m_animFramerate), "%g", seq->desc->framerate);

	m_timelineMax = g_viewerModel.GetCurrentAnimationDurationInFrames() - 1;
	g_viewerModel.SetPlaybackSpeedScale(1.0f, 0);
}

void CState_EGFViewer::OnPoseControllerSelected()
{
	if (m_poseControllerSelection == -1)
		return;

	g_viewerModel.GetPoseControllerRange(m_poseControllerSelection, m_poseValueMin, m_poseValueMax);
	m_poseValue = g_viewerModel.GetPoseControllerValue(m_poseControllerSelection);
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

	if (g_viewerModel.IsPhysicsEnabled() && m_dragObj)
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

	PhysicsSimulate(fDt * m_simTimescale);
	g_viewerModel.Update(fDt);

	if (g_viewerModel.IsSequencePlaying() && m_motionSelection != -1)
	{
		const AnimSequence* seq = g_viewerModel.m_sequencesList[m_motionSelection];

		const float setFrameRate = atoi(m_animFramerate);
		if (setFrameRate > 0)
			g_viewerModel.SetPlaybackSpeedScale(setFrameRate / seq->desc->framerate, 0);

		m_timelineFrame = g_viewerModel.GetCurrentAnimationFrame();
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

	g_viewerModel.Render(renderFlags, m_cameraDistance, m_lod, m_lodOverride, fDt, modelDrawRenderPass);
	RenderOverlays(modelDrawRenderPass);

	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	// floor 1x1 meters
	if (m_drawFloor)
		RenderFloor(modelDrawRenderPass);

	if (m_drawGrid)
	{
		DrawGrid(1.0f, 8, vec3_zero, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), true, modelDrawRenderPass);
		DbgLine().Start(vec3_zero).End(vec3_right).Color(MColor(1.0f, 0.0f, 0.0f));
		DbgLine().Start(vec3_zero).End(vec3_up).Color(MColor(0.0f, 1.0f, 0.0f));
		DbgLine().Start(vec3_zero).End(vec3_forward).Color(MColor(0.0f, 0.0f, 1.0f));
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

		if (g_viewerModel.IsPhysicsEnabled() && m_dragObj == nullptr)
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
	const float panelsHeight = viewport->WorkSize.y;
	if (m_showBrowser)
		DrawModelBrowser(viewport->WorkPos.y, panelsHeight);

	const float browserWidth = m_showBrowser ? s_browserPanelWidth : 0.0f;
	const float inspectorWidth = min(s_inspectorPanelWidth, viewport->WorkSize.x * 0.5f);
	DrawSceneViewport(ImVec2(viewport->WorkPos.x + browserWidth, viewport->WorkPos.y), ImVec2(viewport->WorkSize.x - browserWidth - inspectorWidth, panelsHeight));
	DrawInspector(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - inspectorWidth, viewport->WorkPos.y), ImVec2(inspectorWidth, panelsHeight));
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
	if (g_viewerModel.m_model)
	{
		EqString currentModel = g_viewerModel.m_model->GetName();
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
		ImGui::Separator();

		static const char* s_vertexDisplayNames[] = { "No vertex data", "Normals", "Tangents", "Vertex colors", "Weights of the selected bone" };
		for (int i = 0; i < elementsOf(s_vertexDisplayNames); ++i)
		{
			if (ImGui::MenuItem(s_vertexDisplayNames[i], nullptr, m_vertexDisplay == i))
				m_vertexDisplay = i;
		}

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}
