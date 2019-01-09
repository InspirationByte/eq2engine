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
}

CViewParams::CViewParams(const Vector3D &origin,const Vector3D &angles,float fFov)
{
	m_vecOrigin = origin;
	m_vecAngles = angles;
	m_fFOV = fFov;
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

float CViewParams::GetLODScaledDistFrom( const Vector3D& position ) const
{
	float fLodDistScale = m_fFOV * BASE_LOD_FOV;
	fLodDistScale = min(fLodDistScale, 1.0f);

	return fLodDistScale * length(position-m_vecOrigin);
}

void CViewParams::GetMatrices(Matrix4x4& proj, Matrix4x4& view, float width, float height, float zNear, float zFar, bool orthographic) const
{
	Vector3D vRadianRotation = VDEG2RAD(m_vecAngles);

	if(orthographic)
		proj = orthoMatrix(width*-m_fFOV, width*m_fFOV, height*-m_fFOV, height*m_fFOV, zNear, zFar);
	else
		proj = perspectiveMatrixY(DEG2RAD(m_fFOV), width, height, zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}

void CViewParams::GetMatricesOrtho(Matrix4x4& proj, Matrix4x4& view, Rectangle_t rect, float zNear, float zFar) const
{
	Vector3D vRadianRotation = VDEG2RAD(m_vecAngles);

	proj = orthoMatrixR(rect.vleftTop.x, rect.vrightBottom.x, rect.vleftTop.y, rect.vrightBottom.y , zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}