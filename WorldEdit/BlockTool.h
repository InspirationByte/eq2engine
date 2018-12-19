//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Block/object tool
//////////////////////////////////////////////////////////////////////////////////

#include "SelectionEditor.h"

#ifndef BLOCKTOOL_H
#define BLOCKTOOL_H

//---------------------------------------------------
// Clipper tool - used for clipping brushes and dividing models
//---------------------------------------------------
class CBlockTool : public CSelectionBaseTool
{
public:
	CBlockTool();

	// rendering of clipper
	void		Render3D(CEditorViewRender* pViewRender);
	void		DrawSelectionBox(CEditorViewRender* pViewRender);

	void		UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void		UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D) {}

	// down/up key events
	void		OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	// begins a new selection box
	void		BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start);

	// sets selection handle and also computes planes
	void		SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle);

	// tool settings panel, can be null
	wxPanel*				GetToolPanel() {return NULL;}

	void					InitializeToolPanel(wxWindow* pMultiToolPanel) {}
};

#endif // BLOCKTOOL_H