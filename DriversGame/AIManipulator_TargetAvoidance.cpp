//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI target avoidance
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_TargetAvoidance.h"
#include "car.h"

const float AI_DISTANCE_CURVE = 0.25f;

void CAITargetAvoidanceManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const float carSpeed = car->GetSpeed();
	const Vector3D& carPos = car->GetOrigin();

	handling.acceleration = 1.0f;
	handling.braking = 1.0f;
	handling.steering = 1.0f;
	handling.confidence	= 1.0f;

	if(!m_enabled)
	{
		return;
	}

	float distToTarget = length(carPos - m_targetPosition);

	if(distToTarget < m_avoidanceRadius)
	{
		float percentage = distToTarget / m_avoidanceRadius;
		handling.acceleration = pow(percentage, AI_DISTANCE_CURVE);
	}	
}