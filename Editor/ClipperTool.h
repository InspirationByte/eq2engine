//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection model - clipper tool
//////////////////////////////////////////////////////////////////////////////////

#ifndef CLIPPERTOOL_H
#define CLIPPERTOOL_H

#include "SelectionEditor.h"

//---------------------------------------------------
// Clipper tool - used for clipping brushes and dividing models
//---------------------------------------------------
class CClipperTool : public CSelectionBaseTool
{
public:
	CClipperTool();

	// rendering of clipper
	void		Render3D(CEditorViewRender* pViewRender);
	void		Render2D(CEditorViewRender* pViewRender);
	void		DrawSelectionBox(CEditorViewRender* pViewRender);

	void		RenderPoints(CEditorViewRender* pViewRender);

	void		UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void		UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D) {}

	// down/up key events
	void		OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	void		SetClipperPoint(Vector3D &point, Vector3D &cam_forward);

	// tool settings panel, can be null
	wxPanel*	GetToolPanel() {return NULL;}

	void		InitializeToolPanel(wxWindow* pMultiToolPanel) {}
private:

	Vector3D	m_clipperTriangle[3];
	int			m_nPointsDefined;

	int			m_nSelectedPoint;
};

#endif // CLIPPERTOOL_H