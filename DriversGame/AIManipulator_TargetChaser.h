//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI chaser manipulator
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_TARGETCHASER_H
#define AIMANIPULATOR_TARGETCHASER_H

#include "AIHandling.h"
#include "road_defs.h"

class CCar;
class CEqCollisionObject;

class CAITargetChaserManipulator
{
public:
	CAITargetChaserManipulator();

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	// the position driver needs reach to
	Vector3D			m_driveTarget;
	Vector3D			m_driveTargetVelocity;
	CEqCollisionObject*	m_excludeColl;
};

#endif // AIMANIPULATOR_TARGETCHASER_H