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

#include "car.h"
#include "CameraAnimator.h"

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
ConVar cam_velocity_mindiff("cam_velocity_mindiff", "0.15");

#define CAR_ACCEL_VEL_FACTOR	cam_velocity_accel.GetFloat() //0.15f
#define CAR_DECEL_VEL_MINDIFF		cam_velocity_mindiff.GetFloat() //5.0f

ConVar cam_velocity_springconst("cam_velocity_springconst", "15");
ConVar cam_velocity_springdamp("cam_velocity_springdamp", "8");

ConVar cam_velocityeffects("cam_velocityeffects", "1", "Enable velocity effects", CV_ARCHIVE);
/*
ConVar cam_velocity_forwardmod("cam_velocity_forwardmod", "1.0");
ConVar cam_velocity_sidemod("cam_velocity_sidemod", "0.0");
ConVar cam_velocity_upmod("cam_velocity_upmod", "2.0");
*/

ConVar cam_velocity_forwardmod("cam_velocity_forwardmod", "1.0");
ConVar cam_velocity_sidemod("cam_velocity_sidemod", "-0.5");
ConVar cam_velocity_upmod("cam_velocity_upmod", "-1.0");

ConVar cam_custom("cam_custom", "0", NULL, CV_CHEAT);
ConVar cam_custom_height("cam_custom_height", "1.3", NULL, CV_ARCHIVE);
ConVar cam_custom_dist("cam_custom_dist", "7", NULL, CV_ARCHIVE);
ConVar cam_custom_fov("cam_custom_fov", "52", NULL, CV_ARCHIVE);

DECLARE_CMD(v_shake, "shakes view", 0)
{
	if(CMD_ARGC < 1)
		return;

	float shakeMagnitude = atof(CMD_ARGV(0).c_str());
	float shakeTime = CMD_ARGC > 1 ? atof(CMD_ARGV(1).c_str()) : 1.0f;

	g_pCameraAnimator->ViewShake(shakeMagnitude, shakeTime);
}

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
	m_scriptControl(false),
	m_shakeDecayCurTime(0.0f),
	m_shakeMagnitude(0.0f)
	//m_targetForwardSpeedModifier(1.0f)
{
	m_carConfig.dist = 7.0f;
	m_carConfig.distInCar = 7.0f;
	m_carConfig.height = 1.3f;
	m_carConfig.heightInCar = 0.5f;
	m_carConfig.widthInCar = 0.7f;
	m_carConfig.fov = DEFAULT_CAMERA_FOV;
}

void CCameraAnimator::SetCameraProps( const carCameraConfig_t& conf )
{
	m_carConfig = conf;

	if(cam_custom.GetBool())
	{
		m_carConfig.dist = cam_custom_dist.GetFloat();
		m_carConfig.height = cam_custom_height.GetFloat();
		m_carConfig.fov = cam_custom_fov.GetFloat();
	}
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
	m_scriptControl = false;

	m_shakeDecayCurTime = 0.0f;
	m_shakeMagnitude = 0.0f;

	//m_targetForwardSpeedModifier = 1.0f;

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
			Vector3D dropPos = target->GetOrigin() + Vector3D(0,target->m_conf->physics.body_size.y,0) - target->GetForwardVector()*target->m_conf->physics.body_size.z*1.1f;

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

		SetCameraProps( target->m_conf->cameraConf );

		Animate(camMode, nButtons, target->GetOrigin(), target->GetOrientation(), target->GetVelocity(), fDt, vec3_zero);
	}
	else
		Animate(m_mode, nButtons, vec3_zero, Quaternion(), vec3_zero, fDt, vec3_zero);

	m_oldBtns = nButtons;
}

Vector3D CCameraAnimator::ShakeView( float fDt )
{
	if(m_shakeDecayCurTime <= 0.0f)
	{
		m_shakeMagnitude = 0.0f;
		m_shakeDecayCurTime = 0.0f;
		return vec3_zero;
	}

	// maximum for 3 seconds shake, heavier is more
	float fShakeRate = m_shakeDecayCurTime / 3.0f;

	m_shakeMagnitude -= m_shakeMagnitude*(1.0f-fShakeRate)*fDt;
	m_shakeDecayCurTime -= fDt;

	return fShakeRate * m_shakeMagnitude * Vector3D(
		sin(cos(fShakeRate+m_shakeDecayCurTime*44.0f)),
		sin(cos(fShakeRate+m_shakeDecayCurTime*115.0f)),
		sin(cos(fShakeRate+m_shakeDecayCurTime*77.0f)));
}

void CCameraAnimator::ViewShake(float fMagnutude, float fTime)
{
	m_shakeDecayCurTime += fTime;

	// 1 sec only
	if(m_shakeDecayCurTime > 1.0f)
		m_shakeDecayCurTime = 1.0f;

	m_shakeMagnitude += fMagnutude;
}

