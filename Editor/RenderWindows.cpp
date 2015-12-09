//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior render window
//////////////////////////////////////////////////////////////////////////////////

#include "RenderWindows.h"
#include "IDebugOverlay.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "math/math_util.h"
#include "SelectionEditor.h"
#include "GameSoundEmitterSystem.h"

#include "MaterialListFrame.h"

#include "EditableEntity.h"
#include "EditableSurface.h"
#include "EqBrush.h"

#include "wx/dcbuffer.h"

#include "grid.h"

float g_realtime = 0;
float g_oldrealtime = 0;
float g_frametime = 0;

CEditorViewRender g_views[4];

void ShowFPS()
{
	// FPS status
	static float accTime = 0.1f;
	static int fps = 0;
	static int nFrames = 0;

	if (accTime > 0.1f)
	{
		fps = (int) (nFrames / accTime + 0.5f);
		nFrames = 0;
		accTime = 0;
	}

	accTime += g_frametime;
	nFrames++;

	ColorRGBA col(1,0,0,1);

	if(fps > 25)
	{
		col = ColorRGBA(1,1,0,1);
		if(fps > 45)
			col = ColorRGBA(0,1,0,1);
	}

	debugoverlay->Text(col, "FPS: %d", fps);
}

BEGIN_EVENT_TABLE(CViewWindow, wxWindow)
    EVT_SIZE(CViewWindow::OnSize)
    EVT_ERASE_BACKGROUND(CViewWindow::OnEraseBackground)
    EVT_IDLE(CViewWindow::OnIdle)
	EVT_MOUSE_EVENTS(CViewWindow::ProcessMouseEvents)
	EVT_PAINT(CViewWindow::OnPaint)

	EVT_LEFT_DOWN(ProcessMouseEnter)
	EVT_LEFT_UP(ProcessMouseLeave)
	EVT_MIDDLE_DOWN(ProcessMouseEnter)
	EVT_MIDDLE_UP(ProcessMouseLeave)
	EVT_RIGHT_DOWN(ProcessMouseEnter)
	EVT_RIGHT_UP(ProcessMouseLeave)

	EVT_KEY_DOWN(CViewWindow::ProcessKeyboardDownEvents)
	EVT_KEY_UP(CViewWindow::ProcessKeyboardUpEvents)
	EVT_CONTEXT_MENU(CViewWindow::OnContextMenu)
	EVT_SET_FOCUS(CViewWindow::OnFocus)
END_EVENT_TABLE()

void ResetViews()
{
	g_views[0].SetCameraMode(CPM_PERSPECTIVE);
	g_views[0].SetRenderMode(VRM_TEXTURED);
	g_views[0].GetView()->SetAngles(Vector3D(-25, 225,0));
	g_views[0].GetView()->SetOrigin(Vector3D(-25,25,-25));

	g_views[1].SetCameraMode(CPM_TOP);
	g_views[1].SetRenderMode(VRM_WIREFRAME);
	g_views[1].GetView()->SetAngles(Vector3D(90,0,0));
	g_views[1].GetView()->SetFOV(120);
	g_views[1].GetView()->SetOrigin(vec3_zero);

	g_views[2].SetCameraMode(CPM_RIGHT);
	g_views[2].SetRenderMode(VRM_WIREFRAME);
	g_views[2].GetView()->SetOrigin(vec3_zero);
	g_views[2].GetView()->SetAngles(Vector3D(0,90,0));
	g_views[2].GetView()->SetFOV(120);

	g_views[3].SetCameraMode(CPM_FRONT);
	g_views[3].SetRenderMode(VRM_WIREFRAME);
	g_views[3].GetView()->SetAngles(Vector3D(180,180,180));
	g_views[3].GetView()->SetFOV(120);
	g_views[3].GetView()->SetOrigin(vec3_zero);
}

