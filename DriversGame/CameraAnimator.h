//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate camera animator
//////////////////////////////////////////////////////////////////////////////////

#ifndef CAMERAANIMATOR_H
#define CAMERAANIMATOR_H

#include "math/DkMath.h"
#include "ViewParams.h"

enum ECameraMode
{
	CAM_MODE_OUTCAR = 0,
	CAM_MODE_INCAR,
	CAM_MODE_TRIPOD_ZOOM,
	CAM_MODE_TRIPOD_FIXEDZOOM,
	CAM_MODE_TRIPOD_STATIC,

	CAM_MODE_OUTCAR_FIXED,

	CAM_MODES,
};

struct carCameraConfig_t
{
	float						height;
	float						dist;
	float						fov;
	float						distInCar;
	float						heightInCar;
	float						widthInCar;
};

class CCameraAnimator
{
public:
	CCameraAnimator();
	virtual ~CCameraAnimator() {}

	void					SetCameraProps(const carCameraConfig_t& conf);

	void					SetFOV(float fFOV);

	void					SetDropPosition(const Vector3D& camPos);

	void					SetRotation(const Vector3D& camPos);

	void					CalmDown();

	void					Animate(	ECameraMode mode, int nButtons, 
										const Vector3D& targetOrigin, 
										const Matrix4x4& targetRotation, 
										const Vector3D& targetVelocity,
										float fDt,
										Vector3D& addRot);

	CViewParams&			GetCamera();

	ECameraMode				GetMode() const {return m_mode;}

protected:

	carCameraConfig_t		m_carConfig;
	float					m_cameraDistVar;
	float					m_cameraFOV;

	float					m_fLookAngle;

	float					m_fTempCamAngle;
	Vector3D				m_vecCameraVel;
	Vector3D				m_vecCameraVelDiff;

	Vector3D				m_vecCameraSpringVel;

	Vector3D				m_dropPos;
	Vector3D				m_rotation;

	CViewParams				m_viewParams;

	ECameraMode				m_mode;
};

extern CCameraAnimator*		g_pCameraAnimator;

#endif // CAMERAANIMATOR_H