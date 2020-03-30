//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI stability control
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_StabilityControl.h"
#include "car.h"

ConVar ai_debug_stability("ai_debug_stability", "0");

const float AI_STABILITY_MIN_INAIR_TIME = 0.15f;
const float AI_STABILITY_LANDING_COOLDOWN = 0.25f;
const float AI_MIN_LATERALSLIDE = 0.5f;
const float AI_MIN_ANGULARVEL = 0.35f;

const float AI_STEERING_SLIDING_ACCELERATOR_COMPENSATION_SCALE = 0.15f;
const float AI_STEERING_SLIDING_ACCELERATOR_COMPENSATION_MIN = 0.2f;

CAIStabilityControlManipulator::CAIStabilityControlManipulator()
{
	m_landingCooldown = 0.0f;
	m_inAirTime = 0.0f;
}

void CAIStabilityControlManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	float lateralSliding = car->GetLateralSlidingAtWheels(true);
	lateralSliding = fabs(max(lateralSliding, AI_MIN_LATERALSLIDE) - AI_MIN_LATERALSLIDE) * sign(lateralSliding);

	const float carSpeedMPS = car->GetSpeed()*KPH_TO_MPS;

	const Vector3D& linearVelocity = car->GetVelocity();
	const Vector3D& angularVelocity = car->GetPhysicsBody()->GetAngularVelocity();

	if(ai_debug_stability.GetBool())
	{
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "lateral sliding: %g", lateralSliding);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "angular rot: %g", angularVelocity.y);
	}

	const float AI_SLIDING_CORRECTION = 0.15f;
	const float AI_SLIDING_CURVE = 1.5f;

	const float AI_ROTATION_CORRECTION = 0.07f;
	const float AI_ROTATION_CURVE = 1.25f;

	const float AI_CORRECTION_LIMIT = 0.8f;

	const float AI_AUTOHANDBRAKE_STEERING_PERCENTAGE = 0.65f;

	const float AI_MAX_ANGULAR_VELOCITY_AUTOHANDBRAKE = 1.5f;

	const float AI_SPEED_CORRECTION_MINSPEED = 10.0f;	// meters per sec
	const float AI_SPEED_CORRECTION_MAXSPEED = 25.0f;	// meters per sec
	const float AI_SPEED_CORRECTION_INITIAL = 0.25f;

	float torqueRange = 1.0f / car->GetTorqueScale();

	bool allWheelsOnGround = (car->GetWheelCount() - car->GetWheelsOnGround()) == 0;

	if (!allWheelsOnGround)
		handling.acceleration = 0.0f;
	
	if (carSpeedMPS > AI_SPEED_CORRECTION_MINSPEED)// && !m_initialHandling.autoHandbrake)
	{
		float angularVel = fabs(max(angularVelocity.y, AI_MIN_ANGULARVEL) - AI_MIN_ANGULARVEL) * sign(angularVelocity.y);

		float counterSteeringScale = angularVel + m_initialHandling.steering;
		float brakingFac = 1.0f - m_initialHandling.braking;

		handling.autoHandbrake = m_initialHandling.autoHandbrake;
		
		if (fabs(counterSteeringScale)*brakingFac > AI_MAX_ANGULAR_VELOCITY_AUTOHANDBRAKE)
			handling.autoHandbrake = false;
		
		// calculate amounts of stability control
		handling.steering = sign(lateralSliding)*pow(fabs(lateralSliding)*AI_SLIDING_CORRECTION, AI_SLIDING_CURVE) +
			sign(angularVel)*pow(fabs(angularVel)*AI_ROTATION_CORRECTION, AI_ROTATION_CURVE);

		handling.steering = clamp(handling.steering, -AI_CORRECTION_LIMIT, AI_CORRECTION_LIMIT)*counterSteeringScale;

		float correctionFactor = RemapVal(carSpeedMPS, AI_SPEED_CORRECTION_MINSPEED, AI_SPEED_CORRECTION_MAXSPEED, AI_SPEED_CORRECTION_INITIAL, 1.0f);

		handling.steering *= correctionFactor;
		//handling.acceleration *= correctionFactor;

		Vector3D hintTargetDir = fastNormalize(m_hintTargetPosition - car->GetOrigin());

		// don't reduce acceleration if we're heading to hinted target position
		float accelReduceFactor = 1.0f - fabs(dot(car->GetForwardVector(), hintTargetDir));
		//accelReduceFactor = pow(accelReduceFactor, 2.5f);

		// compute based on steering
		float accelCompensation = fabs(handling.steering) * fabs(lateralSliding) * accelReduceFactor * AI_STEERING_SLIDING_ACCELERATOR_COMPENSATION_SCALE;

		handling.acceleration -= max(accelCompensation, AI_STEERING_SLIDING_ACCELERATOR_COMPENSATION_MIN) - AI_STEERING_SLIDING_ACCELERATOR_COMPENSATION_MIN;

		// too much compensation?
		if (handling.acceleration < 0.0f)
			handling.acceleration = 1.0f;
	}
	else
		handling.autoHandbrake = abs(m_initialHandling.steering) > AI_AUTOHANDBRAKE_STEERING_PERCENTAGE;
		
	// add to stability
	if (!car->IsAnyWheelOnGround())
	{
		m_inAirTime += fDt;
	}
	else
	{
		// if our car were in air, add some steering reducing
		if (m_inAirTime > AI_STABILITY_MIN_INAIR_TIME)
			m_landingCooldown = AI_STABILITY_LANDING_COOLDOWN;

		m_inAirTime = 0.0f;
	}
	
	m_landingCooldown -= fDt;

	if (m_landingCooldown < 0.0f)
		m_landingCooldown = 0.0f;

	float landingSteeringPercentage = 1.0f - (m_landingCooldown / AI_STABILITY_LANDING_COOLDOWN);

	handling.steering *= landingSteeringPercentage;
}