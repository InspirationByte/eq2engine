//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI collision avoidance
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_COLLISIONAVOIDANCE_H
#define AIMANIPULATOR_COLLISIONAVOIDANCE_H

#include "AIHandling.h"
#include "level.h"

class CCar;

class CAICollisionAvoidanceManipulator
{
public:
	CAICollisionAvoidanceManipulator() {}

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);
};

#endif // AIMANIPULATOR_COLLISIONAVOIDANCE_H
