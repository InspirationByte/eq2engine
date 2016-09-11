//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate camera animator
//////////////////////////////////////////////////////////////////////////////////


//
// TODO:	rework this code, it looks like piece of shit
//			add director features HERE
//

#include "CameraAnimator.h"
#include "ConVar.h"
#include "math/math_util.h"
#include "car.h"

#define DEFAULT_CAMERA_FOV			(52.0f)

#define CAM_TURN_SPEED				3.4f
#define CAM_LOOK_TURN_SPEED			15.0f

#define CAM_HEIGHT_TRACE			(-0.3f)

#define ZOOM_START_DIST				8.0f
#define ZOOM_END_DIST				200.0f

#define MAX_CAR_DIST				250.0f

#define START_FOV					40.0f
#define END_FOV						10.0f

ConVar cam_velocity_accel("cam_velocity_accel", "0.5");
ConVar cam_velocity_mindiff("cam_velocity_mindiff", "0.8");

#define CAR_ACCEL_VEL_FACTOR	cam_velocity_accel.GetFloat() //0.15f
#define CAR_DECEL_VEL_MINDIFF		cam_velocity_mindiff.GetFloat() //5.0f

ConVar cam_velocity_springconst("cam_velocity_springconst", "15");
ConVar cam_velocity_springdamp("cam_velocity_springdamp", "8");

ConVar cam_velocityeffects("cam_velocityeffects", "1", "Enable velocity effects", CV_ARCHIVE);

ConVar cam_velocity_forwardmod("cam_velocity_forwardmod", "1.0");
ConVar cam_velocity_sidemod("cam_velocity_sidemod", "0.0");
ConVar cam_velocity_upmod("cam_velocity_upmod", "1.0");

//---------------------------------------------------------------------------------------------------------------

CCameraAnimator::CCameraAnimator() :
	m_vecCameraVel(0.0f),
	m_vecCameraVelDiff(0.0f),
	m_vecCameraSpringVel(0.0f),
	m_cameraDistVar(100.0f),
	m_fLookAngle(0.0f),
	m_fTempCamAngle(0.0f),
	m_dropPos(0.0f),
	m_rotation(0.0f),
	m_cameraFOV(DEFAULT_CAMERA_FOV),
	m_carConfig(NULL)
{

}

void CCameraAnimator::SetCameraProps( const carCameraConfig_t& conf )
{
	m_carConfig = (carCameraConfig_t*)&conf;
}

void CCameraAnimator::SetFOV(float fFOV)
{
	m_cameraFOV = fFOV;
}

void CCameraAnimator::SetOrigin(const Vector3D& camPos)
{
	m_dropPos = camPos;
}

void CCameraAnimator::SetAngles(const Vector3D& camRot)
{
	m_rotation = camRot;
}

const Vector3D& CCameraAnimator::GetOrigin() const
{
	return m_dropPos;
}

const Vector3D&	CCameraAnimator::GetAngles() const
{
	return m_rotation;
}

float CCameraAnimator::GetFOV() const 
{
	return m_cameraFOV;
}

void CCameraAnimator::Reset()
{
	m_vecCameraVel = vec3_zero;
	m_vecCameraVelDiff = vec3_zero;
	m_vecCameraSpringVel = vec3_zero;
	m_fLookAngle = 0.0f;
	m_fTempCamAngle = 0.0f;
	m_carConfig = NULL;

	if(m_mode > CAM_MODE_INCAR)
		m_mode = CAM_MODE_OUTCAR;
}

void CCameraAnimator::Update( float fDt, int nButtons, CCar* target )
{
	// Check camera switch buttons
	if(	(nButtons & IN_CHANGECAM) && !(m_oldBtns & IN_CHANGECAM) )
	{
		int newMode = (int)m_mode + 1;

		if(newMode == CAM_MODE_TRIPOD_ZOOM)
		{
			// compute drop  position
			Vector3D dropPos = target->GetOrigin() + Vector3D(0,target->m_conf->m_body_size.y,0) - target->GetForwardVector()*target->m_conf->m_body_size.z*1.1f;

			SetOrigin( dropPos );
		}

		// rollin
		if( newMode > CAM_MODE_TRIPOD_ZOOM )
			newMode = CAM_MODE_OUTCAR;

		m_mode = (ECameraMode)newMode;
	}

	// TODO: other control for addRot

	if( target )
	{
		ECameraMode camMode = m_mode;

		if( target->IsInWater() && camMode == CAM_MODE_INCAR )
			camMode = CAM_MODE_OUTCAR;

		SetCameraProps( target->m_conf->m_cameraConf );

		Animate(camMode, nButtons, target->GetOrigin(), target->GetOrientation(), target->GetVelocity(), fDt, vec3_zero);
	}
	else
		Animate(m_mode, nButtons, vec3_zero, Quaternion(), vec3_zero, fDt, vec3_zero);

	m_oldBtns = nButtons;
}