CViewWindow::CViewWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size) : wxWindow( parent, -1, pos, size )
{
	ResetViews();

	m_viewid = 0;

	m_mustupdate = true;

	m_bIsMoving = false;

	//SetDoubleBuffered(true);
}

void CViewWindow::NotifyUpdate()
{
	m_mustupdate = true;
}

void CViewWindow::OnSize(wxSizeEvent &event)
{
	if(materials)
	{
		int w, h;
		g_editormainframe->GetMaxRenderWindowSize(w,h);

		materials->SetDeviceBackbufferSize(w,h);

		NotifyUpdate();
		/*
#ifdef VR_TEST
		GetSize(&w, &h);

		viewrenderer->SetViewportSize(w, h, false);
		viewrenderer->ShutdownResources();
		viewrenderer->InitializeResources();
#endif // VR_TEST*/

		Redraw();
	}
}

void CViewWindow::OnIdle(wxIdleEvent &event)
{
	event.RequestMore(true);

	Redraw();
}

void CViewWindow::OnEraseBackground(wxEraseEvent& event)
{
	
}

Vector2D last_mouse_pos(0);
wxStockCursor g_old_cursor = wxCURSOR_ARROW;

wxStockCursor GetSelectionCursor(SelectionHandle_e type)
{
	switch(type)
	{
	case SH_NONE:
		return wxCURSOR_ARROW;
		break;
	case SH_CENTER:
		return wxCURSOR_SIZING;
		break;
	case SH_TOP:
	case SH_BOTTOM:
		return wxCURSOR_SIZENS;
		break;
	case SH_LEFT:
	case SH_RIGHT:
		return wxCURSOR_SIZEWE;
		break;
	case SH_TOPRIGHT:
	case SH_BOTTOMLEFT:
		return wxCURSOR_SIZENESW;
		break;
	case SH_TOPLEFT:
	case SH_BOTTOMRIGHT:
		return wxCURSOR_SIZENWSE;
		break;
	default:
		return wxCURSOR_ARROW;
	}
}

void CViewWindow::ProcessObjectMovement(Vector3D &delta3D, Vector2D &delta2D, wxMouseEvent& event)
{
	if(g_pSelectionTools[g_editormainframe->GetToolType()])
	{
		g_pSelectionTools[g_editormainframe->GetToolType()]->UpdateManipulation(GetActiveView(), event, delta3D, delta2D);

		// try to change the mouse cursor
		//if(((CSelectionBaseTool*)g_pSelectionTools[g_editormainframe->GetToolType()])->GetSelectionState() == SELECTION_NONE || ((CSelectionBaseTool*)g_pSelectionTools[g_editormainframe->GetToolType()])->GetMouseOverSelectionHandle() == SH_NONE)
		//	return;

		
		wxStockCursor curType = GetSelectionCursor(((CSelectionBaseTool*)g_pSelectionTools[g_editormainframe->GetToolType()])->GetMouseOverSelectionHandle());
		if(curType != g_old_cursor)
		{
			wxCursor cur(curType);
			g_old_cursor = curType;
			SetCursor(cur);
		}
	}
}

#define CAM_MOVE_SPEED 200
#define CAM_ZOOM_SPEED 100

void CViewWindow::ProcessMouseEnter(wxMouseEvent& event)
{
	CaptureMouse();
	//Msg("Capture mouse\n");
}

void CViewWindow::ProcessMouseLeave(wxMouseEvent& event)
{
	//if(!event.Dragging())
	//{
		ReleaseMouse();
	//	Msg("Release mouse\n");
	//}
}

