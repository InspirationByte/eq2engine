//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI target avoidance
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_TARGETAVOIDANCE_H
#define AIMANIPULATOR_TARGETAVOIDANCE_H

#include "AIHandling.h"
#include "level.h"

class CCar;

class CAITargetAvoidanceManipulator
{
public:
	CAITargetAvoidanceManipulator() {}

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	Vector3D			m_targetPosition;
	float				m_avoidanceRadius;
	bool				m_enabled;
};

#endif // AIMANIPULATOR_COLLISIONAVOIDANCE_H
