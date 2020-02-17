//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor camera
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORCAMERA_H
#define EDITORCAMERA_H

//#include "ViewParams.h"
//#include "RenderList.h"

#include "ViewRenderer.h"

enum CameraProjectionMode_e
{
	CPM_PERSPECTIVE = 0,
	CPM_TOP,
	CPM_RIGHT,
	CPM_FRONT,
};

enum ViewRenderMode_e
{
	VRM_WIREFRAME = 0,	// wireframe
	VRM_SMOOTHSHADED,	// smooth shaded mode
	VRM_TEXTURED,		// textured mode
	VRM_LIGHTING		// lighting preview
};

enum CameraControlMode_e
{
	CCM_DEFAULT = 0,
	CCM_FPS,
};

class CDCRender2D;
class CViewWindow;
class wxDC;

class CEditorViewRender : public CViewRenderer
{
public:
	CEditorViewRender()
	{
		m_rendermode = VRM_TEXTURED;
		m_cameramode = CPM_PERSPECTIVE;
	}

	CViewParams*			GetView()							{return &m_viewparameters;}

	ViewRenderMode_e		GetRenderMode();
	void					SetRenderMode(ViewRenderMode_e mode);

	CameraProjectionMode_e	GetCameraMode();
	void					SetCameraMode(CameraProjectionMode_e mode);

	void					Set2DDimensions(float x, float y);
	Vector2D				Get2DDimensions() {return m_screendims;}


	void					DrawRenderList(CBasicRenderList* list);
	void					DrawRenderListAdv(CBasicRenderList* list);
	void					DrawEditorThings();

	Vector3D				GetAbsoluteCameraPosition();
	void					GetViewProjection(Matrix4x4 &view, Matrix4x4 &proj);

	void					DrawSelectionBBox(Vector3D &mins, Vector3D &maxs, ColorRGB &color, bool drawHandles);

	void					SetupRenderMatrices();

	void					SetWindow(CViewWindow *pWindow);
	CViewWindow*			GetWindow();

	int						GetVisibleGridSizeBy(int nGridSize);

protected:

	void					DrawGridCoordinates();

	CViewParams				m_viewparameters;

	ViewRenderMode_e		m_rendermode;
	CameraProjectionMode_e	m_cameramode;

	Vector2D				m_screendims;

	int						m_nDim1;
	int						m_nDim2;

	CViewWindow*			m_pWindow;
	
};

#endif // EDITORCAMERA_H