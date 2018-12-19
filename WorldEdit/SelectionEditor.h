//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection model
//////////////////////////////////////////////////////////////////////////////////

#ifndef SELECTION_H
#define SELECTION_H

#include "EditorCamera.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "wx_inc.h"

enum SelectionHandle_e
{
	SH_NONE = -1,

	SH_CENTER = 0,

	SH_TOPLEFT,
	SH_TOPRIGHT,
	SH_BOTTOMLEFT,
	SH_BOTTOMRIGHT,

	SH_LEFT,
	SH_RIGHT,

	SH_TOP,
	SH_BOTTOM,

	SH_COUNT
};

inline bool IsDiagonalHandle(SelectionHandle_e handle)
{
	return (handle > SH_CENTER) && (handle < SH_LEFT);
}

inline void TranslateHandleType(SelectionHandle_e handle, SelectionHandle_e outtypes[2])
{
	// convert diagonal handles to directionals
	if(IsDiagonalHandle(handle))
	{
		switch(handle)
		{
		case SH_TOPLEFT:
			outtypes[0] = SH_TOP;
			outtypes[1] = SH_LEFT;
			break;
		case SH_TOPRIGHT:
			outtypes[0] = SH_TOP;
			outtypes[1] = SH_RIGHT;
			break;
		case SH_BOTTOMLEFT:
			outtypes[0] = SH_BOTTOM;
			outtypes[1] = SH_LEFT;
			break;
		case SH_BOTTOMRIGHT:
			outtypes[0] = SH_BOTTOM;
			outtypes[1] = SH_RIGHT;
			break;
		}
		return;
	}

	outtypes[0] = handle;
	outtypes[1] = SH_NONE;
}

bool PointToScreenOrtho(const Vector3D& point, Vector2D& screen, Matrix4x4 &mvp);

void SelectionRectangleFromBBox(CEditorViewRender* pViewRender, const Vector3D &mins, const Vector3D &maxs, Rectangle_t &rect);	// converts 3d bbox to rectangle
void SelectionBBoxFromRectangle(CEditorViewRender* pViewRender, const Rectangle_t &rect, Vector3D &mins, Vector3D &maxs);	// converts rectangle to 3d bbox

void SelectionGetHandlePositions(Rectangle_t &rect, Vector2D *handles, Rectangle_t &window_rect, float offset = 0.0f); // 2D handle position

Vector3D SelectionGetHandleDirection(CEditorViewRender* pViewRender, SelectionHandle_e type); // returns movement multipler
Vector3D GetKeyDirection(CEditorViewRender* pViewRender, int keyCode); // returns movement multipler

void SelectionGetVolumePlanes(CEditorViewRender* pViewRender, SelectionHandle_e type, Volume &vol, int *plane_ids);

void UpdateAllWindows();
void Update3DView();

void SnapVector3D(Vector3D &vec);
void SnapFloatValue(float &val);

void OnDeactivateCurrentTool();
void OnActivateCurrentTool();

//------------------------------
// Selection box implementation
//------------------------------

class CBaseEditableObject;

// callback to process each selected objects
typedef bool (*DOFOREACHSELECTED)(CBaseEditableObject* selection, void* userdata);

enum SelectionState_e
{
	SELECTION_NONE = 0,		// no box, no objects selected
	SELECTION_PREPARATION,	// box is ready, hit 'enter' to select
	SELECTION_SELECTED		// any objects is selected. If none selected, it will reset ot SELECTION_NONE
};

enum SelectionMode_e
{
	SM_GROUP = 0,
	SM_OBJECT,
	SM_VERTEX,
};

//---------------------------------------------------
// IEditorTool - special class with custom modificators
//---------------------------------------------------
class IEditorTool
{
public:

	virtual void		OnDeactivate() {}

	virtual void		InitializeToolPanel(wxWindow* pMultiToolPanel) {}

	// tool settings panel, can be null
	virtual wxPanel*	GetToolPanel() {return NULL;}

	// draws selection box and objects for movement preview
	virtual void		Render3D(CEditorViewRender* pViewRender) = 0;
	virtual void		Render2D(CEditorViewRender* pViewRender) = 0;

	virtual void		DrawSelectionBox(CEditorViewRender* pViewRender) = 0; // may be differrent in drawing

