//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: pursuer car controller AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIPURSUERCARCONTROLLER_H
#define AIPURSUERCARCONTROLLER_H

#include "AITrafficCarController.h"

enum EPursuerAIType
{
	PURSUER_TYPE_COP	= 0,	// he looks on your felony rate
	PURSUER_TYPE_GANG,			// he looks only for your car
};

// There are some traffic infraction types that cop can recognize
enum EInfractionType
{
	INFRACTION_NONE = 0,

	INFRACTION_HAS_FELONY,		// this car has a felony

	INFRACTION_SPEEDING,		// speed over 65 mph
	INFRACTION_RED_LIGHT,		// he crossed red light
	INFRACTION_WRONG_LANE,		// he drives opposite lane
	INFRACTION_HIT_MINOR,		// hit the debris
	INFRACTION_HIT,				// hit the object or world
	INFRACTION_HIT_VEHICLE,		// hit the vehicle
};

class CAIPursuerCar : public CAITrafficCar
{
public:

	DECLARE_CLASS(CAIPursuerCar, CAITrafficCar);

	CAIPursuerCar();

	CAIPursuerCar(carConfigEntry_t* carConfig, EPursuerAIType type);
	~CAIPursuerCar();

	virtual void		InitAI(CLevelRegion* reg, levroadcell_t* cell);

	virtual void		Spawn();
	void				Precache();
	void				OnRemove();

	virtual void		OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	virtual void		OnPrePhysicsFrame( float fDt );
	virtual void		OnPhysicsFrame( float fDt );

	int					TrafficDrive( float fDt );
	int					DeadState( float fDt );

	int					PursueTarget( float fDt );

	bool				CheckObjectVisibility(CCar* obj);
	void				SetPursuitTarget(CCar* obj);

	EInfractionType		CheckTrafficInfraction( CCar* car, bool checkFelony = true, bool checkSpeeding = true );

	void				BeginPursuit();
	void				EndPursuit( bool death );

	bool				IsPursuer() const { return true; }

	bool				Speak( const char* soundName, bool force = false );
	void				TrySayTaunt();

	Vector3D			GetAdvancedPointByDist(int& startSeg, float distFromSegment);

protected:

	CCar*					m_pursuitTarget;
	int						m_targetDirection;	// swne

	float					m_deathTime;
	FReal					m_blockingTime;

	bool					m_isColliding;
	Vector3D				m_lastCollidingPosition;

	EPursuerAIType			m_type;

	float					m_notSeeingTime;

	float					m_nextCheckImpactTime;

	ISoundController*		m_taunts;

	float					m_nextPathUpdateTime;

	pathFindResult_t		m_path;
	int						m_pathTargetIdx;

	int						m_targetLastInfraction;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAIPursuerCar, CCar)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(SetPursuitTarget)
	OOLUA_MFUNC(CheckObjectVisibility)
	OOLUA_MFUNC(BeginPursuit)
	OOLUA_MFUNC(EndPursuit)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // AIPURSUERCARCONTROLLER_H