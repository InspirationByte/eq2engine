//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_BLOCKEDITOR_H
#define UI_BLOCKEDITOR_H

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

#include "editaxis.h"

enum EBlockEditorTools
{
	BlockEdit_Selection = 0,
	BlockEdit_Brush,
	BlockEdit_Polygon,
	BlockEdit_VertexManip,
	BlockEdit_Clipper,
};

enum EBlockEditorMode
{
	BLOCK_MODE_READY = 0,
	BLOCK_MODE_CREATION,
	BLOCK_MODE_CLIPPER,

	BLOCK_MODE_TRANSLATE,
	BLOCK_MODE_ROTATE,
	BLOCK_MODE_SCALE
};

class CBrushPrimitive;
struct winding_t;

struct brushVertexSelect_t
{
	brushVertexSelect_t() : brush(nullptr) {}
	brushVertexSelect_t(CBrushPrimitive* b) : brush(b)
	{
	}

	CBrushPrimitive* brush;
	DkList<int> vertex_ids;
};

class CUI_BlockEditor : public wxPanel, public CBaseTilebasedEditor
{
	friend class CMaterialAtlasList;
	DECLARE_EVENT_TABLE()
public:
	CUI_BlockEditor(wxWindow* parent);
	~CUI_BlockEditor();

	void				ConstructToolbars(wxBoxSizer* addTo);
	void				OnFilterTextChanged(wxCommandEvent& event);
	void				OnChangePreviewParams(wxCommandEvent& event);

	void				ProcessAllMenuCommands(wxCommandEvent& event);

	// IEditorTool stuff

	void				MouseEventOnTile(wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos);
	void				ProcessMouseEvents(wxMouseEvent& event);

	bool				ProcessSelectionAndBrushMouseEvents(wxMouseEvent& event);
	bool				ProcessVertexManipMouseEvents(wxMouseEvent& event);
	bool				ProcessClipperMouseEvents(wxMouseEvent& event);

	void				OnKey(wxKeyEvent& event, bool bDown);
	void				OnRender();

	void				InitTool();
	void				OnLevelUnload();

	void				Update_Refresh();

	float				GridSize() const;

protected:

	void				OnHistoryEvent(CUndoableObject* undoable, int eventType);

	void				ToggleBrushSelection(CBrushPrimitive* brush);
	void				ToggleFaceSelection(winding_t* winding);
	void				ToggleVertexSelection(CBrushPrimitive* brush, int vertex_idx);

	void				CancelSelection();
	void				DeleteSelection();
	void				DeleteBrush(CBrushPrimitive* brush);

	void				RecalcSelectionBox();
	void				RecalcFaceSelection();
	void				RecalcVertexSelection();

	int					GetSelectedVertsCount();
	void				MoveBrushVerts(DkList<Vector3D>& outVerts, CBrushPrimitive* brush);
	void				ClipSelectedBrushes();

	void				RenderBrushVerts(DkList<Vector3D>& verts, DkList<int>* selected_ids, const Matrix4x4& view, const Matrix4x4& proj);
	void				RenderVertsAndSelection();
	void				RenderClipper();

	void				UpdateCursor(wxMouseEvent& event, const Vector3D& ppos);

	Matrix4x4			CalcSelectionTransform(CBrushPrimitive* brush);

	CMaterialAtlasList* m_texPanel;

	//DkList<CBrushPrimitive*>	m_brushes;

	wxPanel*					m_pSettingsPanel;
	wxChoice*					m_gridSize;
	wxCheckBox*					m_textureLock;

	wxTextCtrl*					m_filtertext;
	wxTextCtrl*					m_pTags;
	

	wxCheckBox*					m_onlyusedmaterials;
	wxCheckBox*					m_pSortByDate;
	wxChoice*					m_pPreviewSize;
	wxCheckBox*					m_pAspectCorrection;

	//----------------------------------------------

	Vector3D					m_cursorPos;
	Vector3D					m_cursorDir;
	EBlockEditorTools			m_selectedTool;

	EBlockEditorMode			m_mode;
	BoundingBox					m_creationBox;
	int							m_draggablePlane;

	CEditGizmo					m_centerAxis;
	CEditGizmo					m_faceAxis;

	BoundingBox					m_selectionBox;

	DkList<CBrushPrimitive*>	m_selectedBrushes;
	DkList<winding_t*>			m_selectedFaces;
	DkList<brushVertexSelect_t>	m_selectedVerts;

	// clipper stuff
	struct {
		Vector3D start;
		Vector3D end;

		Vector3D dir;	// the plane axis
		int		 sides;		// 1 = front, 2 = back, 3 = both
	} m_clipper;

	// dragging properties
	Vector3D					m_dragOffs;
	Matrix3x3					m_dragInitRot;
	Matrix3x3					m_dragRot;

	bool						m_anyDragged;
};

#endif // UI_BLOCKEDITOR_H