//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection model - clipper tool
//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "ClipperTool.h"
#include "EditableSurface.h"
#include "EqBrush.h"
#include "IDebugOverlay.h"

CClipperTool::CClipperTool()
{
	m_nPointsDefined = 0;
	m_nSelectedPoint = -2;
}

// rendering of clipper
void CClipperTool::Render3D(CEditorViewRender* pViewRender)
{
	g_pSelectionTools[0]->Render3D(pViewRender);

	if(m_nPointsDefined == 3)
	{
		// draw only single point
		Vertex3D_t clipper_line[] = {	Vertex3D_t(m_clipperTriangle[0],vec2_zero), Vertex3D_t(m_clipperTriangle[1],vec2_zero),
										Vertex3D_t(m_clipperTriangle[1],vec2_zero), Vertex3D_t(m_clipperTriangle[2],vec2_zero),
										Vertex3D_t(m_clipperTriangle[2],vec2_zero), Vertex3D_t(m_clipperTriangle[0],vec2_zero)};

		materials->DrawPrimitivesFFP(PRIM_LINES, clipper_line, elementsOf(clipper_line), NULL, ColorRGBA(0,1,0,1));

		Vertex3D_t draw_tri[] = {	Vertex3D_t(m_clipperTriangle[0],vec2_zero), Vertex3D_t(m_clipperTriangle[1],vec2_zero),
									Vertex3D_t(m_clipperTriangle[2],vec2_zero)};

		materials->DrawPrimitivesFFP(PRIM_TRIANGLES, draw_tri, elementsOf(draw_tri), NULL, ColorRGBA(0.6,0.6,1,0.25));

		materials->Setup2D(pViewRender->Get2DDimensions().x,pViewRender->Get2DDimensions().y);
	}

	RenderPoints(pViewRender);
}

void CClipperTool::Render2D(CEditorViewRender* pViewRender)
{
	materials->Setup2D(pViewRender->Get2DDimensions().x,pViewRender->Get2DDimensions().y);
	RenderPoints(pViewRender);
}

