//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal tool
//////////////////////////////////////////////////////////////////////////////////

#ifndef DECALTOOL_H
#define DECALTOOL_H

#include "SelectionEditor.h"

//---------------------------------------------------
// decal tool - creates decal by click
//---------------------------------------------------
class CDecalTool : public CSelectionBaseTool
{
public:
	CDecalTool();

	// rendering of clipper
	void					Render3D(CEditorViewRender* pViewRender);
	void					Render2D(CEditorViewRender* pViewRender);
	void					DrawSelectionBox(CEditorViewRender* pViewRender);

	void					UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void					UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D);

	// down/up key events
	void					OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	void					OnDeactivate();

	// tool settings panel, can be null
	wxPanel*				GetToolPanel() {return NULL;}

	void					InitializeToolPanel(wxWindow* pMultiToolPanel) {}

private:

	Vector3D				m_point;
	float					m_fBoxSize;
	int						m_preparationstate;
};

#endif // DECALTOOL_H