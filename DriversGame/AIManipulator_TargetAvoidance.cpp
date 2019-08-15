//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI target avoidance
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_TargetAvoidance.h"
#include "car.h"

const float AI_DISTANCE_CURVE = 1.0f;
const float AI_DISTANCE_SPEED_SCALE = 0.5f;
const float AI_BRAKEDIST_SCALE = 0.35f;

void CAITargetAvoidanceManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const float carSpeed = car->GetSpeed()*KPH_TO_MPS;
	const Vector3D& carPos = car->GetOrigin();

	handling.acceleration = 0.0f;
	handling.braking = 0.0f;
	handling.steering = 0.0f;
	handling.confidence	= 1.0f;

	if(!m_enabled)
	{
		return;
	}

	float brakeDistancePerSec = car->m_conf->GetBrakeEffectPerSecond() * AI_BRAKEDIST_SCALE;
	float brakeToStopTime = carSpeed / brakeDistancePerSec * 2.0f;
	float brakeDistAtCurSpeed = brakeDistancePerSec * brakeToStopTime;

	float distToTarget = length(carPos - (m_targetPosition + m_targetVelocity * 0.7f));

	float avoidanceDistWithSpeedModifier = m_avoidanceRadius + brakeDistAtCurSpeed;//fabs(carSpeed * AI_DISTANCE_SPEED_SCALE);

	if(distToTarget < avoidanceDistWithSpeedModifier)
	{
		float percentage = distToTarget / avoidanceDistWithSpeedModifier;

		float perc = pow(percentage, AI_DISTANCE_CURVE);

		handling.acceleration = clamp(1.0f - perc, 0.0f, 1.0f);
		handling.braking = clamp(1.0f-perc, 0.0f, 1.0f) * min(carSpeed, 1.0f);
	}	
}