void CClipperTool::RenderPoints(CEditorViewRender* pViewRender)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	if(m_nPointsDefined == 1)
	{
		Vector2D pointScrA;
		PointToScreen(m_clipperTriangle[0], pointScrA, proj*view, pViewRender->Get2DDimensions());

		debugoverlay->Text3D(m_clipperTriangle[0], MAX_COORD_UNITS, color4_white, 0.0f, "1");

		// draw only single point
		Vertex2D_t pointA[] = {MAKETEXQUAD(pointScrA.x-3,pointScrA.y-3,pointScrA.x+3,pointScrA.y+3, 0)};

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,1,0,1));

		// draw only single point
		Vertex2D_t pointR[] = {MAKETEXRECT(pointScrA.x-3,pointScrA.y-3,pointScrA.x+3,pointScrA.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
	}
	else if(m_nPointsDefined == 2)
	{
		Vector2D pointScrA, pointScrB;
		PointToScreen(m_clipperTriangle[0], pointScrA, proj*view, pViewRender->Get2DDimensions());
		PointToScreen(m_clipperTriangle[1], pointScrB, proj*view, pViewRender->Get2DDimensions());

		debugoverlay->Text3D(m_clipperTriangle[0], MAX_COORD_UNITS, color4_white, 0.0f, "1");
		debugoverlay->Text3D(m_clipperTriangle[1], MAX_COORD_UNITS, color4_white, 0.0f, "2");

		// draw only single point
		Vertex2D_t pointA[] = {MAKETEXQUAD(pointScrA.x-3,pointScrA.y-3,pointScrA.x+3,pointScrA.y+3, 0)};
		Vertex2D_t pointB[] = {MAKETEXQUAD(pointScrB.x-3,pointScrB.y-3,pointScrB.x+3,pointScrB.y+3, 0)};

		Vertex2D_t clipper_line[] = {Vertex2D(pointScrA,vec2_zero), Vertex2D(pointScrB,vec2_zero)};
		materials->DrawPrimitives2DFFP(PRIM_LINES, clipper_line, elementsOf(clipper_line), NULL, ColorRGBA(0,1,0,1));

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,1,0,1));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointB, elementsOf(pointB), NULL, ColorRGBA(1,1,0,1));

		Vertex2D_t pointRA[] = {MAKETEXRECT(pointScrA.x-3,pointScrA.y-3,pointScrA.x+3,pointScrA.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointRA, elementsOf(pointRA), NULL, ColorRGBA(0,0,0,1));

		Vertex2D_t pointRB[] = {MAKETEXRECT(pointScrB.x-3,pointScrB.y-3,pointScrB.x+3,pointScrB.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointRB, elementsOf(pointRB), NULL, ColorRGBA(0,0,0,1));
	}
	else if(m_nPointsDefined == 3)
	{
		Vector2D pointScr1, pointScr2, pointScr3;
		PointToScreen(m_clipperTriangle[0], pointScr1, proj*view, pViewRender->Get2DDimensions());
		PointToScreen(m_clipperTriangle[1], pointScr2, proj*view, pViewRender->Get2DDimensions());
		PointToScreen(m_clipperTriangle[2], pointScr3, proj*view, pViewRender->Get2DDimensions());

		debugoverlay->Text3D(m_clipperTriangle[0], MAX_COORD_UNITS, color4_white, 0.0f, "1");
		debugoverlay->Text3D(m_clipperTriangle[1], MAX_COORD_UNITS, color4_white, 0.0f, "2");
		debugoverlay->Text3D(m_clipperTriangle[2], MAX_COORD_UNITS, color4_white, 0.0f, "3");

		// draw only single point
		Vertex2D_t point1[] = {MAKETEXQUAD(pointScr1.x-3,pointScr1.y-3,pointScr1.x+3,pointScr1.y+3, 0)};
		Vertex2D_t point2[] = {MAKETEXQUAD(pointScr2.x-3,pointScr2.y-3,pointScr2.x+3,pointScr2.y+3, 0)};
		Vertex2D_t point3[] = {MAKETEXQUAD(pointScr3.x-3,pointScr3.y-3,pointScr3.x+3,pointScr3.y+3, 0)};

		Vertex2D_t clipper_line[] = {	Vertex2D(pointScr1,vec2_zero), Vertex2D(pointScr2,vec2_zero),
										Vertex2D(pointScr2,vec2_zero), Vertex2D(pointScr3,vec2_zero),
										Vertex2D(pointScr3,vec2_zero), Vertex2D(pointScr1,vec2_zero)};

		materials->DrawPrimitives2DFFP(PRIM_LINES, clipper_line, elementsOf(clipper_line), NULL, ColorRGBA(0,1,0,1));

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, point1, elementsOf(point1), NULL, ColorRGBA(1,1,0,1));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, point2, elementsOf(point2), NULL, ColorRGBA(1,1,0,1));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, point3, elementsOf(point3), NULL, ColorRGBA(1,1,0,1));

		Vertex2D_t pointR1[] = {MAKETEXRECT(pointScr1.x-3,pointScr1.y-3,pointScr1.x+3,pointScr1.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR1, elementsOf(pointR1), NULL, ColorRGBA(0,0,0,1));

		Vertex2D_t pointR2[] = {MAKETEXRECT(pointScr2.x-3,pointScr2.y-3,pointScr2.x+3,pointScr2.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR2, elementsOf(pointR2), NULL, ColorRGBA(0,0,0,1));

		Vertex2D_t pointR3[] = {MAKETEXRECT(pointScr3.x-3,pointScr3.y-3,pointScr3.x+3,pointScr3.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR3, elementsOf(pointR3), NULL, ColorRGBA(0,0,0,1));
	}
}

void CClipperTool::DrawSelectionBox(CEditorViewRender* pViewRender)
{

}

void CClipperTool::SetClipperPoint(Vector3D &point, Vector3D &cam_forward)
{
	if(m_nSelectedPoint < 0)
		return;

	m_clipperTriangle[m_nSelectedPoint] = point;

	if(m_nPointsDefined == 2)
		m_clipperTriangle[2] = point + cam_forward*(float)g_gridsize;
}

void CClipperTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, IVector2D &delta2D)
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
		if(!mouseEvents.Dragging() && m_nSelectedPoint != -2 && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			m_nSelectedPoint = -1;

			// check mouseover
			for(int i = 0; i < m_nPointsDefined; i++)
			{
				Vector2D pointScr;
				PointToScreenOrtho(m_clipperTriangle[i], pointScr, proj*view);

				pointScr *= 0.5f;
				pointScr = pointScr*screenDims+screenDims*0.5f;
				pointScr.y = screenDims.y - pointScr.y;

				if(length(mouse_position - pointScr) < 8.0)
				{
					m_nSelectedPoint = i;
					break;
				}
			}
		}

		if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.Dragging() && m_nPointsDefined != 3 && m_nSelectedPoint < 0)
		{
			m_clipperTriangle[ m_nPointsDefined ] = ray_start;

			if(m_nPointsDefined == 1)
				m_clipperTriangle[2] = ray_start + view.rows[2].xyz()*(float)g_gridsize;

			m_nSelectedPoint = m_nPointsDefined;

			m_nPointsDefined++;

			g_editormainframe->UpdateAllWindows();
		}

		if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && mouseEvents.Dragging() && m_nPointsDefined == 1 && m_nSelectedPoint == 0)
		{
			m_nSelectedPoint++;
			m_nPointsDefined++;
		}


		/*
		if(m_nSelectedPoint == -2)
		{
			
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && mouseEvents.Dragging() && m_nPointsDefined > 0)
			{
				m_clipperTriangle[m_nPointsDefined] = ray_start;
				m_nPointsDefined++;
				g_editormainframe->UpdateAllWindows();
			}
			else 
			

			if(mouseEvents.ButtonUp(wxMOUSE_BTN_LEFT))
				m_nSelectedPoint = -1;
		}
		*/

		if(m_nPointsDefined > 3)
			m_nPointsDefined = 3;

		/*
		if(m_nSelectedPoint == -2)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && mouseEvents.Dragging() && m_nPointsDefined != 0)
			{
				m_clipperTriangle[2] = ray_start;
				m_nPointsDefined = 2;
				g_editormainframe->UpdateAllWindows();
			}
			else if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_nPointsDefined == 0)
			{
				m_clipperTriangle[0] = ray_start;
				m_clipperTriangle[1] = ray_start + view.rows[2].xyz()*g_gridsize;

				m_nPointsDefined = 1;
				g_editormainframe->UpdateAllWindows();
			}

			if(mouseEvents.ButtonUp(wxMOUSE_BTN_LEFT))
				m_nSelectedPoint = -1;
		}
		else if (m_nSelectedPoint >= 0)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && mouseEvents.Dragging() && m_nPointsDefined != 0)
			{
				// drag selected point
				m_clipperTriangle[m_nSelectedPoint] = ray_start;
				g_editormainframe->UpdateAllWindows();

				if(m_nSelectedPoint == 0 && m_nPointsDefined != 2)
					m_clipperTriangle[1] = ray_start + view.rows[2].xyz()*g_gridsize;
			}
		}
		*/

		if (m_nSelectedPoint >= 0)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && mouseEvents.Dragging() && m_nPointsDefined != 0 && m_nSelectedPoint >= 0)
			{
				// drag selected point
				//m_clipperTriangle[m_nSelectedPoint] = ray_start;

				Vector3D addPoint = view.rows[0].xyz()*sign(view.rows[0].xyz())*ray_start + view.rows[1].xyz()*sign(view.rows[1].xyz())*ray_start;
				Vector3D oldPoint = m_clipperTriangle[m_nSelectedPoint]*view.rows[2].xyz()*sign(view.rows[2].xyz());

				SetClipperPoint(oldPoint+addPoint, sign(view.rows[2].xyz()));

				g_editormainframe->UpdateAllWindows();

				//if(m_nSelectedPoint == 0 && m_nPointsDefined != 2)
				//	m_clipperTriangle[1] = ray_start + view.rows[2].xyz()*g_gridsize;
			}
		}
	
		
	}
}