	// updates manipulation using mouse events.
	void UpdateManipulation(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
	{
		// manipulate in two modes
		if(pViewRender->GetCameraMode() == CPM_PERSPECTIVE)
		{
			UpdateManipulation3D(pViewRender, mouseEvents, delta3D, delta2D);
		}
		else
		{
			UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D);
		}
	}

	// updates manipulation, from 2D window. return false will cause doing UpdateManipulation2D from CSelectionBaseTool
	virtual void	UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D) = 0;

	// updates manipulation, from 3D window. return false will cause doing UpdateManipulation3D from CSelectionBaseTool
	virtual void	UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D) = 0;

	// down/up key events
	virtual void	OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender) = 0;
};

class CSelectionToolPanel;

//---------------------------------------------------
// Base selection tool, it's inherits the IEditorTool.
//---------------------------------------------------
class CSelectionBaseTool : public IEditorTool
{
public:
	CSelectionBaseTool();

	// does anything with each selected objects using callbacks and user data
	void							DoForEachSelectedObjects(DOFOREACHSELECTED pFunction, void *userdata, bool bUpdateSelection = false); 

	// updates manipulation using mouse events.
	virtual void					UpdateManipulation(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// updates manipulation in 2D mode
	virtual void					UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);
	
	// updates manipulation in 3D mode
	virtual void					UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// down/up key events
	virtual void					OnKey(wxKeyEvent& event, bool bDown,CEditorViewRender* pViewRender);

	// begins a new selection box
	virtual void					BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start);

	// resets state to SELECTION_NONE
	void							CancelSelection(bool bCancelSurfaces = false, bool bExitCleanup = false);

	// selects the object
	void							SelectObjects(bool selectOnlySpecified = false, CBaseEditableObject** pAddObjects = NULL, int nObjects = 0);

	// retunrs selection state
	SelectionState_e				GetSelectionState();

	// retunrs handle that cursor on (drag mode only)
	SelectionHandle_e				GetSelectionHandle();

	// sets selection handle and also computes planes
	virtual void					SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle);

	// retunrs handle that mouse over on
	virtual SelectionHandle_e		GetMouseOverSelectionHandle();

	// sets new selection state
	void							SetSelectionState(SelectionState_e state);

	// transforms by preview (applies the movement)
	void							TransformByPreview();

	// draws 3D boxes or 
	void							Render3D(CEditorViewRender* pViewRender);

	// draw 2D boxes and others
	void							Render2D(CEditorViewRender* pViewRender);

	// may be differrent in drawing
	void							DrawSelectionBox(CEditorViewRender* pViewRender);

	// returns selection bbox center
	Vector3D						GetSelectionCenter();

	// selection bbox
	void							GetSelectionBBOX(Vector3D &mins, Vector3D &maxs);

	// sets selection box.
	void							SetSelectionBBOX(Vector3D &mins, Vector3D &maxs);

	// returns pointer to selected object list
	DkList<CBaseEditableObject*>*	GetSelectedObjects() {return &m_selectedobjects;}

	// is selection box currently dragging
	bool							IsDragging() {return m_bIsDragging;}

	// adds object to selection, and objects will be selected after DoForEachSelectedObjects
	void							OnDoForEach_AddSelectionObject(CBaseEditableObject* pObject);

	bool							IsSelected(CBaseEditableObject* pObject, int &selection_id);

	void							SUB_ToggleSelection(CBaseEditableObject* pObject);

	void							InvertSelection();

	// tool settings panel, can be null
	wxPanel*						GetToolPanel();

	void							InitializeToolPanel(wxWindow* pMultiToolPanel);

	void							BackupSelectionForUndo();

	
protected:

	// selection box (as planes/volume)
	Volume							m_bbox_volume;

	SelectionHandle_e				m_mouseover_handle;

	SelectionHandle_e				m_selectedhandle;
	int								m_plane_ids[2];

	// preview transformation
	Vector3D						m_translation;
	Vector3D						m_rotation;
	Vector3D						m_scaling;

	Vector3D						m_beginupvec;
	Vector3D						m_beginrightvec;

	bool							m_bIsDragging;
	bool							m_bIsPainting;

	SelectionState_e				m_state;

	DkList<CBaseEditableObject*>	m_selectedobjects;

	DkList<CBaseEditableObject*>	m_addToSelection;

	CSelectionToolPanel*			m_selection_panel;
};

extern IEditorTool*					g_pSelectionTools[];

#endif // SELECTION_H