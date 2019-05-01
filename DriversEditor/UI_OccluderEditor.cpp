//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#include "UI_OccluderEditor.h"
#include "EditorLevel.h"
#include "EditorMain.h"


extern Vector3D g_camera_target;

CUI_OccluderEditor::CUI_OccluderEditor( wxWindow* parent) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize )
{
	m_mode = ED_OCCL_READY;
	m_currentGizmo = -1;
}

CUI_OccluderEditor::~CUI_OccluderEditor()
{
}

// IEditorTool stuff

void CUI_OccluderEditor::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  )
{
	//int tileIdx = m_selectedRegion->GetHField()->m_sizew*ty + tx;

	if (m_selection.numElem() > 0)
	{
		Vector3D ray_start, ray_dir;
		g_pMainFrame->GetMouseScreenVectors(event.GetX(), event.GetY(), ray_start, ray_dir);

		MouseTranslateEvents(event, ray_start, ray_dir);
	}
	else
	{
		if (m_mode == ED_OCCL_POINT2)
			m_newOccl.end = ppos;

		if (!event.ControlDown() && !event.AltDown())
		{
			if (event.ButtonIsDown(wxMOUSE_BTN_LEFT) && !event.Dragging())
			{
				if (m_mode == ED_OCCL_READY)
					m_mode = ED_OCCL_POINT1;	// make to the point 1

				if (m_mode == ED_OCCL_POINT1)
				{
					m_newOccl.start = ppos;
				}
				else if (m_mode == ED_OCCL_POINT2)
				{
					m_newOccl.end = ppos;
				}

				(int)m_mode++;
				if (m_mode == ED_OCCL_DONE)
					m_mode = ED_OCCL_READY;
			}
		}
	}
}

void CUI_OccluderEditor::MouseTranslateEvents(wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir)
{
	if (m_selection.numElem() != 1)
		return;

	if (event.ButtonIsDown(wxMOUSE_BTN_LEFT))
	{
		int initAxes = 0;

		if (m_currentGizmo == -1)
		{
			for (int i = 0; i < 3; i++)
			{
				float clength = length(m_editPoints[i].m_position - g_camera_target);

				// TODO: use camera parameters
				Vector3D plane_dir = normalize(m_editPoints[i].m_position - g_camera_target);

				initAxes = m_editPoints[i].TestRay(ray_start, ray_dir, clength, true);

				if (initAxes > 0)
				{
					m_currentGizmo = i;
					break;
				}

			}
		}

		if (m_currentGizmo == -1)
			return;

		CEditGizmo& editAxis = m_editPoints[m_currentGizmo];

		// update movement
		Vector3D plane_dir = normalize(editAxis.m_position - g_camera_target);
		Vector3D movement = editAxis.PerformTranslate(ray_start, ray_dir, plane_dir, initAxes);

		levOccluderLine_t& occl = m_selection[0].region->m_occluders[m_selection[0].occIdx];

		switch (m_currentGizmo)
		{
			case 0:
				occl.start += movement;
				break;
			case 1:
				occl.end += movement;
				break;
			case 2:
				occl.height += movement.y;
				break;
		}

		editAxis.m_position += movement;
	}

	if (event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
		if (m_currentGizmo > -1)
		{
			m_editPoints[m_currentGizmo].EndDrag();
			m_currentGizmo = -1;
		}

		g_pMainFrame->NotifyUpdate();

		g_pEditorActionObserver->EndModify();
	}
}

