//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity tool
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITYTOOL_H
#define ENTITYTOOL_H

#include "SelectionEditor.h"

class CEntityToolPanel;

//---------------------------------------------------
// Entity tool - for creating map dynamic objects
//---------------------------------------------------
class CEntityTool : public CSelectionBaseTool
{
public:
	CEntityTool();

	// rendering of clipper
	void		Render3D(CEditorViewRender* pViewRender);
	void		Render2D(CEditorViewRender* pViewRender);
	void		DrawSelectionBox(CEditorViewRender* pViewRender);

	void		UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// 3D manipulation for clipper is unsupported
	void		UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D);

	// down/up key events
	void		OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender);

	// tool settings panel, can be null
	wxPanel*	GetToolPanel();

	void		InitializeToolPanel(wxWindow* pMultiToolPanel);

	void		OnDeactivate();
private:

	Vector3D	m_entitypoint;
	bool		m_preparation;

	CEntityToolPanel* m_pToolPanel;
};

#endif // ENTITYTOOL_H