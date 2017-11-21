//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI collision avoidance
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_CollisionAvoidance.h"
#include "car.h"

ConVar ai_debug_collision_avoidance("ai_debug_collision_avoidance", "0");

const float AI_COP_BLOCK_DELAY						= 1.5f;
const float AI_COP_BLOCK_REALIZE_FRONTAL_TIME		= 0.8f;
const float AI_COP_BLOCK_REALIZE_COLLISION_TIME		= 0.2f;
const float AI_COP_BLOCK_MAX_SPEED					= 2.0f;	// meters per second
const float AI_COP_BLOCK_DISTANCE_FROM_COLLISION	= 5.0f;

CAICollisionAvoidanceManipulator::CAICollisionAvoidanceManipulator()
{
	m_isColliding = false;
	m_blockTimeout = 0.0f;
	m_blockingTime = 0.0f;
	m_enabled = false;
}

void CAICollisionAvoidanceManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const Vector3D& linearVelocity = car->GetVelocity();
	Vector3D carBodySize = car->m_conf->physics.body_size;
	Vector3D carForwardDir = car->GetForwardVector();

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;

	m_enabled = false;

	float frontCollDist = clamp(speedMPS, 1.0f, 8.0f);

	btBoxShape carBoxShape(btVector3(carBodySize.x, carBodySize.y, 0.25f));

	CollisionData_t frontColl;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject( car->GetPhysicsBody() );

	int traceContents = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE;

	// trace in the front direction
	g_pPhysics->TestConvexSweep(&carBoxShape, car->GetOrientation(),
		car->GetOrigin(), car->GetOrigin()+carForwardDir*frontCollDist, frontColl,
		traceContents, &collFilter);

	bool frontBlocked = frontColl.fract < 1.0f;

	if (m_blockTimeout <= 0.0f && (frontBlocked || m_isColliding) && speedMPS < AI_COP_BLOCK_MAX_SPEED)
	{
		m_blockingTime += fDt;

		if( m_isColliding && m_blockingTime > AI_COP_BLOCK_REALIZE_COLLISION_TIME ||
			frontBlocked && m_blockingTime > AI_COP_BLOCK_REALIZE_FRONTAL_TIME)
		{
			m_blockTimeout = AI_COP_BLOCK_DELAY;
			m_blockingTime = 0.0f;
		}
	}
	else
	{
		FReal distFromCollPoint = length(m_lastCollidingPosition-car->GetOrigin());

		if(distFromCollPoint > AI_COP_BLOCK_DISTANCE_FROM_COLLISION)
			m_blockTimeout = 0.0f;

		m_blockTimeout -= fDt;

		if(m_blockTimeout > 0.0f || frontColl.fract < 1.0f && speedMPS < AI_COP_BLOCK_MAX_SPEED)
		{
			handling.braking = 1.0f;
			handling.acceleration = 0.0f;
			handling.steering = m_initialHandling.steering*-1.0f;

			m_enabled = true;
		}
	}
}