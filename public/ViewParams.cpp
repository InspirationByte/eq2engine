//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: View parameters.
// Used by GameDLL and Engine to determine renderable camera position, rotation and FOV
//////////////////////////////////////////////////////////////////////////////////

#include "ViewParams.h"

const float BASE_LOD_FOV	=	(1.0f / 39.0f);		// the good value

CViewParams::CViewParams() 
{
	m_vecOrigin = vec3_zero;
	m_vecAngles = vec3_zero;
	m_fFOV = 90;

	//m_orthoScale = 1.0f;
	//m_cubemapFace = 0;
}

CViewParams::CViewParams(const Vector3D &origin,const Vector3D &angles,float fFov)
{
	m_vecOrigin = origin;
	m_vecAngles = angles;
	m_fFOV = fFov;

	//m_orthoScale = 1.0f;
	//m_cubemapFace = 0;
}

void CViewParams::SetOrigin(const Vector3D &origin)
{
	m_vecOrigin = origin;
}
void CViewParams::SetAngles(const Vector3D &angles)
{
	m_vecAngles = angles;
}
void CViewParams::SetFOV(float fFov)
{
	m_fFOV = fFov;
}

const Vector3D& CViewParams::GetOrigin() const
{
	return m_vecOrigin;
}

const Vector3D& CViewParams::GetAngles() const
{
	return m_vecAngles;
}

float CViewParams::GetFOV() const
{
	return m_fFOV;
}

float CViewParams::GetLODScaledDistFrom( const Vector3D& position )
{
	float fLodDistScale = m_fFOV * BASE_LOD_FOV;
	fLodDistScale = min(fLodDistScale, 1.0f);

	return fLodDistScale * length(position-m_vecOrigin);
}

void CViewParams::GetMatrices(Matrix4x4& proj, Matrix4x4& view, float width, float height, float zNear, float zFar, bool orthographic)
{
	Vector3D vRadianRotation = VDEG2RAD(m_vecAngles);

	if(orthographic)
		proj = orthoMatrixR(width*-m_fFOV, width*m_fFOV, height*-m_fFOV, height*m_fFOV, zNear, zFar);
	else
		proj = perspectiveMatrixY(DEG2RAD(m_fFOV), width, height, zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}

void CViewParams::GetMatricesOrtho(Matrix4x4& proj, Matrix4x4& view, Rectangle_t rect, float zNear, float zFar)
{
	Vector3D vRadianRotation = VDEG2RAD(m_vecAngles);

	proj = orthoMatrixR(rect.vleftTop.x, rect.vrightBottom.x, rect.vleftTop.y, rect.vrightBottom.y , zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}

/*
void CViewParams::BuildViewMatrices(float fNear, float fFar, int width, int height, bool orthographic, bool cubemap)
{
	Vector3D radianAngles = VDEG2RAD(m_vecAngles);

	// this is negated matrix
	Matrix4x4 viewMatrix;

	if( cubemap )
	{
		m_matrices[MATRIXMODE_PROJECTION] = cubeProjectionMatrixD3D(fNear, fFar);
		viewMatrix = cubeViewMatrix( m_cubemapFace );
	}
	else
	{
		m_matrices[MATRIXMODE_PROJECTION]	= perspectiveMatrixY(DEG2RAD(m_fFOV), width, height, fNear, fFar);

		if(orthographic)
			m_matrices[MATRIXMODE_PROJECTION]	= orthoMatrixR(width*-m_orthoScale, width*m_orthoScale, height*-m_orthoScale, height*m_orthoScale, -fFar, fFar);

		viewMatrix = rotateZXY4(-radianAngles.x,-radianAngles.y,-radianAngles.z);
	}

	viewMatrix.translate( -m_vecOrigin );

	m_matrices[MATRIXMODE_VIEW]			= viewMatrix;
	m_matrices[MATRIXMODE_WORLD]		= identity4();

	// store the viewprojection matrix for some purposes
	m_viewprojection = m_matrices[MATRIXMODE_PROJECTION] * m_matrices[MATRIXMODE_VIEW];
}

void CViewParams::SetupMatrices()
{
	materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
	materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_PROJECTION]);
}*/