//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Vertex tool
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXTOOL_H
#define VERTEXTOOL_H

#include "SelectionEditor.h"

enum VertexEditMode_e
{
	VERTEXEDIT_VERTEX = 0,
	VERTEXEDIT_TRIANGLE,
};

struct brushpolyselected_t
{
	int face_id;
	DkList<int> vertex_ids;
};

class CEditableBrush;
class CEditableSurface;
struct EqBrushPoly_t;
class CVertexToolPanel;

struct brushVertexSelectionData_t
{
	CEditableBrush*					pBrush;
	DkList<brushpolyselected_t>		selected_polys;
};

struct surfaceSelectionData_t
{
	CEditableSurface*	pSurface;
	DkList<int>			vertex_ids;
};

struct vertexSelectionData_t
{
	DkList<brushVertexSelectionData_t>	selectedBrushes;
	DkList<surfaceSelectionData_t>		selectedSurfaces;
};

struct surfaceTriangleSelectionData_t
{
	CEditableSurface*	pSurface;
	DkList<int>			triangle_ids;
};

struct triangleSelectionData_t
{
	DkList<surfaceTriangleSelectionData_t>	 selectedSurfaces;
};

//---------------------------------------------------
// Clipper tool - used for clipping brushes and dividing models
//---------------------------------------------------
class CVertexTool : public CSelectionBaseTool
{
public:
	CVertexTool();

	// rendering of clipper
	void					Render3D(CEditorViewRender* pViewRender);
	void					Render2D(CEditorViewRender* pViewRender);
	void					DrawSelectionBox(CEditorViewRender* pViewRender);

	void					UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void					UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D);

	void					MoveSelectedVerts(Vector3D &delta, bool bApplyBrushes);
	void					SnapSelectedVertsToGrid();

	void					MoveSelectionTexCoord(Vector2D &delta);

	void					UpdateSelectionCenter();

	// down/up key events
	void					OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	// applies vertex manipulations
	void					ApplyVertexManipulation(bool clear = true, bool bApplyBrushes = true);

	void					OnDeactivate();

	// tool settings panel, can be null
	wxPanel*				GetToolPanel();

	void					InitializeToolPanel(wxWindow* pMultiToolPanel);

	// Advanced geometry editor
	void					AddVertexToCurrentEdge(Vector3D &point);
	void					AddTriangle();
	void					RemoveSelectedVerts();
	void					FlipTriangleOrders();
	void					FlipQuads();
	void					RemovePolygons();
	void					MakeSmoothingGroup();
	void					MakeSurfaceFromSelection();
	void					ExtrudeEdges();

	void					ClearSelection();

private:

	vertexSelectionData_t			m_vertexselection;
	triangleSelectionData_t			m_selectedTriangles;

	Vector3D				m_selectionoffset;
	Vector3D				m_selectioncenter;

	CVertexToolPanel*		m_pPanel;
};

#endif // VERTEXTOOL_H