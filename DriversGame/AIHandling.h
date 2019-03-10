//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base AI driver, affectors, manipulators
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIBASEDRIVER_H
#define AIBASEDRIVER_H

#include "utils/DkList.h"
#include "math/Vector.h"
#include "EventFSM.h"
#include "worldenv.h"

struct ai_handling_t
{
	float		steering;
	float		acceleration;
	float		braking;

	bool		handbrake;
	bool		autoHandbrake;

	float		confidence;		// 0..1

	void operator += (const ai_handling_t& v)
	{
		steering += v.steering;
		acceleration += v.acceleration;
		braking += v.braking;

		confidence += v.confidence;
	}

	void operator -= (const ai_handling_t& v)
	{
		steering -= v.steering;
		acceleration -= v.acceleration;
		braking -= v.braking;

		confidence -= v.confidence;
	}

	void operator *= (const ai_handling_t& v)
	{
		steering *= v.steering;
		acceleration *= v.acceleration;
		braking *= v.braking;

		confidence *= v.confidence;
	}
};

class CCar;
//class CAIBaseDriver;

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

	void Update(CCar* car, float fDt)
	{
		m_manipulator.UpdateAffector(m_handling, car, fDt);
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

// asphalt: 0.24	- 0.76
// grass:	0.15	- 0.85
// dirt:	0.20	- 0.80

static const float s_weatherBrakeDistanceModifier[WEATHER_COUNT] =	// * (1.0f - tirefriction)
{
	0.65f,	// 0.50f,
	0.68f,	// 0.57f,
	0.71f	// 0.59f,
};

static const float s_weatherBrakeCurve[WEATHER_COUNT] =	// * tirefriction
{
	4.20f,	// 1.00f,
	4.00f,	// 0.95f,
	3.80f	// 0.79f,
};

/*
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
	CAIHandlingAffector<CAINavigationManipulator>			m_nav;
	CAIHandlingAffector<CAIChasingManipulator>				m_chaseAffector;
	CAIHandlingAffector<CAIObstacleAvoidanceManipulator>	m_obstacleAffector;
};*/

#endif //AIBASEDRIVER_H
