//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI stability control
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_StabilityControl.h"
#include "car.h"

ConVar ai_debug_stability("ai_debug_stability", "0");

void CAIStabilityControlManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const float lateralSliding = car->GetLateralSlidingAtWheels(true);

	const Vector3D& linearVelocity = car->GetVelocity();
	const Vector3D& angularVelocity = car->GetPhysicsBody()->GetAngularVelocity();

	if(ai_debug_stability.GetBool())
	{
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "lateral sliding: %g", lateralSliding);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "angular rot: %g", angularVelocity.y);
	}

	const float AI_SLIDING_CORRECTION = 0.1f;
	const float AI_SLIDING_CURVE = 2.5f;

	const float AI_ROTATION_CORRECTION = 0.2f;
	const float AI_ROTATION_CURVE = 1.5f;

	const float AI_CORRECTION_LIMIT = 0.5f;

	const float AI_MAX_ANGULAR_VELOCITY_AUTOHANDBRAKE = 2.0f;

	if(fabs(angularVelocity.y) > AI_MAX_ANGULAR_VELOCITY_AUTOHANDBRAKE)
		handling.autoHandbrake = false;

	// calculate amounts of stability control
	handling.steering = sign(lateralSliding)*pow(fabs(lateralSliding)*AI_SLIDING_CORRECTION, AI_SLIDING_CURVE) + 
						sign(angularVelocity.y)*pow(fabs(angularVelocity.y)*AI_ROTATION_CORRECTION, AI_ROTATION_CURVE);

	handling.steering = clamp(handling.steering, -AI_CORRECTION_LIMIT, AI_CORRECTION_LIMIT);
}