void CViewWindow::ProcessMouseEvents(wxMouseEvent& event)
{
	if(GetForegroundWindow() != g_editormainframe->GetMaterials()->GetHWND())
	{
		SetFocus();

		// windows-only
		//SetForegroundWindow( GetHWND() );
	}

	// here is an camera movement processing
	NotifyUpdate();

	Vector3D cam_angles = GetActiveView()->GetView()->GetAngles();
	Vector3D cam_pos = GetActiveView()->GetView()->GetOrigin();
	float fov = GetActiveView()->GetView()->GetFOV();

	float move_delta_x = (event.GetX() - last_mouse_pos.x);
	float move_delta_y = (event.GetY() - last_mouse_pos.y);

	wxPoint point(last_mouse_pos.x, last_mouse_pos.y);

	if(!m_bIsMoving)
	{
		m_vLastCursorPos = point;
		m_vLastClientCursorPos = ClientToScreen(point);
	}

	point = ClientToScreen(point);

	int w, h;
	GetSize(&w, &h);

	float zoom_factor = (fov/100)*0.5f*0.55f;

	// make x and y size
	float camera_move_factor = (fov/h)*2; // why height? Because x has aspect...

	//wxCursor cursor(wxCURSOR_ARROW);
	//SetCursor(cursor);

	Matrix4x4 view, proj;
	GetActiveView()->GetViewProjection(view,proj);

	Vector3D temp;

	Vector3D current_mouse_dir;
	Vector3D last_mouse_dir;

	ScreenToDirection(g_views[m_viewid].GetAbsoluteCameraPosition(), Vector2D(w-event.GetX(),event.GetY()), Vector2D(w,h), current_mouse_dir, temp, proj*view, true);
	ScreenToDirection(g_views[m_viewid].GetAbsoluteCameraPosition(), Vector2D(w-last_mouse_pos.x,last_mouse_pos.y), Vector2D(w,h), last_mouse_dir, temp, proj*view, true);

	Vector2D prev_mouse_pos = last_mouse_pos;
	last_mouse_pos = Vector2D(event.GetX(),event.GetY());

	bool bAnyMoveButton = false;

	if(event.ShiftDown())
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			if(event.Dragging() && (GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE))
			{
				cam_angles.x += move_delta_y*0.5f;
				cam_angles.y -= move_delta_x*0.5f;

				last_mouse_pos = prev_mouse_pos;
			}

			m_bIsMoving = true;
			bAnyMoveButton = true;
			//wxCursor cursor(wxCURSOR_ARROW);
			//SetCursor(cursor);
		}

		if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		{
			if(event.Dragging())
			{
				if(GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE)
				{
					Vector3D forward;
					AngleVectors(cam_angles, &forward, NULL, NULL);

					cam_pos -= forward*move_delta_y * camera_move_factor;
				}	
				else
					fov += move_delta_y*zoom_factor * g_frametime * CAM_ZOOM_SPEED;

				m_bIsMoving = true;
				bAnyMoveButton = true;

				last_mouse_pos = prev_mouse_pos;
			}

			//wxCursor cursor(wxCURSOR_MAGNIFIER);
			//SetCursor(cursor);
		}

		if(event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
		{
			if(event.Dragging())
			{
				Vector3D right, up;
				AngleVectors(cam_angles, NULL, &right, &up);

				if(GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE)
				{
					camera_move_factor *= -1;
				}
				else
				{
					right = view.rows[0].xyz() * sign(view.rows[0].xyz());
					up = view.rows[1].xyz() *  sign(view.rows[1].xyz());
				}

				m_bIsMoving = true;
				bAnyMoveButton = true;

				cam_pos -= right*move_delta_x * camera_move_factor/* * g_frametime * CAM_MOVE_SPEED*/;
				cam_pos += up*move_delta_y * camera_move_factor/* * g_frametime * CAM_MOVE_SPEED*/;

				last_mouse_pos = prev_mouse_pos;
			}
		}
	}

	if(!bAnyMoveButton)
	{
		m_bIsMoving = false;
		while(ShowCursor(TRUE) < 0);
	}

	if(m_bIsMoving)
	{
		SetCursorPos(m_vLastClientCursorPos.x, m_vLastClientCursorPos.y);
		ShowCursor(FALSE);
	}

	/*
	else
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT) && !event.ControlDown() && !event.Dragging() && g_editormainframe->GetToolType() == Tool_Selection)
			PopupMenu(g_editormainframe->GetContextMenu(), event.GetX(), event.GetY());
	}
	*/

	// process zooming
	float mouse_wheel = event.GetWheelRotation()*0.5f * zoom_factor;

	if(GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE)
		fov += mouse_wheel;
	else
		fov -= mouse_wheel;

	fov = clamp(fov,10.0f,MAX_COORD_UNITS);

	cam_pos = clamp(cam_pos, Vector3D(-MAX_COORD_UNITS), Vector3D(MAX_COORD_UNITS));

	GetActiveView()->GetView()->SetAngles(cam_angles);
	GetActiveView()->GetView()->SetOrigin(cam_pos);
	GetActiveView()->GetView()->SetFOV(fov);

	Vector3D right, up;
	AngleVectors(cam_angles, NULL, &right, &up);

	Vector3D move_3d_delta(0);

	move_3d_delta += right*move_delta_x * camera_move_factor;
	move_3d_delta -= up*move_delta_y * camera_move_factor;

	if(!event.ShiftDown() && !m_bIsMoving)
	{
		// object movement processing
		ProcessObjectMovement(move_3d_delta, Vector2D(move_delta_x,move_delta_y), event);
	}
	else
	{
		if(event.Dragging())
			SetCursorPos(point.x, point.y);
	}

	/*
	if(event.Dragging())
		CaptureMouse();
	
	if(event.LeftUp() || event.MiddleUp() || event.RightUp())
		ReleaseMouse();*/

	event.Skip();
}

