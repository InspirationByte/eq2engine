//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI collision avoidance
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_CollisionAvoidance.h"
#include "car.h"

ConVar ai_debug_collision_avoidance("ai_debug_collision_avoidance", "0");

const float AI_CORRECTION_LIMIT = 0.9f;

void CAICollisionAvoidanceManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const Vector3D& linearVelocity = car->GetVelocity();
	/*
	if(ai_debug_collision_avoidance.GetBool())
	{

	}*/



	handling.steering = clamp(handling.steering, -AI_CORRECTION_LIMIT, AI_CORRECTION_LIMIT);
}