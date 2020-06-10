//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: traffic car controller AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef AITRAFFICCARCONTROLLER_H
#define AITRAFFICCARCONTROLLER_H

#include "car.h"
#include "level.h"
#include "EventFSM.h"

#include "utils/DkList.h"

#include "AIHandling.h"

#include "AIManipulator_Navigation.h"
#include "AIManipulator_TargetChaser.h"
#include "AIManipulator_StabilityControl.h"
#include "AIManipulator_CollisionAvoidance.h"
#include "AIManipulator_TargetAvoidance.h"
#include "AIManipulator_Traffic.h"

#include "AIHornSequencer.h"

#define AI_TRACE_CONTENTS (OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE)

//-----------------------------------------------------------------------------------------------

class CAITrafficCar :	public CFSM_Base,
						public CCar
{
public:
	DECLARE_CLASS(CAITrafficCar, CCar);

	CAITrafficCar( vehicleConfig_t* carConfig );
	~CAITrafficCar();

	virtual void		InitAI( bool isParked );
	virtual void		Precache();
	virtual void		OnRemove();

	virtual void		Spawn();
	virtual void		OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);
	
	int					ObjType() const { return GO_CAR_AI; }
	virtual bool		IsPursuer() const {return false;}

protected:
	virtual void		OnPrePhysicsFrame( float fDt );

	// states
	int					SearchForRoad( float fDt, EStateTransition transition );
	virtual int			TrafficDrive( float fDt, EStateTransition transition );
	int					InitTrafficState( float fDt, EStateTransition transition );

	virtual int			DeadState( float fDt, EStateTransition transition );

	CAIHandlingAffector<CAITrafficManipulator>	m_traffic;
	CAIHornSequencer	m_hornSequencer;
	ISoundController*	m_music;

	float				m_thinkTime;
	float				m_refreshTime;

	
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAITrafficCar, CCar)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(InitAI)
	OOLUA_MFUNC_CONST(IsPursuer)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // AITRAFFICCONTROLLER_H