void CViewWindow::OnContextMenu(wxContextMenuEvent& event)
{
	if(m_bIsMoving)
	{
		m_bIsMoving = false;
		return;
	}

	wxPoint pos = ScreenToClient(event.GetPosition());

	// Display context menu of object
	//PopupMenu(g_editormainframe->m_objectmenu, pos.x, pos.y);
}

void CViewWindow::OnFocus(wxFocusEvent& event)
{

}

void CViewWindow::ChangeView(int nNewView)
{
	m_viewid = nNewView;
	GetActiveView()->SetWindow(this);
}

void CViewWindow::ProcessKeyboardDownEvents(wxKeyEvent& event)
{
	if(GetForegroundWindow() != g_editormainframe->GetMaterials()->GetHWND())
		SetFocus();

	if(g_pSelectionTools[g_editormainframe->GetToolType()])
		g_pSelectionTools[g_editormainframe->GetToolType()]->OnKey(event, true, GetActiveView());
}

void CViewWindow::ProcessKeyboardUpEvents(wxKeyEvent& event)
{
	if(GetForegroundWindow() != g_editormainframe->GetMaterials()->GetHWND())
		SetFocus();

	if(g_pSelectionTools[g_editormainframe->GetToolType()])
		g_pSelectionTools[g_editormainframe->GetToolType()]->OnKey(event, false, GetActiveView());
}

CEditorViewRender* CViewWindow::GetActiveView()
{
	return &g_views[m_viewid];
}

typedef void (*LIGHTPARAMS)(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);

void ComputePointLightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_CUBEMAP;

	pView->SetOrigin(pLight->position);
	pView->SetAngles(vec3_zero);
	pView->SetFOV(90);
}

void ComputeSpotlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(pLight->fovWH.z);

	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 spot_proj = perspectiveMatrixY(DEG2RAD(pLight->fovWH.z), pLight->fovWH.x,pLight->fovWH.y, 1, pLight->radius.x);
		Matrix4x4 spot_view = rotateZXY4(DEG2RAD(-pLight->angles.x),DEG2RAD(-pLight->angles.y),DEG2RAD(-pLight->angles.z));
		pLight->lightRotation = !spot_view.getRotationComponent();
		spot_view.translate(-pLight->position);

		pLight->lightWVP = spot_proj*spot_view;
	}

	pLight->nFlags |= LFLAG_MATRIXSET;
}

void ComputeSunlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_ORTHOGONAL;
	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(90);

	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 sun_proj = orthoMatrix(-pLight->fovWH.x,pLight->fovWH.x,-pLight->fovWH.y,pLight->fovWH.y, pLight->radius.x, pLight->radius.y);
		Matrix4x4 sun_view = rotateZXY4(DEG2RAD(-pLight->angles.x),DEG2RAD(-pLight->angles.y),DEG2RAD(-pLight->angles.z));
		sun_view.translate(-pLight->position);
		pLight->lightWVP = sun_proj*sun_view;
	}

	
	pLight->nFlags |= LFLAG_MATRIXSET;
}

static LIGHTPARAMS pParamFuncs[DLT_COUNT] =
{
	ComputePointLightParams,
	ComputeSpotlightParams,
	ComputeSunlightParams
};

void ComputeLightViewsAndFlags(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_NO_MATERIALS;
	pParamFuncs[pLight->nType](pView,pLight,nDrawFlags);
}

void DrawViewInfo(CViewWindow* pWindow)
{
	wxPaintDC dc(pWindow);
	//dc.SetPen(*wxWHITE_PEN);
	//dc.SetBrush(*wxWHITE_BRUSH);
	dc.SetTextBackground(*wxBLACK);
	dc.SetTextForeground(*wxWHITE);

	switch(pWindow->GetActiveView()->GetCameraMode())
	{
		case CPM_PERSPECTIVE:
		{
			dc.DrawText( "Perspective", 5, 5 );
			break;
		}
		case CPM_TOP:
		{
			dc.DrawText( "Top X-Z", 5, 5 );
			break;
		}
		case CPM_RIGHT:
		{
			dc.DrawText( "Right Y-Z", 5, 5 );
			break;
		}
		case CPM_FRONT:
		{
			dc.DrawText( "Front X-Y", 5, 5 );
			break;
		}
	}
}

void CViewWindow::OnPaint(wxPaintEvent& event)
{

}

