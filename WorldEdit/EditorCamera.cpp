//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor camera
//////////////////////////////////////////////////////////////////////////////////

#include "EditorCamera.h"
#include "EditorHeader.h"
#include "IDebugOverlay.h"
#include "grid.h"
#include "math/math_util.h"
#include "SelectionEditor.h"
#include "DCRender2D.h"
#include "EditorMainFrame.h"
#include "IFont.h"

ViewRenderMode_e CEditorViewRender::GetRenderMode()
{
	return m_rendermode;
}

void CEditorViewRender::SetRenderMode(ViewRenderMode_e mode)
{
	m_rendermode = mode;
}


CameraProjectionMode_e CEditorViewRender::GetCameraMode()
{
	return m_cameramode;
}

void CEditorViewRender::SetCameraMode(CameraProjectionMode_e mode)
{
	m_cameramode = mode;

	switch(m_cameramode)
	{
	case CPM_PERSPECTIVE:
		m_nDim1 = 0;
		m_nDim2 = 0;
		break;
	case CPM_TOP:
		m_nDim1 = 0;
		m_nDim2 = 2;
		break;
	case CPM_RIGHT:
		m_nDim1 = 1;
		m_nDim2 = 2;
		break;
	case CPM_FRONT:
		m_nDim1 = 0;
		m_nDim2 = 1;
		break;
	}
}

void CEditorViewRender::DrawRenderList(CBasicRenderList* list)
{
	g_pShaderAPI->Reset();

	SetupRenderMatrices();

	drawlist_struct_t options;
	options.pViewRender = this;
	options.bWireframe = false;

	if(m_rendermode == VRM_WIREFRAME)
	{
		options.bWireframe = true;
		materials->SetAmbientColor(ColorRGBA(0.8f,0.8f,0.8f,1));
	}

	list->Update(0, &options);
	list->Render(VR_FLAG_NO_TRANSLUCENT, &options);

	if(m_rendermode != VRM_WIREFRAME && m_cameramode == CPM_PERSPECTIVE)
		list->Render(VR_FLAG_NO_OPAQUE, &options);
}

void CEditorViewRender::DrawRenderListAdv(CBasicRenderList* list)
{
	g_pShaderAPI->Reset();

	SetupRenderMatrices();

	drawlist_struct_t options;
	options.pViewRender = this;
	options.bWireframe = false;

	if(m_rendermode == VRM_WIREFRAME)
	{
		options.bWireframe = true;
		materials->SetAmbientColor(ColorRGBA(0.8f,0.8f,0.8f,1));
	}

	list->Update(0, &options);
	list->Render(VR_FLAG_NO_TRANSLUCENT, &options);

	if(m_rendermode != VRM_WIREFRAME && m_cameramode == CPM_PERSPECTIVE)
		list->Render(VR_FLAG_NO_OPAQUE, &options);
}

int	CEditorViewRender::GetVisibleGridSizeBy(int nGridSize)
{
	float gridSize = nGridSize;
		
	while(GetView()->GetFOV()/Get2DDimensions().y > gridSize*0.095f)
	{
		gridSize *= 2;
	}

	return gridSize;
}

