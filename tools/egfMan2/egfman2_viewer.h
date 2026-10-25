//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - model viewer application state
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sys/sys_state.h"

class CEqRigidBody;
class IGPURenderPassRecorder;

enum EAppStateType
{
	APP_STATE_VIEWER = 1,
	APP_STATE_COUNT,
};

class CState_EGFViewer : public CAppStateBase
{
public:
	int			GetType() const override { return APP_STATE_VIEWER; }

	void		OnEnter(CAppStateBase* from) override;
	void		OnLeave(CAppStateBase* to) override;

	bool		Update(float fDt) override;

	void		HandleKeyPress(int key, bool down) override;
	void		HandleMouseClick(int x, int y, int buttons, bool down) override;
	void		HandleMouseMove(int x, int y, float deltaX, float deltaY) override;

	void		GetMouseCursorProperties(bool& visible, bool& centered) override;

	void		DrawUI(bool& visible);

private:
	struct MotionItem
	{
		EqString	name;
		int			sequenceId{ -1 };
	};

	void		LoadModel(const char* modelPath);
	void		ReloadModel();
	void		OpenModelDialog();
	void		CompileScriptsDialog();

	void		ResetView();
	void		SetOptimalCameraDistance();
	void		RefreshGUI();

	void		ProcessMouse(int x, int y, const Vector2D& moveDelta);
	void		WrapCursor(int x, int y);
	void		RenderFloor(IGPURenderPassRecorder* rendPassRecorder);
	void		ShowFPS(float frameTime);

	void		DrawMenuBar();
	void		DrawModelTab();
	void		DrawMotionTab();
	void		DrawPhysicsTab();
	void		DrawIKTab();

	void		OnSequenceSelected();
	void		OnPoseControllerSelected();

	// camera
	Vector3D	m_cameraRotation{ 25.0f, 225.0f, 0.0f };
	Vector3D	m_cameraTarget{ 0.0f };
	float		m_cameraDistance{ 10.0f };
	Matrix4x4	m_projMat{ identity4 };
	Matrix4x4	m_viewMat{ identity4 };

	// mouse
	int			m_mouseButtons{ 0 };
	IVector2D	m_cursorWrapPos{ 0 };
	bool		m_cursorWrapPending{ false };

	// physics object dragging
	CEqRigidBody*	m_dragObj{ nullptr };
	Vector3D	m_dragObjLocalPoint{ 0.0f };
	Vector3D	m_dragStartPoint{ 0.0f };
	Vector3D	m_dragTargetPoint{ 0.0f };

	// View menu
	bool		m_drawPhysModel{ false };
	bool		m_drawBones{ false };
	bool		m_drawAttachments{ false };
	bool		m_drawFloor{ false };
	bool		m_drawGrid{ true };
	bool		m_wireframe{ false };

	// Model tab
	Array<EqString>	m_bodyGroupNames{ PP_SL };
	int			m_lod{ 0 };
	bool		m_lodOverride{ false };

	// Motion tab
	Array<MotionItem>	m_motionItems{ PP_SL };
	int			m_motionSelection{ -1 };
	char		m_animFramerate[32]{};
	int			m_timelineFrame{ 0 };
	int			m_timelineMax{ 25 };
	int			m_animMode{ 0 };
	int			m_poseControllerSelection{ -1 };
	float		m_poseValue{ 0.0f };
	float		m_poseValueMin{ 0.0f };
	float		m_poseValueMax{ 1.0f };

	// Physics tab, not connected to anything yet
	char		m_objMass[32]{};
	char		m_objSurfProps[64]{};
	char		m_jointLimits[6][32]{};
	char		m_simTimescale[32]{};

	bool		m_selectMotionTab{ true };
};

extern CStaticAutoPtr<CState_EGFViewer> g_stateViewer;
