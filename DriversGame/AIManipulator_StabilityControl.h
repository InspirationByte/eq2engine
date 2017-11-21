//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI stability control
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_STABILITYCONTROL_H
#define AIMANIPULATOR_STABILITYCONTROL_H

#include "AIHandling.h"
#include "level.h"

class CCar;

class CAIStabilityControlManipulator
{
public:
	CAIStabilityControlManipulator() {}

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	ai_handling_t m_initialHandling;
};

#endif // AIMANIPULATOR_STABILITYCONTROL_H