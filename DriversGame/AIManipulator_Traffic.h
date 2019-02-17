//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI traffic driver
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_TRAFFIC_H
#define AIMANIPULATOR_TRAFFIC_H

#include "AIHandling.h"
#include "EventFSM.h"
#include "road_defs.h"

// junction details - holds already found roads. 
// Using this AI selects straight and turns on repeater indicator
struct junctionDetail_t
{
	junctionDetail_t()
	{
		selectedExit = 0;
	}

	DkList<straight_t>	exits;
	int					selectedExit;
	int					size;
	bool				availDirs[4];
};

class CCar;

class CAITrafficManipulator
{
public:
	CAITrafficManipulator();

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	void				DebugDraw(float fDt);

	bool				HasRoad();

//protected:
	// task
	void				SearchJunctionAndStraight();
	
	void				ChangeRoad(const straight_t& road);
	void				RoadProvision();
	void				HandleJunctionExit(CCar* car);

	void				ChangeLanes(CCar* car);

	//------------------------------------------------

	bool				m_provisionCompleted;

	float				m_triggerHorn;
	float				m_triggerHornDuration;

	straight_t			m_straights[2];
	IVector2D			m_currEnd;

	junctionDetail_t	m_junction;

	bool				m_changingLane;
	bool				m_changingDirection;

	float				m_prevFract;

	float				m_maxSpeed;
	float				m_speedModifier;
	
	float				m_nextSwitchLaneTime;

	bool				m_emergencyEscape;
	float				m_emergencyEscapeTime;
	float				m_emergencyEscapeSteer;


};

#endif // AIMANIPULATOR_TRAFFIC_H
