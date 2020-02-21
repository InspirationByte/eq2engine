//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate camera animator
//////////////////////////////////////////////////////////////////////////////////


//
// TODO:	rework this code, it looks like piece of shit
//			add director features HERE
//

#include "car.h"
#include "CameraAnimator.h"
#include "input.h"

const float DEFAULT_CAMERA_FOV		= 52.0f;

const float CAM_TURN_SPEED			= 2.65f;
const float CAM_LOOK_TURN_SPEED		= 15.0f;

const float CAM_HEIGHT_TRACE		= -0.3f;

const float ZOOM_START_DIST			= 8.0f;
const float ZOOM_END_DIST			= 200.0f;

const float MAX_CAR_DIST			= 250.0f;

const float START_FOV				= 40.0f;
const float END_FOV					= 10.0f;

const float CAM_DISTANCE_SPEED		= 2.0f;

ConVar cam_velocity_accel("cam_velocity_accel", "0.5");
ConVar cam_velocity_mindiff("cam_velocity_mindiff", "0.15");

#define CAR_ACCEL_VEL_FACTOR		cam_velocity_accel.GetFloat() //0.15f
#define CAR_DECEL_VEL_MINDIFF		cam_velocity_mindiff.GetFloat() //5.0f

ConVar cam_velocity_springconst("cam_velocity_springconst", "15");
ConVar cam_velocity_springdamp("cam_velocity_springdamp", "8");

ConVar cam_velocityeffects("cam_velocityeffects", "1", "Enable velocity effects", CV_ARCHIVE);

ConVar cam_velocity_forwardmod("cam_velocity_forwardmod", "1.0");
ConVar cam_velocity_sidemod("cam_velocity_sidemod", "-0.5");
ConVar cam_velocity_upmod("cam_velocity_upmod", "1.0");

bool IsStaticCameraMode(ECameraMode mode)
{
	return	mode == CAM_MODE_TRIPOD ||
			mode == CAM_MODE_TRIPOD_FOLLOW ||
			mode == CAM_MODE_TRIPOD_FOLLOW_ZOOM;
}

