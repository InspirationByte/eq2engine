//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI target avoidance
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_TARGETAVOIDANCE_H
#define AIMANIPULATOR_TARGETAVOIDANCE_H

#include "AIHandling.h"

class CCar;

class CAITargetAvoidanceManipulator
{
public:
	CAITargetAvoidanceManipulator() {}

	void Setup(bool enabled, float radius, const Vector3D& targetPos, const Vector3D& targetVelocity);
	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	Vector3D			m_targetPosition;
	Vector3D			m_targetVelocity;
	float				m_avoidanceRadius;
	bool				m_enabled;
};

#endif // AIMANIPULATOR_COLLISIONAVOIDANCE_H