void CUI_OccluderEditor::ProcessMouseEvents( wxMouseEvent& event )
{
	if(m_mode == ED_OCCL_READY)
	{
		if(event.ControlDown())
		{
			if(event.Dragging())
				return;

			// selection mode
			if(event.ButtonIsDown(wxMOUSE_BTN_LEFT) )
			{
				Vector3D ray_start, ray_dir;
				g_pMainFrame->GetMouseScreenVectors(event.GetX(),event.GetY(), ray_start, ray_dir);

				SelectOccluder(ray_start, ray_dir);
			}
		}
	}

	if(m_mode == ED_OCCL_HEIGHT)
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT) && !event.Dragging())
		{
			(int)m_mode++;
			if(m_mode == ED_OCCL_DONE)
			{
				// add to current region
				m_selectedRegion->m_occluders.append( m_newOccl );

				g_pMainFrame->NotifyUpdate();

				m_mode = ED_OCCL_READY;
			}
		}

		Vector3D ray_start, ray_dir;
		g_pMainFrame->GetMouseScreenVectors(event.GetX(),event.GetY(), ray_start, ray_dir);

		Vector3D nearestCheckPos = m_newOccl.end;

		Vector3D plane_dir = normalize(nearestCheckPos - g_camera_target);

		Plane pl(plane_dir, -dot(plane_dir, nearestCheckPos));

		Vector3D point;
		if( pl.GetIntersectionWithRay(ray_start, ray_dir, point) )
		{
			m_newOccl.height = fabs(point.y-nearestCheckPos.y);
		}

		return;
	}

	CBaseTilebasedEditor::ProcessMouseEvents(event);
}

void CUI_OccluderEditor::OnKey(wxKeyEvent& event, bool bDown)
{
	// hotkeys
	if(!bDown)
	{
		if(event.m_keyCode == WXK_ESCAPE)
		{
			if(m_mode == ED_OCCL_READY)
				ClearSelection();

			if(m_mode != ED_OCCL_READY)
				m_mode = ED_OCCL_READY;
		}
		else if(event.m_keyCode == WXK_DELETE)
		{
			DeleteSelection();
		}
	}
}

void CUI_OccluderEditor::SelectOccluder(const Vector3D& rayStart, const Vector3D& rayDir)
{
	if(!m_selectedRegion)
		return;

	selectedOccluder_t nearest;
	nearest.occIdx = -1;
	nearest.region = nullptr;

	float fNearestDist = DrvSynUnits::MaxCoordInUnits;

	for(int i = 0; i < m_selectedRegion->m_occluders.numElem(); i++)
	{
		levOccluderLine_t& occl = m_selectedRegion->m_occluders[i];

		Vector3D p1 = occl.start + Vector3D(0,occl.height,0);
		Vector3D p2 = occl.start;
		Vector3D p3 = occl.end;
		Vector3D p4 = occl.end + Vector3D(0,occl.height,0);

		float frac;
		if(	IsRayIntersectsTriangle(p1,p2,p3, rayStart, rayDir, frac, true) ||
			IsRayIntersectsTriangle(p3,p4,p1, rayStart, rayDir, frac, true))
		{
			if(frac < fNearestDist)
			{
				fNearestDist = frac;

				nearest.occIdx = i;
				nearest.region = m_selectedRegion;
			}
				
		}
	}

	if(nearest.occIdx != -1)
	{
		// try deselect
		for(int i = 0; i < m_selection.numElem(); i++)
		{
			if(m_selection[i].region == nearest.region &&
				m_selection[i].occIdx == nearest.occIdx)
			{
				m_selection.fastRemoveIndex(i);
				return;
			}
		}

		m_selection.append( nearest );
	}
}

int _selectionSortFn(const selectedOccluder_t& a, const selectedOccluder_t& b)
{
	return b.occIdx - a.occIdx;
}

void CUI_OccluderEditor::DeleteSelection()
{
	// sort in back order to not get crashed
	// so fastRemoveIndex will work properly without any further index correction
	m_selection.quickSort( _selectionSortFn, 0, m_selection.numElem()-1);

	for(int i = 0; i < m_selection.numElem(); i++)
	{
		int occIdx = m_selection[i].occIdx;
		m_selection[i].region->m_occluders.fastRemoveIndex(occIdx);

		g_pMainFrame->NotifyUpdate();
	}

	m_selection.clear();
}

void CUI_OccluderEditor::ClearSelection()
{
	m_selection.clear();
}

extern void ListQuadTex(const Vector3D &v1, const Vector3D &v2, const Vector3D& v3, const Vector3D& v4, int rotate, const ColorRGBA &color, DkList<Vertex3D_t> &verts);

