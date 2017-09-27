//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base AI driver, affectors, manipulators
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIBASEDRIVER_H
#define AIBASEDRIVER_H

#include "utils/DkList.h"
#include "EventFSM.h"

#include "AIManipulator_Navigation.h"
#include "AIManipulator_ObstacleAvoidance.h"
#include "AIManipulator_Chasing.h"

class CCar;
class CAIBaseDriver;

struct ai_handling_t
{
	float		steering;
	float		acceleration;
	float		braking;

	bool		handbrake;
	bool		autoHandbrake;

	float		confidence;		// 0..1
};

//
// TYPE is a CAIBaseManipulator-derived
//
template <class TYPE>
class CAIHandlingAffector
{
public:
	CAIHandlingAffector<TYPE>()
	{
		Reset();
	}

	virtual ~CAIHandlingAffector<TYPE>() {}

	void Reset()
	{
		m_handling.steering = 0.0f;
		m_handling.acceleration = 0.0f;
		m_handling.braking = 0.0f;

		m_handling.handbrake = false;
		m_handling.autoHandbrake = false;

		m_handling.confidence = 0.0f;
	}

	void Update(CAIBaseDriver* driver, float fDt)
	{
		m_manipulator.UpdateAffector(m_handling, driver, fDt);
	}

	TYPE			m_manipulator;
	ai_handling_t	m_handling;
};

//
// AI driver conditions
//

enum EDriverConditionFlags
{
	DRIVER_COND_ISSTUCK			= (1 << 0),
	DRIVER_COND_NO_NAVIGATION	= (1 << 1),
	DRIVER_COND_ANGRY			= (1 << 2),
	DRIVER_COND_SCARED			= (1 << 3),
};

class CAINavigationManipulator;
class CAIChasingManipulator;
class CAIObstacleAvoidanceManipulator;

//
// Base AI driver class
//
class CAIBaseDriver : CFSM_Base
{
public:
	CAIBaseDriver(CCar* car)  : m_vehicle(car), m_condition(0) {}
	virtual ~CAIBaseDriver() {}

	virtual void Update(float fDt) = 0;

protected:
	CCar*		m_vehicle;

	int			m_condition;		// EDriverConditionFlags
	
	// all handling affectors, they are biased by the current behavior state
	CAIHandlingAffector<CAINavigationManipulator>			m_navAffector;
	CAIHandlingAffector<CAIChasingManipulator>				m_chaseAffector;
	CAIHandlingAffector<CAIObstacleAvoidanceManipulator>	m_obstacleAffector;
};

#endif //AIBASEDRIVER_H