struct clipperdata_t
{
	Vector3D* clipperTriangle;
	int clip_mode;
};

bool ClipObjects(CBaseEditableObject* selection, void* userdata)
{
	clipperdata_t clip_data = *(clipperdata_t*)userdata;

	Vector3D	*clipperTriangle = clip_data.clipperTriangle;

	if(selection->GetType() == EDITABLE_STATIC || selection->GetType() == EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurface = (CEditableSurface*)selection;

		CEditableSurface* pNewSurface1 = NULL;
		CEditableSurface* pNewSurface2 = NULL;

		Plane triPlane1(clipperTriangle[0], clipperTriangle[1], clipperTriangle[2], true);
		Plane triPlane2(clipperTriangle[2], clipperTriangle[1], clipperTriangle[0], true);

		if(clip_data.clip_mode == 0)
		{
			pSurface->CutSurfaceByPlane(triPlane1, &pNewSurface1);
			pSurface->CutSurfaceByPlane(triPlane2, &pNewSurface2);

			if(pNewSurface1)
				g_pLevel->AddEditable(pNewSurface1);

			if(pNewSurface2)
				g_pLevel->AddEditable(pNewSurface2);
		}
		else if(clip_data.clip_mode == 1)
		{
			pSurface->CutSurfaceByPlane(triPlane1, &pNewSurface1);

			if(pNewSurface1)
			{
				g_pLevel->AddEditable(pNewSurface1);
				((CSelectionBaseTool*)g_pSelectionTools[0])->OnDoForEach_AddSelectionObject(pNewSurface1);
			}
		}
		else if(clip_data.clip_mode == 2)
		{
			pSurface->CutSurfaceByPlane(triPlane2, &pNewSurface2);

			if(pNewSurface2)
			{
				g_pLevel->AddEditable(pNewSurface2);
				((CSelectionBaseTool*)g_pSelectionTools[0])->OnDoForEach_AddSelectionObject(pNewSurface2);
			}
		}

		return true;

	}
	else if(selection->GetType() == EDITABLE_BRUSH)
	{
		CEditableBrush* pBrush = (CEditableBrush*)selection;
		
		CEditableBrush* pNewBrush1 = NULL;
		CEditableBrush* pNewBrush2 = NULL;

		Plane triPlane1(clipperTriangle[2], clipperTriangle[1], clipperTriangle[0]);
		Plane triPlane2(clipperTriangle[0], clipperTriangle[1], clipperTriangle[2]);

		if(clip_data.clip_mode == 0)
		{
			pBrush->CutBrushByPlane(triPlane1, &pNewBrush1);
			pBrush->CutBrushByPlane(triPlane2, &pNewBrush2);

			if(pNewBrush1)
				g_pLevel->AddEditable(pNewBrush1);

			if(pNewBrush2)
				g_pLevel->AddEditable(pNewBrush2);
		}
		else if(clip_data.clip_mode == 1)
		{
			pBrush->CutBrushByPlane(triPlane1, &pNewBrush1);

			if(pNewBrush1)
			{
				g_pLevel->AddEditable(pNewBrush1);
				((CSelectionBaseTool*)g_pSelectionTools[0])->OnDoForEach_AddSelectionObject(pNewBrush1);
			}
		}
		else if(clip_data.clip_mode == 2)
		{
			pBrush->CutBrushByPlane(triPlane2, &pNewBrush2);

			if(pNewBrush2)
			{
				g_pLevel->AddEditable(pNewBrush2);
				((CSelectionBaseTool*)g_pSelectionTools[0])->OnDoForEach_AddSelectionObject(pNewBrush2);
			}
		}

		return true;
	}

	return false;
}

// down/up key events
void CClipperTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{
	if(!bDown)
	{
		if(event.GetKeyCode() == WXK_ESCAPE)
		{
			m_nPointsDefined = 0;
			m_nSelectedPoint = -2;
			g_editormainframe->UpdateAllWindows();
		}

		if(event.m_keyCode == WXK_RETURN)
		{
			// Create undo
			((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

			m_nPointsDefined = 0;
			m_nSelectedPoint = -2;

			clipperdata_t clip_data;
			clip_data.clipperTriangle = m_clipperTriangle;
			clip_data.clip_mode = 0;

			if(event.GetModifiers() & wxMOD_CONTROL)
				clip_data.clip_mode = 1;

			if(event.GetModifiers() & wxMOD_ALT)
				clip_data.clip_mode = 2;

			((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(ClipObjects,&clip_data,true);
			g_editormainframe->UpdateAllWindows();
		}
	}
}