void CEditorViewRender::DrawEditorThings()
{
	SetupRenderMatrices();

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(m_cameramode == CPM_PERSPECTIVE)
	{
		/*
		Matrix4x4 rotationMatrix = cubeViewMatrix(NEGATIVE_Y);
		materials->SetMatrix(MATRIXMODE_WORLD, rotationMatrix);

		DrawGrid(g_gridsize, ColorRGBA(0.5,0.5,0.5,1));
		*/
	}
	else
	{
		float gridSize = GetVisibleGridSizeBy(g_gridsize);
		float biggridSize = GetVisibleGridSizeBy(gridSize*8);
		
		Vector3D snapped_camera_pos = SnapVector(biggridSize, m_viewparameters.GetOrigin());
		Vector3D grid_offset;

		Matrix4x4 rotationMatrix;

		switch(m_cameramode)
		{
			case CPM_TOP:
				rotationMatrix = cubeViewMatrix(NEGATIVE_Y);
				break;
			case CPM_RIGHT:
				rotationMatrix = cubeViewMatrix(NEGATIVE_X);
				break;
			case CPM_FRONT:
				rotationMatrix = cubeViewMatrix(POSITIVE_Z);
				break;
			default:
				rotationMatrix = cubeViewMatrix(POSITIVE_Z);
		}

		grid_offset = SnapVector(gridSize, snapped_camera_pos);
		materials->SetMatrix(MATRIXMODE_WORLD, translate(grid_offset) * rotationMatrix);

		DrawGrid(gridSize, g_editorCfg->grid1_color, true);

		grid_offset = SnapVector(biggridSize, snapped_camera_pos);
		materials->SetMatrix(MATRIXMODE_WORLD, translate(grid_offset) * rotationMatrix);

		DrawGrid(biggridSize, g_editorCfg->grid2_color, true);

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		DrawWorldCenter();

		// draw camera position (target)
		debugoverlay->Line3D(Vector3D(-10,0,0) + m_viewparameters.GetOrigin(), Vector3D(10,0,0) + m_viewparameters.GetOrigin(), ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
		debugoverlay->Line3D(Vector3D(0,-10,0) + m_viewparameters.GetOrigin(), Vector3D(0,10,0) + m_viewparameters.GetOrigin(), ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
		debugoverlay->Line3D(Vector3D(0,0,-10) + m_viewparameters.GetOrigin(), Vector3D(0,0,10) + m_viewparameters.GetOrigin(), ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
	}

	// draw axe
	debugoverlay->Line3D(vec3_zero, Vector3D(100,0,0), ColorRGBA(0,0,0,1), ColorRGBA(1,0,0,1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0,100,0), ColorRGBA(0,0,0,1), ColorRGBA(0,1,0,1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0,0,100), ColorRGBA(0,0,0,1), ColorRGBA(0,0,1,1));

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());
}

void CEditorViewRender::DrawGridCoordinates()
{
	// draw coordinate text if needed
	if (true)
	{
		//glColor3fv(vector3_to_array(g_xywindow_globals.color_gridtext));
		Vector3D viewOrigin = GetAbsoluteCameraPosition();

		//float dist_scale = GetView()->GetFOV();

		/*

		float offx = viewOrigin[m_nDim2] + m_screendims.y - 6 / dist_scale, offy = viewOrigin[m_nDim1] - m_screendims.x + 1 / dist_scale;
		for (int x = xb - fmod(xb, stepx); x <= xe ; x += stepx)
		{
			glRasterPos2f (x, offx);
			sprintf (text, "%g", x);
			GlobalOpenGL().drawString(text);
		}
		for (int y = yb - fmod(yb, stepy); y <= ye ; y += stepy) {
			glRasterPos2f (offy, y);
			sprintf (text, "%g", y);
			GlobalOpenGL().drawString(text);
		}

		if (Active())
			glColor3fv(vector3_to_array(g_xywindow_globals.color_viewname));

		// we do this part (the old way) only if show_axis is disabled
		if (!g_xywindow_globals_private.show_axis) {
			glRasterPos2f ( m_vOrigin[nDim1] - w + 35 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale );

			GlobalOpenGL().drawString(ViewType_getTitle(m_viewType));
		}
		*/
	}

}

Vector3D CEditorViewRender::GetAbsoluteCameraPosition()
{
	/*
	if(m_cameramode == CPM_PERSPECTIVE)
	{
		Vector3D forward, right;
		AngleVectors(m_viewparameters.GetAngles(), &forward, &right);

		return m_viewparameters.GetOrigin() + forward*-m_viewparameters.GetFOV();
	}
	else
	{
	*/
		return m_viewparameters.GetOrigin();
	//}
}

void CEditorViewRender::SetWindow(CViewWindow *pWindow)
{
	m_pWindow = pWindow;
}

CViewWindow* CEditorViewRender::GetWindow()
{
	return m_pWindow;
}

#define CORNER_BOX_SIZE 1.0f // pixels

void CEditorViewRender::DrawSelectionBBox(Vector3D &mins, Vector3D &maxs, ColorRGB &color, bool drawHandles)
{
	if(m_cameramode != CPM_PERSPECTIVE)
	{
		Rectangle_t rect;
		SelectionRectangleFromBBox(this, mins, maxs, rect);

		Vertex2D_t verts[] = {MAKETEXRECT(rect.vleftTop.x,rect.vleftTop.y,rect.vrightBottom.x,rect.vrightBottom.y, 0)};

		materials->Setup2D(m_screendims.x,m_screendims.y);
		
		BlendStateParam_t params;
		params.alphaTest = false;
		params.alphaTestRef = 1.0f;
		params.blendEnable = false;
		params.srcFactor = BLENDFACTOR_ONE;
		params.dstFactor = BLENDFACTOR_ZERO;
		params.mask = COLORMASK_ALL;
		params.blendFunc = BLENDFUNC_ADD;

		// draw 2D bbox
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(color,1), &params);

		eqFontStyleParam_t fontParams;
		fontParams.styleFlag |= TEXT_STYLE_SHADOW;
		fontParams.textColor = ColorRGBA(color*2.0f, 1);

		if(drawHandles)
		{
			Rectangle_t window_rect(0.0f,0.0f,m_screendims.x,m_screendims.y);

			Vector2D handles[SH_COUNT];
			SelectionGetHandlePositions(rect, handles, window_rect, 8);

			Vertex2D_t center_verts[] = 
			{
				Vertex2D(handles[SH_CENTER] + Vector2D(-4), vec2_zero),
				Vertex2D(handles[SH_CENTER] + Vector2D(4), vec2_zero),

				Vertex2D(handles[SH_CENTER] + Vector2D(4,-4), vec2_zero),
				Vertex2D(handles[SH_CENTER] + Vector2D(-4,4), vec2_zero),
			};

			Vector2D wideTall = Vector2D(fabs(dot(SelectionGetHandleDirection(this, SH_RIGHT), maxs-mins)), fabs(dot(SelectionGetHandleDirection(this, SH_BOTTOM), maxs-mins)));

			// draw center handle as cross
			materials->DrawPrimitives2DFFP(PRIM_LINES, center_verts, elementsOf(center_verts), NULL, color4_white, &params);

			for(int i = 1; i < SH_COUNT; i++)
			{
				Vertex2D_t handle_verts[] = {MAKETEXQUAD(handles[i].x-3,handles[i].y-3,handles[i].x+3,handles[i].y+3, 0)};
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, handle_verts, elementsOf(handle_verts), NULL, color4_white, &params);

				// draw only single point
				Vertex2D_t pointR[] = {MAKETEXRECT(handles[i].x-3,handles[i].y-3,handles[i].x+3,handles[i].y+3, 0)};
				materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
			}

			debugoverlay->GetFont()->RenderText(varargs("X: %g",wideTall.x), Vector2D(handles[SH_BOTTOM].x+5, handles[SH_BOTTOM].y), fontParams);
			debugoverlay->GetFont()->RenderText(varargs("Y: %g",wideTall.y), Vector2D(handles[SH_RIGHT].x, handles[SH_RIGHT].y+5), fontParams);
		}
		else
		{
			Rectangle_t window_rect(0.0f,0.0f,m_screendims.x,m_screendims.y);

			Vector2D handles[SH_COUNT];
			SelectionGetHandlePositions(rect, handles, window_rect);

			Vertex2D_t center_verts[] = 
			{
				Vertex2D(handles[SH_BOTTOMLEFT] + Vector2D(-8), vec2_zero),
				Vertex2D(handles[SH_BOTTOMLEFT] + Vector2D(8), vec2_zero),

				Vertex2D(handles[SH_BOTTOMLEFT] + Vector2D(8,-8), vec2_zero),
				Vertex2D(handles[SH_BOTTOMLEFT] + Vector2D(-8,8), vec2_zero),
			};

			Vector2D wideTall = Vector2D(fabs(dot(SelectionGetHandleDirection(this, SH_RIGHT), maxs-mins)), fabs(dot(SelectionGetHandleDirection(this, SH_BOTTOM), maxs-mins)));

			// draw center handle as cross
			materials->DrawPrimitives2DFFP(PRIM_LINES, center_verts, elementsOf(center_verts), NULL, ColorRGBA(color,1), &params);

			debugoverlay->GetFont()->RenderText(varargs("X: %g",wideTall.x), Vector2D(handles[SH_BOTTOM].x+5, handles[SH_BOTTOM].y), fontParams);
			debugoverlay->GetFont()->RenderText(varargs("Y: %g",wideTall.y), Vector2D(handles[SH_RIGHT].x, handles[SH_RIGHT].y+5), fontParams);
		}
	}
	else
	{
		SetupRenderMatrices();
		debugoverlay->Box3D(mins, maxs, ColorRGBA(color,1));
	}
}

