//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Block/object tool
//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "BlockTool.h"
#include "EqBrush.h"

CBlockTool::CBlockTool()
{

}

// rendering of clipper
void CBlockTool::Render3D(CEditorViewRender* pViewRender)
{
	CSelectionBaseTool::Render3D(pViewRender);
}

void CBlockTool::DrawSelectionBox(CEditorViewRender* pViewRender)
{
	BoundingBox bbox;

	// make various selection colors
	ColorRGB selectionbox_color(1,1,1);

	if(GetSelectionState() == SELECTION_SELECTED)
		selectionbox_color = ColorRGB(1,0,0);

	GetSelectionBBOX( bbox.minPoint, bbox.maxPoint);

	// draw ghost bbox
	pViewRender->DrawSelectionBBox( bbox.minPoint, bbox.maxPoint, IsDragging() ? ColorRGB(0.8,0.8,0) : selectionbox_color, !IsDragging());
}

void CBlockTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{
	/*
	BoundingBox bbox;
	GetSelectionBBOX(bbox.minPoint, bbox.maxPoint);
	bbox.Fix();

	if(bbox.minPoint.x >= bbox.maxPoint.x)
		bbox.maxPoint.x = bbox.minPoint.x + g_gridsize;

	if(bbox.minPoint.y >= bbox.maxPoint.y)
		bbox.maxPoint.y = bbox.minPoint.y + g_gridsize;

	if(bbox.minPoint.z >= bbox.maxPoint.z)
		bbox.maxPoint.z = bbox.minPoint.z + g_gridsize;

	SetSelectionBBOX(bbox.minPoint, bbox.maxPoint);
	*/

	// process pressing/depressing mouse events
	if(mouseEvents.GetButton() == wxMOUSE_BTN_LEFT)
	{
		if(mouseEvents.ButtonDown())
		{
			if(m_mouseover_handle == SH_NONE)
			{
				SetSelectionHandle(pViewRender, SH_NONE);
				return;
			}
		}
	}

	CSelectionBaseTool::UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D);
}

// down/up key events
void CBlockTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{
	if(!bDown)
	{
		if(event.m_keyCode == WXK_RETURN)
		{
			if(GetSelectionState() == SELECTION_PREPARATION)
			{
				BoundingBox bbox;
				GetSelectionBBOX(bbox.minPoint, bbox.maxPoint);

				if(!bbox.IsEmpty())
				{
					Volume bboxvol;
					bboxvol.LoadBoundingBox( bbox.maxPoint, bbox.minPoint, true);
					
					CEditableBrush* pBrush = CreateBrushFromVolume(&bboxvol);
					g_pLevel->AddEditable(pBrush);

					g_editormainframe->UpdateAllWindows();
				}
				else
				{
					MsgError("\nERROR: Can't create empty brush!\n");
				}
			}
		}
		else if(event.GetKeyCode() == WXK_ESCAPE)
			CancelSelection();
	}
}

// begins a new selection box
void CBlockTool::BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	// get old bbox
	BoundingBox oldselection;
	m_bbox_volume.GetBBOXBack(oldselection.minPoint, oldselection.maxPoint);
	oldselection.Fix();

	if(oldselection.minPoint.x >= oldselection.maxPoint.x)
		oldselection.maxPoint.x = oldselection.minPoint.x + g_gridsize;

	if(oldselection.minPoint.y >= oldselection.maxPoint.y)
		oldselection.maxPoint.y = oldselection.minPoint.y + g_gridsize;

	if(oldselection.minPoint.z >= oldselection.maxPoint.z)
		oldselection.maxPoint.z = oldselection.minPoint.z + g_gridsize;

	// set a new bbox
	m_bbox_volume.LoadBoundingBox(-oldselection.minPoint * view.rows[2].xyz() + start,-oldselection.maxPoint * view.rows[2].xyz() + start, true);

	// set state to preparation
	m_state = SELECTION_PREPARATION;

	SetSelectionHandle(pViewRender, SH_BOTTOMRIGHT);
}

// sets selection handle and also computes planes
void CBlockTool::SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle)
{
	CSelectionBaseTool::SetSelectionHandle(pViewRender,handle);
}