void CUI_OccluderEditor::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	DepthStencilStateParams_t depth;

	depth.depthTest = false;
	depth.depthWrite = false;
	depth.depthFunc = COMP_LEQUAL;

	BlendStateParam_t blend;

	blend.blendEnable = true;
	blend.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blend.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	RasterizerStateParams_t raster;

	raster.cullMode = CULL_NONE;
	raster.fillMode = FILL_SOLID;
	raster.multiSample = true;
	raster.scissor = false;

	DkList<Vertex3D_t> occluderQuadVerts(64);

	if(m_selectedRegion)
	{
		CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];

		field->DebugRender(false,m_mouseOverTileHeight);

		// TODO: all occluders:
		occluderQuadVerts.resize( m_selectedRegion->m_occluders.numElem()*6 );

		for(int i = 0; i < m_selectedRegion->m_occluders.numElem(); i++)
		{
			levOccluderLine_t& occl =  m_selectedRegion->m_occluders[i];
			
			Vector3D p1 = occl.start + Vector3D(0,occl.height,0);
			Vector3D p2 = occl.start;
			Vector3D p3 = occl.end;
			Vector3D p4 = occl.end + Vector3D(0,occl.height,0);

			ListQuadTex(p1, p2, p3, p4, 0, color4_white, occluderQuadVerts);
		}

		// current
		if(m_mode > ED_OCCL_READY)
		{
			debugoverlay->Line3D(m_newOccl.start, m_newOccl.end, ColorRGBA(1,1,1,1), ColorRGBA(1,1,1,1), 0.0f);

			if(m_mode == ED_OCCL_HEIGHT)
			{
				Vector3D p1 = m_newOccl.start + Vector3D(0,m_newOccl.height,0);
				Vector3D p2 = m_newOccl.start;
				Vector3D p3 = m_newOccl.end;
				Vector3D p4 = m_newOccl.end + Vector3D(0,m_newOccl.height,0);

				ListQuadTex(p1, p2, p3, p4, 0, color4_white, occluderQuadVerts);
			}
		}
	}

	for(int i = 0; i < m_selection.numElem(); i++)
	{
		int occIdx = m_selection[i].occIdx;
		levOccluderLine_t& occl =  m_selection[i].region->m_occluders[occIdx];

		Vector3D p1 = occl.start + Vector3D(0,occl.height,0);
		Vector3D p2 = occl.start;
		Vector3D p3 = occl.end;
		Vector3D p4 = occl.end + Vector3D(0,occl.height,0);

		ListQuadTex(p1, p2, p3, p4, 0, ColorRGBA(1,0,0,1), occluderQuadVerts);
	}

	if(occluderQuadVerts.numElem())
		materials->DrawPrimitivesFFP(PRIM_TRIANGLES, occluderQuadVerts.ptr(), occluderQuadVerts.numElem(), NULL, ColorRGBA(0.6f,0.6f,0.6f,0.5f), &blend, &depth, &raster);

	CBaseTilebasedEditor::OnRender();

	float clength1 = length(m_editPoints[0].m_position - g_camera_target);
	float clength2 = length(m_editPoints[1].m_position - g_camera_target);
	float clength3 = length(m_editPoints[2].m_position - g_camera_target);

	for (int i = 0; i < m_selection.numElem(); i++)
	{
		int occIdx = m_selection[i].occIdx;
		levOccluderLine_t& occl = m_selection[i].region->m_occluders[occIdx];

		if (m_selection.numElem() == 1)
		{
			m_editPoints[0].SetProps(identity3(), occl.start);
			m_editPoints[1].SetProps(identity3(), occl.end);
			m_editPoints[2].SetProps(identity3(), occl.end + Vector3D(0, occl.height,0));

			m_editPoints[0].Draw(clength1);
			m_editPoints[1].Draw(clength2);
			m_editPoints[2].Draw(clength3);
		}
	}
}

void CUI_OccluderEditor::InitTool()
{

}

void CUI_OccluderEditor::OnLevelUnload()
{
	m_selection.clear();
	m_mode = ED_OCCL_READY;
	CBaseTilebasedEditor::OnLevelUnload();
}

void CUI_OccluderEditor::Update_Refresh()
{

}