void CViewWindow::Redraw()
{
	
	if(!materials)
		return;

	if(GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE)
		ses->Update();

	if(!m_mustupdate)
	{
		//Refresh();
		//DrawViewInfo(this);
		return;
	}

	if(!IsShown())
		return;

	int w, h;
	GetSize(&w, &h);

	// don't draw bogus windows, or editor will crash or heap will be corrupted
	if( w == 0 || h == 0 )
		return;

	// compute time since last frame
	g_realtime = Platform_GetCurrentTime();
	g_frametime = g_realtime - g_oldrealtime;
	g_oldrealtime = g_realtime;

	// update material system and proxies
	materials->Update(g_frametime);

	m_mustupdate = false;

	g_pShaderAPI->SetViewport( 0, 0, w, h );

	materials->GetConfiguration().wireframeMode = ( GetActiveView()->GetRenderMode() == VRM_WIREFRAME );

	materials->SetAmbientColor(color4_white);

	if(materials->BeginFrame())
	{
		GetActiveView()->Set2DDimensions(w,h);

		FogInfo_t fog;
		materials->GetFogInfo(fog);

		fog.viewPos = GetActiveView()->GetView()->GetOrigin();

		if(GetActiveView()->GetCameraMode() == CPM_PERSPECTIVE && g_editormainframe->IsPreviewEnabled())
		{
			bool bFogEnabled = false;

			for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
			{
				if(g_pLevel->GetEditable(i)->GetType() == EDITABLE_ENTITY)
				{
					CEditableEntity* pEnt = (CEditableEntity*)g_pLevel->GetEditable(i);

					if(!stricmp(pEnt->GetClassname(), "env_fog"))
					{
						char* pColorKey = pEnt->GetKeyValue("_color");
						char* pFarKey = pEnt->GetKeyValue("far");
						char* pNearKey = pEnt->GetKeyValue("near");

						if(pColorKey)
							fog.fogColor = UTIL_StringToColor3(pColorKey);
						else
							fog.fogColor = ColorRGB(0.25f,0.25f,0.25f);

						fog.enableFog = true;
						fog.fogdensity = 1.0f;

						bFogEnabled = true;

						if(pFarKey)
							fog.fogfar = atof(pFarKey);
						else
							fog.fogfar = 14250;

						if(pNearKey)
							fog.fognear = atof(pNearKey);
						else
							fog.fognear = -2750;

						g_pShaderAPI->Clear(true,true,false, ColorRGBA(fog.fogColor, 1));
					}
				}
			}

			materials->SetFogInfo(fog);

			if(!bFogEnabled)
				g_pShaderAPI->Clear(true,true,false, ColorRGBA(g_editorCfg->background_color, 1));
		}
		else
		{
			fog.enableFog = false;

			materials->SetFogInfo(fog);

			g_pShaderAPI->Clear(true,true,false, ColorRGBA(g_editorCfg->background_color, 1));
		}

		g_pShaderAPI->SetupFog(&fog);

		
		switch(GetActiveView()->GetCameraMode())
		{
			case CPM_PERSPECTIVE:
				debugoverlay->Text(color4_white, "Perspective\n");
				break;
			case CPM_TOP:
				debugoverlay->Text(color4_white, "X-Z Top\n");
				break;
			case CPM_RIGHT:
				debugoverlay->Text(color4_white, "Z-Y Right\n");
				break;
			case CPM_FRONT:
				debugoverlay->Text(color4_white, "Y-Z Front\n");
				break;
		}

		if(g_editormainframe->IsSnapToVisibleGridEnabled() && GetActiveView()->GetCameraMode() != CPM_PERSPECTIVE)
			debugoverlay->Text(color4_white, "Visible grid size: %d", GetActiveView()->GetVisibleGridSizeBy(g_gridsize));

		GetActiveView()->DrawEditorThings();

#ifdef VR_TEST
		if(m_viewid == 0)
		{
			viewrenderer->SetViewportSize(w,h);

			sceneinfo_t originalSceneInfo = viewrenderer->GetSceneInfo();
			originalSceneInfo.m_cAmbientColor = ColorRGB(0.12,0.12,0.12);

			viewrenderer->SetSceneInfo(originalSceneInfo);
			viewrenderer->SetView(GetActiveView()->GetView());

			// Draw main view in VDM_AMBIENT
			viewrenderer->SetDrawMode(VDM_AMBIENT);
			viewrenderer->DrawRenderList(g_pLevel, VR_FLAG_NO_TRANSLUCENT);

			materials->SetAmbientColor(ColorRGBA(originalSceneInfo.m_cAmbientColor, 1.0f));

			// render ambient
			viewrenderer->DrawDeferredAmbient();

			viewrenderer->DrawDebug();

			for(int i = 0; i < viewrenderer->GetLightCount(); i++)
			{
				dlight_t* pLight = viewrenderer->GetLight(i);
			
				bool bDoShadows = !(pLight->nFlags & LFLAG_NOSHADOWS) && materials->GetConfiguration().enableShadows;

				// setup light view for shadow mapping
				CViewParams lightParams;

				int nDrawFlags = 0;

				// compute view, and rendering flags for each light
				ComputeLightViewsAndFlags(&lightParams, pLight, nDrawFlags);

				// check light for visibility
				if(!viewrenderer->GetViewFrustum()->IsSphereInside(pLight->position, pLight->radius.x))
					continue;

				// setup this light
				materials->SetLight(pLight);

				if(bDoShadows)
				{
					viewrenderer->SetDrawMode(VDM_SHADOW);

					sceneinfo_t lightSceneInfo = originalSceneInfo;
					lightSceneInfo.m_fZNear = 1.0f;
					lightSceneInfo.m_fZFar = pLight->radius.x;

					viewrenderer->SetSceneInfo(lightSceneInfo);
					viewrenderer->SetView(&lightParams);

					//viewrenderer->DrawWorld( nDrawFlags | VR_FLAG_NO_TRANSLUCENT );					// draw world without translucent objects
					viewrenderer->DrawRenderList(g_pLevel, nDrawFlags | VR_FLAG_NO_TRANSLUCENT);	// draw render list
				}

				viewrenderer->SetViewportSize(w,h);

				// restore view
				viewrenderer->SetSceneInfo(originalSceneInfo);
				viewrenderer->SetView(GetActiveView()->GetView());

				viewrenderer->SetDrawMode(VDM_LIGHTING);
			
				viewrenderer->DrawDeferredCurrentLighting(bDoShadows);
			}

			viewrenderer->UpdateLights(1.0f);
		}
		else
#endif // VR_TEST
		{
			GetActiveView()->DrawRenderList( g_pLevel );
		}

		if(g_pSelectionTools[g_editormainframe->GetToolType()])
			g_pSelectionTools[g_editormainframe->GetToolType()]->Render3D(GetActiveView());

		Matrix4x4 proj,view;
		GetActiveView()->GetViewProjection(view, proj);

		g_pShaderAPI->ResetCounters();

		if(g_pSelectionTools[g_editormainframe->GetToolType()])
			g_pSelectionTools[g_editormainframe->GetToolType()]->Render2D(GetActiveView());

		debugoverlay->Draw(proj, view, w, h);

		// draw little helper axis at left bottom

		g_pShaderAPI->SetViewport(0, h-80, 80, 80);

		materials->SetupOrtho(-40.0f, 40.0f, -40.0f, 40.0f, -40.0f, 40.0f);

		materials->SetMatrix( MATRIXMODE_VIEW, view.getRotationComponent());
		materials->SetMatrix( MATRIXMODE_WORLD, identity4() );

		Vertex3D_t vCenter(vec3_zero, vec2_zero, ColorRGBA(0,0,0,1));

		Vertex3D_t vX(Vector3D(25,0,0), vec2_zero, ColorRGBA(1,0,0,1));
		Vertex3D_t vY(Vector3D(0,25,0), vec2_zero, ColorRGBA(0,1,0,1));
		Vertex3D_t vZ(Vector3D(0,0,25), vec2_zero, ColorRGBA(0,0,1,1));

		Vertex3D_t vLines[6] = 
		{
			vCenter, vX,
			vCenter, vY,
			vCenter, vZ,
		};

		char* xyzText[3] = 
		{
			"x",
			"y",
			"z"
		};
		
		Vector3D pos[3] =
		{
			Vector3D(25,0,0),
			Vector3D(0,25,0),
			Vector3D(0,0,25)
		};

		materials->DrawPrimitivesFFP(PRIM_LINES, vLines, elementsOf(vLines), NULL);

		proj = orthoMatrix(-40.0f, 40.0f, -40.0f, 40.0f, -40.0f, 40.0f);
		materials->GetMatrix(MATRIXMODE_VIEW, view);

		materials->Setup2D(80,80);

		for(int i = 0; i < 3; i++)
		{
			if(fabs(dot(view.rows[2].xyz(), normalize(pos[i]))) > 0.75)
				continue;

			Vector2D screen(0);

			bool beh = PointToScreen( pos[i], screen, identity4()*view*proj, Vector2D(80, 80) );

			if(!beh)
			{
				debugoverlay->GetFont()->DrawSetColor(ColorRGBA(1,1,1,1));
				debugoverlay->GetFont()->DrawText(xyzText[i], screen.x, screen.y, 8, 8);
			}
		}
		
		// restore
		g_pShaderAPI->SetViewport( 0, 0, w, h );

		materials->EndFrame((HWND)GetHWND());

		//DrawViewInfo(this);
	}
}
