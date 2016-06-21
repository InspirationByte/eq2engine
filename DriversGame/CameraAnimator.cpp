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
#include "world.h"
#include "input.h"

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
	m_cameraFOV(DEFAULT_CAMERA_FOV)
{
	
}

void CCameraAnimator::SetCameraProps( const carCameraConfig_t& conf )
{
	m_carConfig = conf;
}

void CCameraAnimator::SetFOV(float fFOV)
{
	m_cameraFOV = fFOV;
}

void CCameraAnimator::SetDropPosition(const Vector3D& camPos)
{
	m_dropPos = camPos;
}

void CCameraAnimator::SetRotation(const Vector3D& camRot)
{
	m_rotation = camRot;
}

void CCameraAnimator::CalmDown()
{
	m_vecCameraVel = vec3_zero;
	m_vecCameraVelDiff = vec3_zero;
	m_vecCameraSpringVel = vec3_zero;
	m_fLookAngle = 0.0f;
	m_fTempCamAngle = 0.0f;
}

void CCameraAnimator::Animate(	ECameraMode mode, 
								int nButtons, 
								const Vector3D& targetOrigin, const Matrix4x4& targetRotation, const Vector3D& targetVelocity, 
								float fDt, 
								Vector3D& addRot )
{
	m_mode = mode;

	Vector3D pos = targetOrigin;

	Vector3D car_forward = targetRotation.rows[2].xyz();

	if(mode == CAM_MODE_OUTCAR || mode == CAM_MODE_OUTCAR_FIXED)
	{
		if(cam_velocityeffects.GetBool())
		{
			Vector3D car_side = targetRotation.rows[0].xyz();

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
		Vector3D cam_angles = Vector3D(0, m_fTempCamAngle - m_fLookAngle, 0) + addRot;

		if( bLookBack )
			cam_angles = Vector3D(0, Yangle+180, 0) + addRot;

		Vector3D forward;
		AngleVectors(cam_angles, &forward);

		Vector3D cam_target = pos + Vector3D(0, m_carConfig.height, 0);

		Vector3D cam_pos_h = pos + Vector3D(0,m_carConfig.height,0);
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

		if(m_cameraDistVar > m_carConfig.dist)
			m_cameraDistVar = m_carConfig.dist;

		cam_pos = cam_pos_h - forward*(m_cameraDistVar-0.1f);

		CollisionData_t coll;
		g_pPhysics->TestLine(cam_pos + Vector3D(0,m_carConfig.height,0), cam_pos_low, coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_WATER);

		FReal fCamDot = fabs(dot(coll.normal, Vector3D(0,1,0)));

		if(coll.fract < 1.0f && fCamDot > 0.5f)
		{
			FReal height = coll.position.y;

			cam_pos.y = m_carConfig.height + height - CAM_HEIGHT_TRACE;
		}

		//Vector3D cam_forward;
		//AngleVectors(cam_angles, &cam_forward);

		cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_viewParams.SetOrigin(cam_pos);
		m_viewParams.SetAngles(cam_angles);
		m_viewParams.SetFOV(m_carConfig.fov);
	}
	else if(mode == CAM_MODE_OUTCAR_FIXED)
	{
		Vector3D euler_angles = EulerMatrixZXY(transpose(targetRotation.getRotationComponent()));//*transpose(lookmatrix));
		euler_angles = VRAD2DEG(euler_angles);
		euler_angles *= Vector3D(-1,1,-1);

		euler_angles += addRot + m_rotation;

		Vector3D forward;
		AngleVectors(euler_angles, &forward);

		m_viewParams.SetOrigin( pos - forward * m_carConfig.dist);
		m_viewParams.SetAngles( euler_angles );
		m_viewParams.SetFOV( m_cameraFOV );
	}
	else if(mode == CAM_MODE_INCAR)
	{
		Matrix3x3 lookmatrix;

		if( bLookBack )
			lookmatrix = rotateY3(DEG2RAD(180));
		else
			lookmatrix = rotateY3(DEG2RAD(m_fLookAngle));

		lookmatrix = rotateX3(DEG2RAD(2.5f))*lookmatrix;

		Vector3D euler_angles = EulerMatrixZXY(transpose(targetRotation.getRotationComponent()));//*transpose(lookmatrix));
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
		else
		{
			vecDir *= -1.0f;
		}

		Vector3D camPos = pos + vecDir + up * m_carConfig.heightInCar;

		debugoverlay->Box3D(camPos-0.1f, camPos+0.1f, ColorRGBA(0,1,0,1), 0.0f);
		debugoverlay->Text3D(camPos, 100.0f, ColorRGBA(1), "angle: %g", m_fLookAngle);

		euler_angles = EulerMatrixZXY(transpose(targetRotation.getRotationComponent())*transpose(lookmatrix));
		euler_angles = VRAD2DEG(euler_angles);
		euler_angles *= Vector3D(-1,1,-1);

		m_viewParams.SetOrigin(camPos);
		m_viewParams.SetAngles(euler_angles);
		m_viewParams.SetFOV(m_carConfig.fov);
	}
	else if(mode == CAM_MODE_TRIPOD_ZOOM || mode == CAM_MODE_TRIPOD_FIXEDZOOM)
	{
		Vector3D cam_pos = m_dropPos;
		Vector3D cam_target = pos;//GetOrigin();

		Vector3D cam_angles = VectorAngles(normalize(cam_target - cam_pos));

		m_viewParams.SetAngles(cam_angles);
		m_viewParams.SetOrigin(m_dropPos);

		if(mode == CAM_MODE_TRIPOD_ZOOM)
		{
			float distance = length(cam_pos - cam_target);

			float fDistance = distance;

			fDistance += ZOOM_START_DIST;

			// clamp bounds
			if(fDistance > ZOOM_END_DIST)
				fDistance = ZOOM_END_DIST;

			float distance_factor = 1.0f - (fDistance / ZOOM_END_DIST);

			//distance_factor = distance_factor*distance_factor;

			float fFov = lerp(START_FOV, END_FOV, clamp(2.5f+log(1.0f-distance_factor), 0.0f, 1.0f));
			m_viewParams.SetFOV(fFov);
		}
		else
		{
			m_viewParams.SetFOV(m_cameraFOV);
		}
		
	}
	else if(mode == CAM_MODE_TRIPOD_STATIC)
	{
		m_viewParams.SetAngles(m_rotation);
		m_viewParams.SetOrigin(m_dropPos);
		m_viewParams.SetFOV(m_cameraFOV);
	}
}

CViewParams& CCameraAnimator::GetCamera()
{
	return m_viewParams;
}