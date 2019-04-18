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

	CAM_MODE_OUTCAR_WHEEL_0,	// front left
	CAM_MODE_OUTCAR_WHEEL_1,	// front right
	CAM_MODE_OUTCAR_WHEEL_2,	// rear left
	CAM_MODE_OUTCAR_WHEEL_3,	// rear right

	CAM_MODES,
};

struct cameraConfig_t
{
	float					height;
	float					dist;
	float					fov;
	float					distInCar;
	float					heightInCar;
	float					widthInCar;
};

class CCar;
class CEqRigidBody;
class CGameObject;

class CCameraAnimator
{
public:
	CCameraAnimator();
	virtual ~CCameraAnimator() {}

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

	void					SetFreeLook(bool enable, const Vector3D& angles);

	void					Update( float fDt, int nButtons, CGameObject* target);

	void					SetScripted( bool enable )		{ m_scriptControl = enable; }
	bool					IsScripted() const				{ return m_scriptControl; }

	void					L_Update( float fDt, CGameObject* target );


protected:

	Vector3D				ShakeView( float fDt );

	void					AnimateForObject(ECameraMode mode, int nButtons, float fDt, CGameObject* target, struct eqPhysCollisionFilter& collFilter);

	void					Animate(	ECameraMode mode, int nButtons,
										const Vector3D& targetOrigin,
										const Quaternion& targetRotation,
										const Vector3D& targetVelocity,
										float fDt,
										struct eqPhysCollisionFilter& collFilter);

	cameraConfig_t			m_camConfig;
	float					m_cameraDistVar;
	float					m_cameraFOV;

	Vector3D				m_interpLookAngle;

	float					m_interpCamAngle;
	Vector3D				m_vecCameraVel;
	Vector3D				m_vecCameraVelDiff;

	Vector3D				m_vecCameraSpringVel;

	Vector3D				m_dropPos;
	Vector3D				m_rotation;

	float					m_shakeDecayCurTime;
	float					m_shakeMagnitude;

	Vector3D				m_freelookAngles;
	bool					m_freelook;

	CViewParams				m_computedView;

	ECameraMode				m_mode;
	ECameraMode				m_realMode;
	int						m_oldBtns;

	bool					m_scriptControl;
};

extern CCameraAnimator*		g_pCameraAnimator;

#endif // CAMERAANIMATOR_H
