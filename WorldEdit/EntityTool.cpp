//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity tool
//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "EntityTool.h"
#include "EqBrush.h"
#include "IDebugOverlay.h"
#include "EditableEntity.h"


class CEntityToolPanel : public wxPanel
{
public:
	CEntityToolPanel(wxWindow* pMultiToolPanel) : wxPanel(pMultiToolPanel,-1,wxPoint(0,0),wxSize(200,400))
	{
		m_entList = new wxComboBox(this, -1, "<select entity>",wxPoint(5,5), wxSize(150,25));
	}

	void UpdateClassList()
	{
		EqString copy = GetSelectedClassname();

		m_entList->Clear();

		for(int i = 0; i < g_defs.numElem(); i++)
		{
			if(g_defs[i]->showinlist)
				m_entList->AppendString(wxString(g_defs[i]->classname.GetData()));
		}

		m_entList->SetValue(wxString(copy.GetData()));
	}

	char *GetSelectedClassname()
	{
		static char str[256];
		strcpy(str, m_entList->GetValue());

		return str;
	}

protected:

	wxComboBox* m_entList;
};


CEntityTool::CEntityTool()
{
	m_preparation = false;
	m_entitypoint = vec3_zero;
}

// tool settings panel, can be null
wxPanel* CEntityTool::GetToolPanel()
{
	return m_pToolPanel;
}

void CEntityTool::OnDeactivate()
{
	//m_pToolPanel->UpdateClassList();
}

void CEntityTool::InitializeToolPanel(wxWindow* pMultiToolPanel)
{
	m_pToolPanel = new CEntityToolPanel(pMultiToolPanel);
	m_pToolPanel->UpdateClassList();
}

// rendering of clipper
void CEntityTool::Render3D(CEditorViewRender* pViewRender)
{
	if(!m_preparation)
		return;

	debugoverlay->Box3D(m_entitypoint-Vector3D(16), m_entitypoint+Vector3D(16), ColorRGBA(0,1,0,1));
	debugoverlay->Line3D(m_entitypoint-Vector3D(MAX_COORD_UNITS,0,0), m_entitypoint+Vector3D(MAX_COORD_UNITS,0,0), ColorRGBA(0,1,1,1), ColorRGBA(0,1,1,1));
	debugoverlay->Line3D(m_entitypoint-Vector3D(0,MAX_COORD_UNITS,0), m_entitypoint+Vector3D(0,MAX_COORD_UNITS,0), ColorRGBA(0,1,1,1), ColorRGBA(0,1,1,1));
	debugoverlay->Line3D(m_entitypoint-Vector3D(0,0,MAX_COORD_UNITS), m_entitypoint+Vector3D(0,0,MAX_COORD_UNITS), ColorRGBA(0,1,1,1), ColorRGBA(0,1,1,1));
}

void CEntityTool::Render2D(CEditorViewRender* pViewRender)
{

}

void CEntityTool::DrawSelectionBox(CEditorViewRender* pViewRender)
{

}

void CEntityTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();
	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D ray_start, ray_dir;
	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, proj*view, true);

	g_editormainframe->SnapVector3D(ray_start);

	if(!mouseEvents.ShiftDown())
	{
		if(!m_preparation && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.Dragging())
		{
			Vector3D pointA = m_entitypoint * view.rows[2].xyz()*sign(view.rows[2].xyz());
			Vector3D mouse_point = pointA + ray_start * view.rows[0].xyz()*sign(view.rows[0].xyz()) + ray_start * view.rows[1].xyz()*sign(view.rows[1].xyz());

			SnapVector3D(mouse_point);

			m_entitypoint = mouse_point;
			m_preparation = true;
		}

		if(mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_preparation)
		{
			Vector3D pointA = m_entitypoint * view.rows[2].xyz()*sign(view.rows[2].xyz());
			Vector3D mouse_point = pointA + ray_start * view.rows[0].xyz()*sign(view.rows[0].xyz()) + ray_start * view.rows[1].xyz()*sign(view.rows[1].xyz());

			SnapVector3D(mouse_point);

			m_entitypoint = mouse_point;
		}
	}

	UpdateAllWindows();
}

// 3D manipulation for clipper is unsupported
void CEntityTool::UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();

	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D ray_start, ray_dir;

	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-(mouse_position.x-delta2D.x),mouse_position.y-delta2D.y), screenDims, ray_start, ray_dir, proj*view, false);

	if(!mouseEvents.ShiftDown())
	{
		if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.Dragging())
		{
			Vector3D hit_pos;
			CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start, ray_start+ray_dir, hit_pos, NR_FLAG_BRUSHES | NR_FLAG_SURFACEMODS | NR_FLAG_INCLUDEGROUPPED);
			if(pObject)
			{
				edef_entity_t* pDef = EDef_Find(m_pToolPanel->GetSelectedClassname());

				if(pDef)
				{
					Vector3D b_size = pDef->bboxmaxs - pDef->bboxmins;
					hit_pos -= normalize(ray_dir) * b_size*0.5f;
				}

				CEditableEntity* pEntity = new CEditableEntity;
				pEntity->SetClassName(m_pToolPanel->GetSelectedClassname());
				pEntity->SetPosition(hit_pos);

				g_pLevel->AddEditable(pEntity);
				((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
				((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, (CBaseEditableObject**)&pEntity, 1);
			}
		}
	}

	UpdateAllWindows();
}

// down/up key events
void CEntityTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{
	if(!bDown)
	{
		if(event.GetKeyCode() == WXK_ESCAPE)
		{
			m_preparation = false;
			g_editormainframe->UpdateAllWindows();
		}

		if(event.m_keyCode == WXK_RETURN)
		{
			if(m_preparation)
			{
				if(strlen(m_pToolPanel->GetSelectedClassname()) == 0)
				{
					MsgError("ERROR! You must select entity from the list\n");
				}

				CEditableEntity* pEntity = new CEditableEntity;
				pEntity->SetClassName(m_pToolPanel->GetSelectedClassname());
				pEntity->SetPosition(m_entitypoint);


				g_pLevel->AddEditable(pEntity);
				((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
				((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, (CBaseEditableObject**)&pEntity, 1);
			}

			g_editormainframe->UpdateAllWindows();

			m_preparation = false;
		}
	}
}