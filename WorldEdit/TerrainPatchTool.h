
//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Terrain patch object tool
//////////////////////////////////////////////////////////////////////////////////

#include "SelectionEditor.h"

#ifndef TERRAINPATCHTOOL_H
#define TERRAINPATCHTOOL_H

class CTerrainPatchToolPanel;

//---------------------------------------------------
// Clipper tool - used for clipping brushes and dividing models
//---------------------------------------------------
class CTerrainPatchTool : public CSelectionBaseTool
{
public:
	CTerrainPatchTool();

	// rendering of clipper
	void		Render3D(CEditorViewRender* pViewRender);
	void		DrawSelectionBox(CEditorViewRender* pViewRender);

	void		UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void		UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D) {}

	// down/up key events
	void		OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	// begins a new selection box
	void		BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start);

	// sets selection handle and also computes planes
	void		SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle);


	// tool settings panel, can be null
	wxPanel*	GetToolPanel();

	void		InitializeToolPanel(wxWindow* pMultiToolPanel);

protected:
	CTerrainPatchToolPanel* m_panel;
};

#endif // TERRAINPATCHTOOL_H