void CCameraAnimator::L_Update( float fDt, CCar* target )
{
	if( target )
	{
		ECameraMode camMode = m_mode;

		if( target->IsInWater() && camMode == CAM_MODE_INCAR )
			camMode = CAM_MODE_OUTCAR;

		SetCameraProps( target->m_conf->m_cameraConf );

		Animate(camMode, 0, target->GetOrigin(), target->GetOrientation(), target->GetVelocity(), fDt, vec3_zero);
	}
	else
		Animate(CAM_MODE_TRIPOD_STATIC, 0, vec3_zero, Quaternion(), vec3_zero, fDt, vec3_zero);
}

void CCameraAnimator::Animate(	ECameraMode mode,
								int nButtons,
								const Vector3D& targetOrigin, const Quaternion& targetRotation, const Vector3D& targetVelocity,
								float fDt,
								const Vector3D& addRot )
{
	m_realMode = mode;

	Vector3D pos = targetOrigin;

	Vector3D car_forward = rotateVector(vec3_forward,targetRotation);
	Vector3D car_side = rotateVector(vec3_right,targetRotation);

	Vector3D euler_angles = EulerMatrixZXY(targetRotation); //eulers(targetRotation);
	euler_angles = VRAD2DEG(euler_angles);
	euler_angles *= Vector3D(-1,1,-1);

	if(mode == CAM_MODE_OUTCAR || mode == CAM_MODE_OUTCAR_FIXED)
	{
		if(cam_velocityeffects.GetBool())
		{
			Vector3D velDiffFwd = car_forward * dot(m_vecCameraVelDiff, car_forward) * cam_velocity_forwardmod.GetFloat();
			Vector3D velDiffSide = car_side * dot(m_vecCameraVelDiff, car_side) * cam_velocity_sidemod.GetFloat();
			Vector3D velDiffUp = vec3_up * dot(m_vecCameraVelDiff, vec3_up)  * cam_velocity_upmod.GetFloat();

			pos -= velDiffFwd+velDiffSide+velDiffUp;

			Vector3D camVelDiff = (targetVelocity-m_vecCameraVel)*CAR_ACCEL_VEL_FACTOR;

			if(	length(camVelDiff) > CAR_DECEL_VEL_MINDIFF)
				m_vecCameraSpringVel += camVelDiff;

			SpringFunction(m_vecCameraVelDiff, m_vecCameraSpringVel, cam_velocity_springconst.GetFloat(), cam_velocity_springdamp.GetFloat(), fDt);

			m_vecCameraVel = targetVelocity;
		}
	}
	else
	{
		m_vecCameraVelDiff = vec3_zero;
		m_vecCameraSpringVel = vec3_zero;
		m_vecCameraVel = targetVelocity;
	}

	float look_angle = 0.0f;

	if(nButtons & IN_LOOKLEFT)
		look_angle = -90.0f;

	if(nButtons & IN_LOOKRIGHT)
		look_angle = 90.0f;

	float fLookAngleDiff = AngleDiff(m_fLookAngle, look_angle);

	m_fLookAngle += fLookAngleDiff * fDt * CAM_LOOK_TURN_SPEED;

	m_fLookAngle = ConstrainAngle180(m_fLookAngle);

	bool bLookBack = (nButtons & IN_LOOKLEFT) && (nButtons & IN_LOOKRIGHT);

	Vector3D cam_vec = cross(vec3_up, car_forward);

	float Yangle = RAD2DEG(atan2f(cam_vec.z, cam_vec.x));

	if (Yangle < 0.0)
		Yangle += 360.0f;

	float fAngDiff = AngleDiff(m_fTempCamAngle, Yangle);

	m_fTempCamAngle += fAngDiff * fDt * CAM_TURN_SPEED;

	if(mode == CAM_MODE_OUTCAR && m_carConfig)
	{
		Vector3D cam_angles = Vector3D(0, m_fTempCamAngle - m_fLookAngle, 0) + addRot;

		if( bLookBack )
			cam_angles = Vector3D(0, Yangle+180, 0) + addRot;

		Vector3D forward;
		AngleVectors(cam_angles, &forward);

		Vector3D cam_target = pos + Vector3D(0, m_carConfig->height, 0);

		Vector3D cam_pos_h = pos + Vector3D(0,m_carConfig->height,0);
		Vector3D cam_pos = cam_pos_h - forward*m_cameraDistVar;
		Vector3D cam_pos_low = pos + Vector3D(0,CAM_HEIGHT_TRACE,0) - forward*m_cameraDistVar;

		CollisionData_t back_coll;

		btBoxShape sphere(btVector3(0.5f, 0.5f, 0.5f));
		g_pPhysics->TestConvexSweep(&sphere, identity(), cam_pos_h, cam_pos, back_coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_WATER);

		if( back_coll.fract < 1.0f )
		{
			m_cameraDistVar = lerp(0.0f, m_cameraDistVar, back_coll.fract);
		}

		m_cameraDistVar += fDt*2.0f;

		if(m_cameraDistVar > m_carConfig->dist)
			m_cameraDistVar = m_carConfig->dist;

		cam_pos = cam_pos_h - forward*(m_cameraDistVar-0.1f);

		CollisionData_t coll;
		g_pPhysics->TestLine(cam_pos + Vector3D(0,m_carConfig->height,0), cam_pos_low, coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_WATER);

		FReal fCamDot = fabs(dot(coll.normal, Vector3D(0,1,0)));

		if(coll.fract < 1.0f && fCamDot > 0.5f)
		{
			FReal height = coll.position.y;

			cam_pos.y = m_carConfig->height + height - CAM_HEIGHT_TRACE;
		}

		cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_computedView.SetOrigin(cam_pos);
		m_computedView.SetAngles(cam_angles);
		m_computedView.SetFOV(m_carConfig->fov);
	}
	else if(mode == CAM_MODE_OUTCAR_FIXED && m_carConfig)
	{
		euler_angles += addRot + m_rotation;

		Vector3D forward;
		AngleVectors(euler_angles, &forward);

		m_computedView.SetOrigin( pos - forward * m_carConfig->dist);
		m_computedView.SetAngles( euler_angles );
		m_computedView.SetFOV( m_cameraFOV );
	}
	else if(mode == CAM_MODE_INCAR && m_carConfig)
	{
		float lookAngle = m_fLookAngle;

		if( bLookBack )
			lookAngle = 180.0f;

		euler_angles = EulerMatrixZXY(targetRotation * Quaternion(-DEG2RAD(lookAngle), vec3_up));
		euler_angles = VRAD2DEG(euler_angles);
		euler_angles *= Vector3D(-1,1,-1);

		Vector3D forward, up, right;
		AngleVectors(euler_angles, &forward, &right, &up);

		Vector3D vecDir = forward*m_carConfig->distInCar;

		if( !bLookBack )
		{
			float dist_multipler = (m_fLookAngle / 90.0f);
			vecDir = lerp(forward * m_carConfig->distInCar, right*m_carConfig->widthInCar*sign(dist_multipler), fabs(dist_multipler));
		}

		Vector3D camPos = pos + vecDir + up * m_carConfig->heightInCar;

		m_computedView.SetOrigin(camPos);
		m_computedView.SetAngles(euler_angles);
		m_computedView.SetFOV(m_carConfig->fov);
	}
	else if(mode == CAM_MODE_TRIPOD_ZOOM || mode == CAM_MODE_TRIPOD_FIXEDZOOM)
	{
		Vector3D cam_pos = m_dropPos;
		Vector3D cam_target = pos;

		Vector3D cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_computedView.SetAngles(cam_angles);
		m_computedView.SetOrigin(m_dropPos);

		if(mode == CAM_MODE_TRIPOD_ZOOM)
		{
			float distance = length(cam_pos - cam_target);

			float fDistance = distance;

			fDistance += ZOOM_START_DIST;

			// clamp bounds
			if(fDistance > ZOOM_END_DIST)
				fDistance = ZOOM_END_DIST;

			float distance_factor = 1.0f - (fDistance / ZOOM_END_DIST);

			float fFov = lerp(START_FOV, END_FOV, clamp(2.5f+log(1.0f-distance_factor), 0.0f, 1.0f));

			m_computedView.SetFOV(fFov);
		}
		else
		{
			m_computedView.SetFOV(m_cameraFOV);
		}

	}
	else if(mode == CAM_MODE_TRIPOD_STATIC)
	{
		m_computedView.SetAngles(m_rotation);
		m_computedView.SetOrigin(m_dropPos);
		m_computedView.SetFOV(m_cameraFOV);
	}
}

const CViewParams& CCameraAnimator::GetComputedView() const
{
	return m_computedView;
}