//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI collision avoidance
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_CollisionAvoidance.h"
#include "car.h"

ConVar ai_debug_collision_avoidance("ai_debug_collision_avoidance", "0");

const float AI_COP_BLOCK_DELAY						= 1.0f;
const float AI_COP_BLOCK_COOLDOWN					= 1.0f;
const float AI_COP_BLOCK_REALIZE_FRONTAL_TIME		= 0.8f;
const float AI_COP_BLOCK_REALIZE_COLLISION_TIME		= 0.3f;
const float AI_COP_BLOCK_MAX_SPEED					= 4.0f;	// meters per second
const float AI_COP_BLOCK_DISTANCE_FROM_COLLISION	= 1.5f;

CAICollisionAvoidanceManipulator::CAICollisionAvoidanceManipulator()
{
	m_isColliding = false;
	m_collidingPositionSet = false;
	m_timeToUnblock = 0.0f;
	m_blockingTime = 0.0f;
	m_cooldownTimer = AI_COP_BLOCK_COOLDOWN;
	m_enabled = false;
}

void CAICollisionAvoidanceManipulator::Trigger(const CollisionPairData_t& pair)
{
	m_isColliding = true;

	if (m_enabled && !m_collidingPositionSet)
	{
		m_lastCollidingPosition = pair.position;
		m_collidingPositionSet = true;
	}
	else
	{
		m_collidingPositionSet = false;
	}
}

void CAICollisionAvoidanceManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const Vector3D& linearVelocity = car->GetVelocity();
	Vector3D carBodySize = car->m_conf->physics.body_size;
	Vector3D carForwardDir = car->GetForwardVector();

	float speedMPS = car->GetSpeedWheels()*KPH_TO_MPS;

	const float frontCollDist = 0.1f;

	btBoxShape carBoxShape(btVector3(carBodySize.x, carBodySize.y, 0.15f));

	CollisionData_t frontColl;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(car->GetPhysicsBody());

	int traceContents = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE;

	Vector3D frontColliderPos = car->GetOrigin() + carForwardDir * carBodySize.z;

	// trace in the front direction
	g_pPhysics->TestConvexSweep(&carBoxShape, car->GetOrientation(),
		frontColliderPos, frontColliderPos + carForwardDir * frontCollDist, frontColl,
		traceContents, &collFilter);

	if (m_enabled)
	{
		handling.braking = 1.0f;
		handling.acceleration = 0.0f;
		handling.steering = m_initialHandling.steering*-1.0f;

		m_timeToUnblock -= fDt;

		float distFromCollPoint = length(m_lastCollidingPosition - frontColliderPos);

		if (m_timeToUnblock <= 0.0f || m_collidingPositionSet && distFromCollPoint > AI_COP_BLOCK_DISTANCE_FROM_COLLISION)
		{
			m_cooldownTimer = AI_COP_BLOCK_COOLDOWN;
			m_collidingPositionSet = false;
			m_enabled = false;
			m_isColliding = false;
		}
	}
	else if((m_cooldownTimer -= fDt) < 0.0f)
	{
		if (m_isColliding && m_collidingPositionSet)
		{
			Plane pl(carForwardDir, -dot(carForwardDir, car->GetOrigin()));

			if (pl.Distance(m_lastCollidingPosition) < 0.0f)
			{
				m_collidingPositionSet = false;
				m_isColliding = false;
			}
		}


		bool frontBlock = (frontColl.fract < 1.0f);
		if ((m_isColliding || frontBlock) && speedMPS < AI_COP_BLOCK_MAX_SPEED)
		{
			m_blockingTime += fDt;

			if (m_isColliding && m_blockingTime > AI_COP_BLOCK_REALIZE_COLLISION_TIME ||
				frontBlock && m_blockingTime > AI_COP_BLOCK_REALIZE_FRONTAL_TIME)
			{
				m_timeToUnblock = AI_COP_BLOCK_DELAY;
				m_enabled = true;
			}
		}
	}
}