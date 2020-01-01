//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI collision avoidance
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_COLLISIONAVOIDANCE_H
#define AIMANIPULATOR_COLLISIONAVOIDANCE_H

#include "AIHandling.h"

class CCar;

class CAICollisionAvoidanceManipulator
{
public:
	CAICollisionAvoidanceManipulator();

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	ai_handling_t	m_initialHandling;

	bool			m_enabled;
	bool			m_isColliding;
	bool			m_collidingPositionSet;
	Vector3D		m_lastCollidingPosition;

protected:
	float			m_blockingTime;
	float			m_timeToUnblock;
	float			m_cooldownTimer;
};

#endif // AIMANIPULATOR_COLLISIONAVOIDANCE_H