void CCameraAnimator::L_Update( float fDt, CCar* target )
{
	if( target )
	{
		ECameraMode camMode = m_mode;

		if( target->IsInWater() && camMode == CAM_MODE_INCAR )
			camMode = CAM_MODE_OUTCAR;

		SetCameraProps( target->m_conf->cameraConf );

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

	Vector3D shakeVec = ShakeView( fDt );

	Vector3D pos = targetOrigin;

	Vector3D car_forward = rotateVector(vec3_forward,targetRotation);
	Vector3D car_side = rotateVector(vec3_right,targetRotation);

	Vector3D euler_angles = EulerMatrixZXY(targetRotation); //eulers(targetRotation);
	euler_angles = VRAD2DEG(euler_angles);
	euler_angles *= Vector3D(-1,1,-1);

	//float targetForwardSpeed = pow(1.0f - clamp(fabs(dot(m_vecCameraVel, car_forward)), 0.0f, 20.0f)*0.05f, 2.0f);

	//m_targetForwardSpeedModifier = lerp(m_targetForwardSpeedModifier, targetForwardSpeed, fDt*5.0f);

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

	if(mode == CAM_MODE_OUTCAR)
	{
		float desiredHeight = m_carConfig.height;// - m_targetForwardSpeedModifier*0.15f;
		float desiredDist = m_carConfig.dist;// - m_targetForwardSpeedModifier*0.9f;

		Vector3D cam_angles = Vector3D(0, m_fTempCamAngle - m_fLookAngle, 0) + addRot;

		if( bLookBack )
			cam_angles = Vector3D(0, Yangle+180, 0) + addRot;

		Vector3D forward;
		AngleVectors(cam_angles, &forward);

		Vector3D cam_target = pos + Vector3D(0, desiredHeight, 0);

		Vector3D cam_pos_h = pos + Vector3D(0,desiredHeight,0);
		Vector3D cam_pos = cam_pos_h - forward*m_cameraDistVar;
		Vector3D cam_pos_low = pos + Vector3D(0,CAM_HEIGHT_TRACE,0) - forward*m_cameraDistVar;

		CollisionData_t back_coll;

		btBoxShape sphere(btVector3(0.5f, 0.5f, 0.5f));
		g_pPhysics->TestConvexSweep(&sphere, identity(), cam_pos_h, cam_pos, back_coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS);

		if( back_coll.fract < 1.0f )
		{
			m_cameraDistVar = lerp(0.0f, m_cameraDistVar, back_coll.fract);
		}

		m_cameraDistVar += fDt*2.0f;

		m_cameraDistVar = min(m_cameraDistVar, desiredDist);

		cam_pos = cam_pos_h - forward*(m_cameraDistVar-0.1f);

		CollisionData_t coll;
		g_pPhysics->TestLine(cam_pos + Vector3D(0,desiredHeight,0), cam_pos_low, coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_WATER);

		FReal fCamDot = fabs(dot(coll.normal, Vector3D(0,1,0)));

		if(coll.fract < 1.0f && fCamDot > 0.5f)
		{
			FReal height = coll.position.y;

			cam_pos.y = desiredHeight + height - CAM_HEIGHT_TRACE;
		}

		cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_computedView.SetOrigin(cam_pos);
		m_computedView.SetAngles(cam_angles + shakeVec);
		m_computedView.SetFOV(m_carConfig.fov);
	}
	else if(mode == CAM_MODE_OUTCAR_FIXED)
	{
		euler_angles += addRot + m_rotation;

		Vector3D forward;
		AngleVectors(euler_angles, &forward);

		m_computedView.SetOrigin( pos - forward * m_carConfig.dist);
		m_computedView.SetAngles( euler_angles + shakeVec );
		m_computedView.SetFOV( m_cameraFOV );
	}
	else if(mode == CAM_MODE_INCAR)
	{
		float lookAngle = m_fLookAngle;

		if( bLookBack )
			lookAngle = 180.0f;

		euler_angles = EulerMatrixZXY(targetRotation * Quaternion(-DEG2RAD(lookAngle), vec3_up) * Quaternion(DEG2RAD(1.5), vec3_right));
		euler_angles = VRAD2DEG(euler_angles);
		euler_angles *= Vector3D(-1,1,-1);

		Vector3D forward, up, right;
		AngleVectors(euler_angles, &forward, &right, &up);

		Vector3D vecDir = forward*m_carConfig.distInCar;

		if( !bLookBack )
		{
			float dist_multipler = (m_fLookAngle / 90.0f);
			vecDir = lerp(forward * m_carConfig.distInCar, right*m_carConfig.widthInCar*sign(dist_multipler), fabs(dist_multipler));
		}

		Vector3D camPos = pos + vecDir + up * m_carConfig.heightInCar;

		m_computedView.SetOrigin(camPos);
		m_computedView.SetAngles(euler_angles + shakeVec);
		m_computedView.SetFOV(m_carConfig.fov);
	}
	else if(mode == CAM_MODE_TRIPOD_ZOOM || mode == CAM_MODE_TRIPOD_FIXEDZOOM)
	{
		Vector3D cam_pos = m_dropPos;
		Vector3D cam_target = pos;

		Vector3D cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_computedView.SetAngles(cam_angles + shakeVec);
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

			float fFov = lerp(START_FOV, END_FOV, clamp(2.5f+logf(1.0f-distance_factor), 0.0f, 1.0f));

			m_computedView.SetFOV(fFov);
		}
		else
		{
			m_computedView.SetFOV(m_cameraFOV);
		}

	}
	else if(mode == CAM_MODE_TRIPOD_STATIC)
	{
		m_computedView.SetAngles(m_rotation + shakeVec);
		m_computedView.SetOrigin(m_dropPos);
		m_computedView.SetFOV(m_cameraFOV);
	}
}

const CViewParams& CCameraAnimator::GetComputedView() const
{
	return m_computedView;
}
