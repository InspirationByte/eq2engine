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
#include "dkphysics/IDkPhysics.h"
#include "physics/PhysicsCollisionGroup.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "render/IDebugOverlay.h"
#include "render/ViewParams.h"

#include "CAnimatedModel.h"
#include "grid.h"
#include "egfman2_filedialog.h"

#include "egfman2_viewer.h"

static constexpr const float s_bottomPanelHeight = 200.0f;

static constexpr const int s_physPlaneSize = 512;
static constexpr const int s_physPlaneW = 32;
static constexpr const int s_physPlaneH = 32;
static constexpr const int s_physPlaneVertCount = s_physPlaneW * s_physPlaneH;
static constexpr const int s_physPlaneIdxCount = (s_physPlaneW - 1) * (s_physPlaneH - 1) * 6;

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

// physics keeps pointers to the plane geometry
static Vector3D			s_physPlaneVertices[s_physPlaneVertCount];
static uint				s_physPlaneIndices[s_physPlaneIdxCount];

//-------------------------------------------------------------------------------------------------

static void InitPhysicsScene()
{
	g_physics->Init(0);
	g_physics->CreateScene();

	for (int c = 0; c < s_physPlaneH; c++)
	{
		for (int r = 0; r < s_physPlaneW; r++)
		{
			const int vertexId = r + c * s_physPlaneW;

			const float tcX = static_cast<float>(r) / static_cast<float>(s_physPlaneW - 1);
			const float tcY = static_cast<float>(c) / static_cast<float>(s_physPlaneH - 1);

			const float posX = (tcX - 0.5f) * 2.0f * s_physPlaneSize;
			const float posY = (tcY - 0.5f) * 2.0f * s_physPlaneSize;

			s_physPlaneVertices[vertexId] = Vector3D(posX, 0, posY);
		}
	}

	// edge turning makes more smoothed terrain
	bool turnEdge = false;
	int numTriangles = 0;
	for (int c = 0; c < s_physPlaneH - 1; c++)
	{
		for (int r = 0; r < s_physPlaneW - 1; r++)
		{
			const int index1 = r + c * s_physPlaneW;
			const int index2 = r + (c + 1) * s_physPlaneW;
			const int index3 = (r + 1) + c * s_physPlaneW;
			const int index4 = (r + 1) + (c + 1) * s_physPlaneW;

			if (!turnEdge)
			{
				s_physPlaneIndices[numTriangles * 3] = index1;
				s_physPlaneIndices[numTriangles * 3 + 1] = index2;
				s_physPlaneIndices[numTriangles * 3 + 2] = index3;
				numTriangles++;

				s_physPlaneIndices[numTriangles * 3] = index3;
				s_physPlaneIndices[numTriangles * 3 + 1] = index2;
				s_physPlaneIndices[numTriangles * 3 + 2] = index4;
				numTriangles++;
			}
			else
			{
				s_physPlaneIndices[numTriangles * 3] = index1;
				s_physPlaneIndices[numTriangles * 3 + 1] = index2;
				s_physPlaneIndices[numTriangles * 3 + 2] = index4;
				numTriangles++;

				s_physPlaneIndices[numTriangles * 3] = index1;
				s_physPlaneIndices[numTriangles * 3 + 1] = index4;
				s_physPlaneIndices[numTriangles * 3 + 2] = index3;
				numTriangles++;
			}

			turnEdge = !turnEdge;
		}
	}

	physShapeInfo_t collData;
	collData.pMaterial = nullptr; // default material
	collData.surfaceprops = "default";
	collData.indices = s_physPlaneIndices;
	collData.numIndices = s_physPlaneIdxCount;
	collData.vertices = s_physPlaneVertices;
	collData.vertexPosOffset = 0;
	collData.vertexSize = sizeof(Vector3D);
	collData.numVertices = s_physPlaneVertCount;

	physObjectInfo_t info;
	info.isStatic = true;
	info.flipXAxis = false;
	info.mass = 0.0f;
	info.data = &collData;

	g_physics->CreateStaticObject(info, COLLISION_GROUP_WORLD);
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
	InitPhysicsScene();

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
	g_physics->DestroyScene();
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::LoadModel(const char* modelPath)
{
	s_model.SetModel(nullptr);
	m_dragObj = nullptr;
	FlushCache();

	const int cacheIndex = g_studioCache->PrecacheModel(modelPath);
	if (cacheIndex == STUDIOCACHE_INVALID_IDX)
		ErrorMsg("Can't open %s\n", modelPath);

	s_model.SetModel(g_studioCache->GetModel(cacheIndex));
}

void CState_EGFViewer::ReloadModel()
{
	CEqStudioGeom* model = s_model.m_pModel;
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
			if (system(cmdLine.ToCString()) != 0)
				ErrorMsg("Failed to run command '%s'", cmdLine.ToCString());

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
				if (system(cmdLine.ToCString()) != 0)
					ErrorMsg("Failed to run command %s", cmdLine.ToCString());
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
	if (!s_model.m_pModel)
		return;

	const BoundingBox& bbox = s_model.m_pModel->GetBoundingBox();
	m_cameraDistance = length(bbox.GetSize()) * 2.0f;
}

void CState_EGFViewer::RefreshGUI()
{
	m_bodyGroupNames.clear();
	m_motionItems.clear();
	m_motionSelection = -1;
	m_poseControllerSelection = -1;
	s_model.m_bodyGroupFlags = 0xFFFFFFFF;

	if (!s_model.m_pModel)
		return;

	const studioHdr_t& studio = s_model.m_pModel->GetStudioHdr();
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
	const IVector2D& screenSize = g_pHost->GetWindowSize();

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

	if (s_model.IsPhysicsEnabled() && m_dragObj && m_dragObj->IsDynamic())
	{
		const Vector3D worldDragPoint = transformPointTransposed(m_dragObjLocalPoint, m_dragObj->GetTransformMatrix());
		const Vector3D dragForce = (m_dragTargetPoint - worldDragPoint) * m_dragObj->GetMass();

		DbgSphere().Position(worldDragPoint).Fill(true).Radius(0.05f);
		DbgLine().Start(worldDragPoint).End(m_dragTargetPoint);

		m_dragObj->ApplyImpulse(dragForce, worldDragPoint - m_dragObj->GetPosition());
	}

	g_physics->Simulate(fDt, 1);
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
		.ColorTarget(g_matSystem->GetCurrentBackbuffer(), true, ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f))
		.DepthStencilTarget(g_matSystem->GetDefaultDepthBuffer())
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

	const IVector2D& screenSize = g_pHost->GetWindowSize();
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

	const IVector2D& screenSize = g_pHost->GetWindowSize();
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
			physCast_t tr;
			g_physics->CastLine(rayStart, rayStart + rayDir * 1000.0f, COLLISION_GROUP_ALL, &tr);
			if (tr.hitObj)
			{
				m_dragObj = tr.hitObj;
				m_dragTargetPoint = m_dragStartPoint = tr.traceEnd;
				m_dragObjLocalPoint = transformPointTransposed(tr.traceEnd, !m_dragObj->GetTransformMatrix());
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
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - s_bottomPanelHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, s_bottomPanelHeight));

	constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (ImGui::Begin("##egfman2Panel", nullptr, panelFlags))
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
