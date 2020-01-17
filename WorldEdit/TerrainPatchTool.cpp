//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Block/object tool
//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "TerrainPatchTool.h"
#include "EditableSurface.h"
#include "wx_inc.h"

// create a terrain patch dialog
class CTerrainPatchToolPanel : public wxPanel
{
public:
	CTerrainPatchToolPanel(wxWindow* pMultiToolPanel) : wxPanel(pMultiToolPanel,-1, wxPoint(0,0),wxSize(200,400))
	{
		wxString size_choices[] = 
		{
			"4",
			"8",
			"16",
			"32",
			"64",
		};

		new wxStaticText(this, -1, "Wide", wxPoint(5,5));
		new wxStaticText(this, -1, "Tall", wxPoint(5,30));
		new wxStaticText(this, -1, "Tesselation", wxPoint(5,55));

		m_pWideBox = new wxComboBox(this, -1, size_choices[0], wxPoint(60,5), wxSize(120, 25), 5, size_choices);
		m_pTallBox = new wxComboBox(this, -1, size_choices[0], wxPoint(60,30), wxSize(120, 25), 5, size_choices);

		wxString tesselation_choices[] = 
		{
			"None",
			"4",
			"6",
			"8",
		};

		m_pTesselation = new wxComboBox(this, -1, tesselation_choices[0], wxPoint(60,55), wxSize(120, 25), 4, tesselation_choices);
		m_pTesselation->SetSelection(0);
	}

	int GetPatchWide()
	{
		return atoi(m_pWideBox->GetValue());
	}

	int GetPatchTall()
	{
		return atoi(m_pTallBox->GetValue());
	}

	int GetTesselationLevel()
	{
		int selected = m_pTesselation->GetSelection();

		if(selected == 0)
			return -1;

		return 2 + selected * 2;
	}

protected:

	wxComboBox*				m_pWideBox;
	wxComboBox*				m_pTallBox;
	wxComboBox*				m_pTesselation;
};

CTerrainPatchTool::CTerrainPatchTool()
{

}


// tool settings panel, can be null
wxPanel* CTerrainPatchTool::GetToolPanel()
{
	return m_panel;
}

void CTerrainPatchTool::InitializeToolPanel(wxWindow* pMultiToolPanel)
{
	m_panel = new CTerrainPatchToolPanel(pMultiToolPanel);
}

// rendering of clipper
void CTerrainPatchTool::Render3D(CEditorViewRender* pViewRender)
{
	CSelectionBaseTool::Render3D(pViewRender);
}

void CTerrainPatchTool::DrawSelectionBox(CEditorViewRender* pViewRender)
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

void CTerrainPatchTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{
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
void CTerrainPatchTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
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
					CEditableSurface* pSurface = new CEditableSurface;
					pSurface->MakeTerrainPatch(m_panel->GetPatchWide(), m_panel->GetPatchTall(), m_panel->GetTesselationLevel());

					if(pSurface)
					{
						g_pLevel->AddEditable(pSurface);
						pSurface->Scale(bbox.GetSize()/2.0f);
						pSurface->Translate(bbox.GetCenter());
					
						g_editormainframe->UpdateAllWindows();
					}
				}
				else
				{
					MsgError("\nERROR: Can't create empty terrain patch!\n");
				}
			}
		}
		else if(event.GetKeyCode() == WXK_ESCAPE)
			CancelSelection();
	}
}

// begins a new selection box
void CTerrainPatchTool::BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start)
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
void CTerrainPatchTool::SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle)
{
	CSelectionBaseTool::SetSelectionHandle(pViewRender,handle);
}