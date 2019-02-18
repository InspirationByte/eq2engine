//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI chaser manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_TargetChaser.h"
#include "car.h"

#include "world.h"

ConVar ai_debug_chaser("ai_debug_chaser", "0");

CAITargetChaserManipulator::CAITargetChaserManipulator()
{
	m_driveTarget = vec3_zero;
}

void CAITargetChaserManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(car->GetPhysicsBody());
	collFilter.AddObject(m_excludeColl);

	CollisionData_t steeringTargetColl;

	// use vehicle world matrix
	const Matrix4x4& bodyMat = car->m_worldMatrix;

	Vector3D carPos = car->GetOrigin();
	const Vector3D& carBodySize = car->m_conf->physics.body_size;

	const float AI_OBSTACLE_SPHERE_RADIUS = carBodySize.x*2.0f;
	const float AI_OBSTACLE_SPHERE_SPEED_SCALE = 0.01f;

	Vector3D carForward = car->GetForwardVector();
	Vector3D carRight = car->GetRightVector();

	const Vector3D& carVelocity = car->GetVelocity();

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;

	float traceShapeRadius = AI_OBSTACLE_SPHERE_RADIUS + speedMPS * AI_OBSTACLE_SPHERE_SPEED_SCALE;

	btSphereShape sphereTraceShape(traceShapeRadius);

	// add half car length to the car position
	carPos += carForward * carBodySize.z;

	const float AI_CHASE_TARGET_VELOCITY_SCALE = 0.6f;

	const float AI_STEERING_START_AGRESSIVE_DISTANCE_START = 10.0f;
	const float AI_STEERING_START_AGRESSIVE_DISTANCE_END = 4.0f;

	// brake curve
	const float AI_CHASE_BRAKE_CURVE = 0.45f;

	const float AI_CHASE_BRAKE_MIN = 0.05f;

	const float AI_BRAKE_TARGET_DISTANCE_SCALING = 2.5f;
	const float AI_BRAKE_TARGET_DISTANCE_CURVE = 0.5f;

	const float AI_STEERING_TARGET_DISTANCE_SCALING = 0.5f;
	const float AI_STEERING_TARGET_DISTANCE_CURVE = 2.5f;

	const float AI_LOWSPEED_CURVE = 15.0f;
	const float AI_LOWSPEED_END = 8.0f;	// 8 meters per second

	float tireFrictionModifier = 0.0f;
	{
		int numWheels = car->GetWheelCount();

		for (int i = 0; i < numWheels; i++)
		{
			CCarWheel& wheel = car->GetWheel(i);

			if (wheel.GetSurfaceParams())
				tireFrictionModifier += wheel.GetSurfaceParams()->tirefriction;
		}

		tireFrictionModifier /= float(numWheels);
	}

	float weatherBrakeDistModifier = s_weatherBrakeDistanceModifier[g_pGameWorld->m_envConfig.weatherType] * (1.0f - tireFrictionModifier);
	float weatherBrakePow = s_weatherBrakeCurve[g_pGameWorld->m_envConfig.weatherType] * tireFrictionModifier;

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_CURVE);

	int traceContents = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE;

	// add velocity offset
	Vector3D steeringTargetPos = m_driveTarget + m_driveTargetVelocity * AI_CHASE_TARGET_VELOCITY_SCALE;

	// preliminary steering dir
	Vector3D steeringDir = fastNormalize(steeringTargetPos - carPos);

	float distanceToTarget = length(m_driveTarget - carPos);

	float distanceScaling = 1.0f - RemapValClamp(distanceToTarget, AI_STEERING_START_AGRESSIVE_DISTANCE_END, AI_STEERING_START_AGRESSIVE_DISTANCE_START, 0.0f, 1.0f);

	float steeringScaling = 1.0f + pow(distanceScaling, AI_STEERING_TARGET_DISTANCE_CURVE) * AI_STEERING_TARGET_DISTANCE_SCALING;

	if (ai_debug_chaser.GetBool())
	{
		debugoverlay->Line3D(carPos, steeringTargetPos, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
		debugoverlay->Box3D(steeringTargetPos - 0.5f, steeringTargetPos + 0.5f, ColorRGBA(1, 1, 0, 1.0f), fDt);

		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "distance To Target: %.2f", distanceToTarget);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "distance scaling: %g", distanceScaling);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "steering scaling: %g", steeringScaling);
	}

	// calculate brake first
	float brakeScaling = 1.0f + pow(distanceScaling, AI_BRAKE_TARGET_DISTANCE_CURVE) * AI_BRAKE_TARGET_DISTANCE_SCALING;
	float brakeFac = (1.0f - fabs(dot(steeringDir, carForward)))*brakeScaling;

	if (brakeFac < AI_CHASE_BRAKE_MIN)
		brakeFac = 0.0f;

	float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 6.0f, 16.0f);
	/*
	// trace from position A to position B first
	g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
		carPos, steeringTargetPos, pathColl,
		traceContents,
		&collFilter);

	if(pathColl.fract < 1.0f)
	{
		float AI_OBSTACLE_PATH_CORRECTION_AMOUNT = 1.1f;
		float AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT = 1.5f;

		Vector3D offsetAmount = pathColl.normal * (traceShapeRadius * AI_OBSTACLE_PATH_CORRECTION_AMOUNT + lowSpeedFactor*AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT);

		Vector3D newSteeringTarget = pathColl.position + offsetAmount;

		float steerToCorrectionDot = dot(steeringDir, fastNormalize(newSteeringTarget - carPos));

		if(steerToCorrectionDot > 0.5f)
			steeringTargetPos = newSteeringTarget;

		if(ai_debug_navigator.GetBool())
		{
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "steering to path correction (collision): %g", steerToCorrectionDot);

			debugoverlay->Line3D(pathColl.position, steeringTargetPos, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Sphere3D(steeringTargetPos, traceShapeRadius, ColorRGBA(1, 1, 0, 1.0f), fDt);
		}
	}
	*/
	// final steering dir after collision tests
	steeringDir = fastNormalize(steeringTargetPos - carPos);

	// trace forward from car using speed
	g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
		carPos, carPos + carForward * forwardTraceDistanceBySpeed, steeringTargetColl,
		traceContents,
		&collFilter);

	if (steeringTargetColl.fract < 1.0f)
	{
		float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 0.25f;

		Vector3D collPoint(steeringTargetColl.position);

		Plane dPlane(carRight, -dot(carPos, carRight));
		float posSteerFactor = -dPlane.Distance(collPoint);

		steeringDir += carRight * posSteerFactor * (1.0f - steeringTargetColl.fract) * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;
		steeringDir = normalize(steeringDir);
	}

	// calculate steering
	Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

	handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z) * steeringScaling;
	handling.steering = clamp(handling.steering, -1.0f, 1.0f);

	handling.braking = pow(brakeFac, AI_CHASE_BRAKE_CURVE*weatherBrakePow) * lowSpeedFactor;
	handling.braking = min(handling.braking, 1.0f);
	handling.acceleration = 1.0f - handling.braking;
}