void CEditorViewRender::SetupRenderMatrices()
{
	Matrix4x4 view, proj;
	GetViewProjection(view,proj);

	materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
	materials->SetMatrix(MATRIXMODE_VIEW, view);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());
}

void CEditorViewRender::GetViewProjection(Matrix4x4 &view, Matrix4x4 &proj)
{
	if(m_cameramode == CPM_PERSPECTIVE)
	{
		// perspective mode
		proj = perspectiveMatrixY(DEG2RAD(65), m_screendims.x, m_screendims.y, 1, 15000);
	}
	else
	{
		// find aspect
		float aspect = m_screendims.x / m_screendims.y;

		// get camera distance
		float distance = m_viewparameters.GetFOV();

		distance = clamp(distance,10.0f,MAX_COORD_UNITS);

		// make x and y size
		float half_w = aspect*distance;
		float half_h = distance;

		proj = orthoMatrixR(-half_w,half_w, -half_h,half_h, -MAX_COORD_UNITS/2,MAX_COORD_UNITS/2);
	}

	if(m_cameramode == CPM_PERSPECTIVE)
	{
		//Vector3D forward, right;
		//AngleVectors(m_viewparameters.GetAngles(), &forward, &right);

		Matrix4x4 viewMatrix = rotateZXY4(DEG2RAD(-m_viewparameters.GetAngles().x),DEG2RAD(-m_viewparameters.GetAngles().y),DEG2RAD(-m_viewparameters.GetAngles().z));
		//viewMatrix.translate(-(m_viewparameters.GetOrigin() + forward*-m_viewparameters.GetFOV()));
		viewMatrix.translate(-m_viewparameters.GetOrigin());
		
		view = viewMatrix;
	}
	else
	{
		Matrix4x4 viewMatrix;

		switch(m_cameramode)
		{
			case CPM_TOP:
				viewMatrix = cubeViewMatrix(NEGATIVE_Y);
				break;
			case CPM_RIGHT:
				viewMatrix = cubeViewMatrix(NEGATIVE_X);
				break;
			case CPM_FRONT:
				viewMatrix = cubeViewMatrix(POSITIVE_Z);
				break;
			default:
				viewMatrix = cubeViewMatrix(POSITIVE_Z);
		}

		viewMatrix.translate(-m_viewparameters.GetOrigin());
		
		view = viewMatrix;
	}
}

void CEditorViewRender::Set2DDimensions(float x, float y)
{
	m_screendims = Vector2D(x,y);
}
