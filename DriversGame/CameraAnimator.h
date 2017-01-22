//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

class CCar;

class CCameraAnimator
{
public:
	CCameraAnimator();
	virtual ~CCameraAnimator() {}

	void					SetCameraProps(const carCameraConfig_t& conf);

	void					SetFOV(float fFOV);
	void					SetOrigin(const Vector3D& camPos);
	void					SetAngles(const Vector3D& angles);

	const Vector3D&			GetOrigin() const;
	const Vector3D&			GetAngles() const;
	float					GetFOV() const;

	void					Reset();

	void					ViewShake(float fMagnutude, float fTime);

	const CViewParams&		GetComputedView() const;

	int						GetRealMode() const				{return m_realMode;}
	int						GetMode() const					{return m_mode;}
	void					SetMode( int newMode )			{m_mode = (ECameraMode)newMode;}

	void					Update( float fDt, int nButtons, CCar* target );

	void					SetScripted( bool enable )		{ m_scriptControl = enable; }
	bool					IsScripted() const				{ return m_scriptControl; }

	void					L_Update( float fDt, CCar* target );


protected:

	Vector3D				ShakeView( float fDt );

	void					Animate(	ECameraMode mode, int nButtons,
										const Vector3D& targetOrigin,
										const Quaternion& targetRotation,
										const Vector3D& targetVelocity,
										float fDt,
										const Vector3D& addRot);

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

	float					m_shakeDecayCurTime;
	float					m_shakeMagnitude;

	float					m_targetForwardSpeedModifier;

	CViewParams				m_computedView;

	ECameraMode				m_mode;
	ECameraMode				m_realMode;
	int						m_oldBtns;

	bool					m_scriptControl;
};

extern CCameraAnimator*		g_pCameraAnimator;

#endif // CAMERAANIMATOR_H