// cameras used during gameplay
bool IsGameCamera(ECameraMode mode)
{
	return	mode == CAM_MODE_INCAR ||
			mode == CAM_MODE_OUTCAR ||
			mode == CAM_MODE_TRIPOD_FOLLOW_ZOOM;
}

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
	m_interpLookAngle(0.0f),
	m_interpCamAngle(0.0f),
	m_dropPos(0.0f),
	m_rotation(0.0f),
	m_cameraFOV(DEFAULT_CAMERA_FOV),
	m_scriptControl(false),
	m_shakeDecayCurTime(0.0f),
	m_shakeMagnitude(0.0f),
	m_oldBtns(0),
	m_btns(0)
{
	m_camConfig.dist = 7.0f;
	m_camConfig.distInCar = 7.0f;
	m_camConfig.height = 1.3f;
	m_camConfig.heightInCar = 0.5f;
	m_camConfig.widthInCar = 0.7f;
	m_camConfig.fov = DEFAULT_CAMERA_FOV;
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

void CCameraAnimator::CenterView()
{
	m_vecCameraVel = vec3_zero;
	m_vecCameraVelDiff = vec3_zero;
	m_vecCameraSpringVel = vec3_zero;

	m_shakeDecayCurTime = 0.0f;
	m_shakeMagnitude = 0.0f;
}

void CCameraAnimator::Reset()
{
	m_vecCameraVel = vec3_zero;
	m_vecCameraVelDiff = vec3_zero;
	m_vecCameraSpringVel = vec3_zero;
	m_interpLookAngle = 0.0f;
	m_interpCamAngle = 0.0f;
	m_scriptControl = false;

	m_oldBtns = 0;
	m_btns = 0;

	m_shakeDecayCurTime = 0.0f;
	m_shakeMagnitude = 0.0f;

	//m_targetForwardSpeedModifier = 1.0f;

	if(m_mode > CAM_MODE_INCAR)
		m_mode = CAM_MODE_OUTCAR;
}

void CCameraAnimator::SetMode(int newMode)
{
	if (newMode != m_mode)
	{
		m_interpLookAngle = vec3_zero;
		m_rotation = vec3_zero;
	}

	m_mode = (ECameraMode)newMode;
}

void CCameraAnimator::Update( float fDt, int nButtons, CGameObject* target)
{
	extern ConVar r_zfar;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;

	m_oldBtns = m_btns;

	if(fDt > 0.0f)
		m_btns = nButtons;

	if (target)
	{
		// Check camera switch buttons
		if ((nButtons & IN_CHANGECAM) && !(m_oldBtns & IN_CHANGECAM))
		{
			int newMode = (int)m_mode+1;

			// rollin
			while (!IsGameCamera((ECameraMode)newMode))
			{
				newMode++;
				if (newMode >= CAM_MODES)
				{
					newMode = CAM_MODE_OUTCAR;
					break;
				}
			}

			// find a drop position
			if (IsStaticCameraMode((ECameraMode)newMode))
			{
				cameraConfig_t cam;
				eqPhysCollisionFilter tmp;

				target->ConfigureCamera(cam, tmp);

				const Vector3D& velocity = target->GetVelocity();

				Vector3D forward = transpose(target->m_worldMatrix).rows[2].xyz();

				if (dot(velocity, velocity) > 0.25f)
					forward = normalize(target->GetVelocity());

				// compute drop position
				m_dropPos = target->GetOrigin() + vec3_up*cam.heightInCar - forward*cam.distInCar*2.0f;
			}

			m_mode = (ECameraMode)newMode;
		}

		// automatic mode switching based on distance
		if (IsStaticCameraMode(m_mode))
		{
			float dist = length(target->GetOrigin() - GetOrigin());

			float zfar = r_zfar.GetFloat();

			if (dist > min(zfar, 200.0f))
				m_mode = CAM_MODE_OUTCAR;
		}

		if (fDt <= 0.0f)
			return;

		AnimateForObject(m_mode, fDt, target, collFilter);
	}
	else
		Animate(m_mode, vec3_zero, Quaternion(), vec3_zero, fDt, collFilter);
}

void CCameraAnimator::AnimateForObject(ECameraMode camMode, float fDt, CGameObject* target, eqPhysCollisionFilter& collFilter)
{
	// TODO: other control for addRot

	Quaternion camOrient;

	target->ConfigureCamera(m_camConfig, collFilter);

	// calculate car-specific camera with extra features
	if (IsCar(target))
	{
		CCar* carObj = (CCar*)target;

		camOrient = carObj->GetOrientation();

		if (carObj->IsInWater() && camMode == CAM_MODE_INCAR)
			camMode = CAM_MODE_OUTCAR;
	}
	else
	{
		const Vector3D camAngles = target->GetAngles();
		camOrient = Quaternion(DEG2RAD(camAngles.x), DEG2RAD(camAngles.y), DEG2RAD(camAngles.z));
	}

	Animate(camMode, target->GetOrigin(), camOrient, target->GetVelocity(), fDt, collFilter);
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

void CCameraAnimator::L_Update( float fDt, CGameObject* target )
{
	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;

	if(target)
		AnimateForObject(m_mode, fDt, target, collFilter);
	else
		Animate(CAM_MODE_TRIPOD, vec3_zero, Quaternion(), vec3_zero, fDt, collFilter);
}

void CCameraAnimator::Animate(ECameraMode mode,
	const Vector3D& targetOrigin, const Quaternion& targetRotation, const Vector3D& targetVelocity,
	float fDt,
	eqPhysCollisionFilter& traceIgnore)
{
	m_realMode = mode;

	Vector3D shakeVec = ShakeView(fDt);

	Vector3D finalTargetPos = targetOrigin;

	Vector3D targetForward = rotateVector(vec3_forward, targetRotation);
	Vector3D targetSide = rotateVector(vec3_right, targetRotation);
	Vector3D targetUp = rotateVector(vec3_up, targetRotation);

	if (mode == CAM_MODE_OUTCAR || mode == CAM_MODE_ONCAR)
	{
		if (cam_velocityeffects.GetBool())
		{
			Vector3D velDiffFwd = targetForward * dot(m_vecCameraVelDiff, targetForward) * cam_velocity_forwardmod.GetFloat();
			Vector3D velDiffSide = targetSide * dot(m_vecCameraVelDiff, targetSide) * cam_velocity_sidemod.GetFloat();
			Vector3D velDiffUp = vec3_up * dot(m_vecCameraVelDiff, vec3_up)  * cam_velocity_upmod.GetFloat();

			finalTargetPos -= velDiffFwd + velDiffSide + velDiffUp;

			Vector3D camVelDiff = (targetVelocity - m_vecCameraVel)*CAR_ACCEL_VEL_FACTOR;

			if (length(camVelDiff) > CAR_DECEL_VEL_MINDIFF)
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

	{
		Vector3D desiredLookAngle(0.0f);

		if (m_btns & IN_LOOKLEFT)
			desiredLookAngle.y = -90.0f;
		else if (m_btns & IN_LOOKRIGHT)
			desiredLookAngle.y = 90.0f;

		desiredLookAngle += m_rotation;

		Vector3D fLookAngleDiff = AnglesDiff(m_interpLookAngle, desiredLookAngle);

		if (fDt*CAM_LOOK_TURN_SPEED*0.5f > length(fLookAngleDiff))
		{
			fLookAngleDiff = vec3_zero;
			m_interpLookAngle = desiredLookAngle;
		}


		m_interpLookAngle += fLookAngleDiff * fDt * CAM_LOOK_TURN_SPEED;
		m_interpLookAngle = NormalizeAngles180(m_interpLookAngle);
	}

	bool bLookBack = (m_btns & IN_LOOKLEFT) && (m_btns & IN_LOOKRIGHT);

	Vector3D cam_vec = cross(vec3_up, targetForward);

	float Yangle = RAD2DEG(atan2f(cam_vec.z, cam_vec.x));

	if (Yangle < 0.0)
		Yangle += 360.0f;

	float fAngDiff = AngleDiff(m_interpCamAngle, Yangle);

	m_interpCamAngle += fAngDiff * fDt * CAM_TURN_SPEED;

	Vector3D finalLookAngle = Vector3D(0, m_interpCamAngle, 0) - m_interpLookAngle;
	Vector3D finalLookAngleIn = m_interpLookAngle;

	if (bLookBack)
	{
		finalLookAngle.y = Yangle + 180.0f;
		finalLookAngleIn.y = 180.0f;
	}

	switch (mode)
	{
		case CAM_MODE_OUTCAR:
		{
			float desiredHeight = m_camConfig.height;
			float desiredDist = m_camConfig.dist;

			Vector3D forward;
			AngleVectors(finalLookAngle, &forward);

			float heightOffset = 0.0f;

			// trace camera for height
			{
				
				Vector3D camPosTest = finalTargetPos - forward * desiredDist;

				Vector3D cam_pos_hi = camPosTest + Vector3D(0, desiredHeight*3.0f, 0);
				Vector3D cam_pos_low = camPosTest + Vector3D(0, CAM_HEIGHT_TRACE, 0);

				CollisionData_t height_coll;
				g_pPhysics->TestLine(cam_pos_hi, cam_pos_low, height_coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_WATER);

				float facingGround = dot(height_coll.normal, vec3_up);

				if (height_coll.fract < 1.0f && facingGround > 0.5f)
				{
					heightOffset = (height_coll.position.y - cam_pos_low.y);
					desiredDist *= height_coll.fract*0.5f+0.5f;
				}
			}

			Vector3D camTargetPos = finalTargetPos + Vector3D(0, desiredHeight, 0);
			Vector3D finalCamPos = finalTargetPos + Vector3D(0, desiredHeight + heightOffset, 0) - forward*desiredDist;

			// trace far back to determine distToTarget
			{
				CollisionData_t back_coll;

				btBoxShape sphere(btVector3(0.5f, 0.5f, 0.5f));
				if (g_pPhysics->TestConvexSweep(&sphere, identity(), camTargetPos, finalCamPos, back_coll,
					OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_VEHICLE, &traceIgnore))
				{
					desiredDist *= max(back_coll.fract, 0.05f);
				}
			}

			// make camera out from car smoothly
			m_cameraDistVar += (desiredDist - m_cameraDistVar) * fDt * CAM_DISTANCE_SPEED;
			m_cameraDistVar = min(m_cameraDistVar, desiredDist);

			// recalculate distToTarget
			finalCamPos = finalTargetPos + Vector3D(0, desiredHeight + heightOffset, 0) - forward*(m_cameraDistVar-0.1f);

			finalLookAngle = VectorAngles(normalize(camTargetPos-finalCamPos));

			m_computedView.SetOrigin(finalCamPos);
			m_computedView.SetAngles(finalLookAngle + shakeVec);
			m_computedView.SetFOV(m_camConfig.fov);

			break;
		}
		case CAM_MODE_ONCAR:
		{
			Vector3D camAngle = Vector3D(0, Yangle, 0) - m_rotation;
			Vector3D camOffset = rotateVector(m_dropPos, targetRotation);

			m_computedView.SetOrigin(finalTargetPos + camOffset);
			m_computedView.SetAngles(camAngle + shakeVec);
			m_computedView.SetFOV(m_cameraFOV);
			break;
		}
		case CAM_MODE_INCAR:
		{
			Vector3D desiredAngles = EulerMatrixZXY(targetRotation * Quaternion(-DEG2RAD(finalLookAngleIn.y), vec3_up) * Quaternion(DEG2RAD(1.5), vec3_right));
			desiredAngles = VRAD2DEG(desiredAngles);
			desiredAngles *= Vector3D(-1, 1, -1);

			// calculate camera offset based on the look angle
			Vector3D forward;
			AngleVectors(desiredAngles, &forward);

			Vector3D camOffset = targetForward * m_camConfig.distInCar * dot(targetForward, forward) +
								 targetSide * m_camConfig.widthInCar * dot(targetSide, forward) + 
								 targetUp * m_camConfig.heightInCar;

			m_computedView.SetOrigin(finalTargetPos + camOffset);
			m_computedView.SetAngles(desiredAngles + shakeVec);
			m_computedView.SetFOV(m_camConfig.fov);
			break;
		}
		case CAM_MODE_TRIPOD_FOLLOW:
		case CAM_MODE_TRIPOD_FOLLOW_ZOOM:
		{
			Vector3D cam_target = finalTargetPos + targetUp * m_camConfig.heightInCar;
			Vector3D cam_angles = VectorAngles(normalize(cam_target - m_dropPos));

			m_computedView.SetAngles(cam_angles + shakeVec);
			m_computedView.SetOrigin(m_dropPos);

			if (mode == CAM_MODE_TRIPOD_FOLLOW_ZOOM)
			{
				float distToTarget = length(m_dropPos - cam_target);

				float zoomDistance = ZOOM_START_DIST + distToTarget;
				zoomDistance = min(zoomDistance, ZOOM_END_DIST);

				float distance_factor = zoomDistance / ZOOM_END_DIST;

				float fFov = lerp(START_FOV, END_FOV, clamp(2.5f + logf(distance_factor), 0.0f, 1.0f));

				m_computedView.SetFOV(fFov);
			}
			else
			{
				m_computedView.SetFOV(m_cameraFOV);
			}
			break;
		}
		case CAM_MODE_TRIPOD:
		{
			m_computedView.SetAngles(m_rotation + shakeVec);
			m_computedView.SetOrigin(m_dropPos);
			m_computedView.SetFOV(m_cameraFOV);
			break;
		}
	}
}

const CViewParams& CCameraAnimator::GetComputedView() const
{
	return m_computedView;
}
