//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Move / rotate / scale gizmo of the editors
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IGPURenderPassRecorder;

#define EDAXIS_SCALE 0.15f

enum EAxisSelectionFlags
{
	AXIS_NONE = 0,

	AXIS_X = (1 << 0),
	AXIS_Y = (1 << 1),
	AXIS_Z = (1 << 2),

	AXIS_ALL = AXIS_X | AXIS_Y | AXIS_Z
};

enum class EGizmoMode
{
	TRANSLATE,
	ROTATE,
	SCALE,
};

enum EGizmoMouseEvent
{
	GIZMO_MOUSE_MOVE = 0,
	GIZMO_MOUSE_DOWN,		// the button that drags, usually the left one
	GIZMO_MOUSE_UP,
};

// Move / rotate gizmo of the editors and tools. The mouse input either goes to HandleMouse, which
// highlights the axes under the cursor and runs the drag with the handlers set by SetDragHandlers,
// or is handled by the caller with TestRay, BeginDrag*, ProcessDrag and EndDrag.
class CEditGizmo
{
public:
	using DragBeginFunc = EqFunction<bool(int axes)>;
	using DragTranslateFunc = EqFunction<void(const Vector3D& movementDelta)>;
	using DragRotateFunc = EqFunction<void(const Matrix3x3& rotationDelta)>;
	using DragCompleteFunc = EqFunction<void(bool cancel)>;

	CEditGizmo() = default;

	void		SetProps(const Matrix3x3& rotate, const Vector3D& pos);
	void		SetProps(const Transform3D& transform);

	// the mode and the axes (EAxisSelectionFlags) HandleMouse drags and Draw draws; rotation is around one axis
	void		SetMode(EGizmoMode mode, int axes = AXIS_ALL);
	EGizmoMode	GetMode() const { return m_mode; }
	int			GetAxes() const { return m_axes; }

	// HandleMouse drag handlers: onBegin is called with the axes before the drag starts and may refuse
	// it; onTranslate gets the movement since the last call, onRotate the rotation since the drag start
	// (as edtRotatePointAround and the level objects take it); onComplete gets cancel from Cancel
	void		SetDragHandlers(const DragBeginFunc& onBegin, const DragTranslateFunc& onTranslate, const DragRotateFunc& onRotate, const DragCompleteFunc& onComplete);

	// the cursor ray in the world, camDist sets the gizmo size as in Draw; true when the gizmo takes the
	// event (a drag started or runs), then the caller does not select or pick under it
	bool		HandleMouse(EGizmoMouseEvent event, const Vector3D& rayStart, const Vector3D& rayDir, float camDist);

	// cancels the drag (Escape), onComplete gets cancel
	void		Cancel();

	// draws in the mode and the axes of SetMode
	void		Draw(float camDist, IGPURenderPassRecorder* rendPassRecorder) const { Draw(camDist, m_axes, m_mode, rendPassRecorder); }

	void		BeginDragTranslate(int initAxes, const DragTranslateFunc& dragFunc, const DragCompleteFunc& onComplete = nullptr);
	void		BeginDragRotate(int initAxes, const DragRotateFunc& dragFunc, const DragCompleteFunc& onComplete = nullptr);

	bool		IsDragging() const;
	int			GetDraggedAxes() const;
	void		EndDrag(bool cancel = false);

	void		ProcessDrag(const Vector3D& rayStart, const Vector3D& rayDir);

	void		SetHighlightAxes(int axes) { m_highlightAxes = axes; }
	int			GetHighlightAxes() const { return m_draggedAxes ? m_draggedAxes : m_highlightAxes; }

	int			TestRay(const Vector3D& start, const Vector3D& dir, float camDist, bool testPlanes = true) const;
	void		Draw(float camDist, int axes, EGizmoMode mode, IGPURenderPassRecorder* rendPassRecorder) const;

	// DEPRECATED
	Vector3D	PerformTranslate(const Vector3D& rayStart, const Vector3D& rayDir, int initAxes);

	// DEPRECATED
	Matrix3x3	PerformRotation(const Vector3D& rayStart, const Vector3D& rayDir, int initAxes);
	
	// DEPRECATED public
	Transform3D	m_transform;

protected:
	DragTranslateFunc	m_dragFuncTranslate;
	DragRotateFunc		m_dragFuncRotate;
	DragCompleteFunc	m_dragFuncComplete;

	DragBeginFunc		m_onBegin;
	DragTranslateFunc	m_onTranslate;
	DragRotateFunc		m_onRotate;
	DragCompleteFunc	m_onComplete;
	Matrix3x3			m_dragInitRot{ identity3 };
	EGizmoMode			m_mode{ EGizmoMode::TRANSLATE };
	int					m_axes{ AXIS_ALL };
	bool				m_dragRotStarted{ false };

	Vector3D	m_dragStart{ vec3_zero };
	Vector3D	m_dragDirCur{ vec3_zero };
	int			m_draggedAxes{ 0 };
	int			m_highlightAxes{ 0 };
	bool		m_dragMode{ false };
};