//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: View parameters.
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "math/Utility.h"
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

void CViewParams::SetZNear(float fNear)
{
	m_fZNear = fNear;
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

float CViewParams::GetZNear() const
{
	return m_fZNear;
}

float CViewParams::GetLODScaledDistFrom( const Vector3D& position ) const
{
	float fLodDistScale = m_fFOV * BASE_LOD_FOV;
	fLodDistScale = min(fLodDistScale, 1.0f);

	return fLodDistScale * length(position-m_vecOrigin);
}

void CViewParams::GetMatrices(Matrix4x4& proj, Matrix4x4& view, float width, float height, float zNear, float zFar, bool orthographic) const
{
	Vector3D vRadianRotation = DEG2RAD(m_vecAngles);

	if(orthographic)
		proj = orthoMatrixR(width*-m_fFOV, width*m_fFOV, height*-m_fFOV, height*m_fFOV, zNear, zFar);
	else
		proj = perspectiveMatrixY(DEG2RAD(m_fFOV), width, height, zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}

void CViewParams::GetMatricesOrtho(Matrix4x4& proj, Matrix4x4& view, AARectangle rect, float zNear, float zFar) const
{
	Vector3D vRadianRotation = DEG2RAD(m_vecAngles);

	proj = orthoMatrixR(rect.vleftTop.x, rect.vrightBottom.x, rect.vleftTop.y, rect.vrightBottom.y , zNear, zFar);

	view = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);

	view.translate(-m_vecOrigin);
}

void CViewParams::Interpolate(const CViewParams& from, const CViewParams& to, float factor, CViewParams& out)
{
	const Vector3D anglesA = from.GetAngles();
	const Vector3D anglesB = to.GetAngles();
	const Vector3D vRadianRotationA = DEG2RAD(anglesA);
	const Vector3D vRadianRotationB = DEG2RAD(anglesB);

	Quaternion qA(vRadianRotationA.x, vRadianRotationA.y, vRadianRotationA.z);
	Quaternion qB(vRadianRotationB.x, vRadianRotationB.y, vRadianRotationB.z);
	qA.normalize();
	qB.normalize();
	Quaternion qR = slerp(qA, qB, factor);
	qR.normalize();

	Vector3D qRotationEulers;
	quaternionToEulers(qR, QuatRot_yzx, qRotationEulers);
	qRotationEulers = Vector3D(qRotationEulers.x, qRotationEulers.z, qRotationEulers.y);

	out.SetOrigin(lerp(from.GetOrigin(), to.GetOrigin(), factor));
	out.SetAngles(RAD2DEG(qRotationEulers));
	out.SetFOV(lerp(from.GetFOV(), to.GetFOV(), factor));
}