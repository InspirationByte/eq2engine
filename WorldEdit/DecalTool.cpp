//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal tool
//////////////////////////////////////////////////////////////////////////////////

#include "Decaltool.h"
#include "IDebugOverlay.h"
#include "EditableDecal.h"

#include "MaterialListFrame.h"

CDecalTool::CDecalTool()
{
	m_preparationstate = 0;
	m_fBoxSize = 0.0f;
	m_point = vec3_zero;
}

// rendering of clipper
void CDecalTool::Render3D(CEditorViewRender* pViewRender)
{
	if(m_preparationstate == 1)
	{
		debugoverlay->Box3D(m_point-Vector3D(m_fBoxSize),m_point+Vector3D(m_fBoxSize), ColorRGBA(1,0,0,1),0);
	}
}

void CDecalTool::Render2D(CEditorViewRender* pViewRender)
{

}

void CDecalTool::DrawSelectionBox(CEditorViewRender* pViewRender)
{

}

void CDecalTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{

}

// 3D manipulation for clipper is unsupported
void CDecalTool::UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
{
	// only 3D view has actions

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
			if(m_preparationstate == 1)
			{
				// place decal

				CEditableDecal* pDecal = new CEditableDecal;

				pDecal->Translate(m_point-normalize(ray_dir)*m_fBoxSize*0.25f);
				pDecal->Scale(Vector3D(m_fBoxSize));

				pDecal->GetSurfaceTexture(0)->pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

				g_pLevel->AddEditable(pDecal);

				m_preparationstate = 0;

				return;
			}

			Vector3D hit_pos;
			CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start, ray_start+ray_dir, hit_pos, NR_FLAG_BRUSHES | NR_FLAG_SURFACEMODS | NR_FLAG_INCLUDEGROUPPED);
			if(pObject)
			{
				m_preparationstate = 1;
				m_point = hit_pos;
				SnapVector3D(m_point);
			}
		}
	}

	if(m_preparationstate == 1)
	{
		Vector3D forward = view.rows[2].xyz();

		// setup plane
		Plane pl(forward.x,forward.y,forward.z, -dot(forward, m_point));

		Vector3D intersect;

		if(pl.GetIntersectionWithRay(ray_start, ray_dir,intersect))
			m_fBoxSize = length(intersect - m_point);
	}
}

// down/up key events
void CDecalTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{

}

void CDecalTool::OnDeactivate()
{
	m_preparationstate = 0;
}
