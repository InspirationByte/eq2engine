//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: pursuer car controller AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIPURSUERCARCONTROLLER_H
#define AIPURSUERCARCONTROLLER_H

#include "AITrafficCar.h"

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

	INFRACTION_MINOR,			// minor infraction ratio

	INFRACTION_SPEEDING,		// speed over 65 mph
	INFRACTION_RED_LIGHT,		// he crossed red light
	INFRACTION_WRONG_LANE,		// he drives opposite lane
	INFRACTION_HIT_MINOR,		// hit the debris
	INFRACTION_HIT,				// hit the object or world
	INFRACTION_HIT_VEHICLE,		// hit the vehicle
};

const float AI_COPVIEW_FAR			= 60.0f;
const float AI_COPVIEW_FAR_WANTED	= 100.0f;

class CAIPursuerCar : public CAITrafficCar
{
public:

	DECLARE_CLASS(CAIPursuerCar, CAITrafficCar);

	CAIPursuerCar();

	CAIPursuerCar(carConfigEntry_t* carConfig, EPursuerAIType type);
	~CAIPursuerCar();

	virtual void		Spawn();
	void				Precache();
	void				OnRemove();

	virtual void		OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	bool				IsPursuer() const { return true; }
	bool				InPursuit() const;

	void				SetPursuitTarget(CCar* obj);
	void				BeginPursuit( float delay = 0.25f );
	void				EndPursuit( bool death );

	bool				CheckObjectVisibility(CCar* obj);

protected:

	int					PassiveCopState( float fDt, EStateTransition transition );

	virtual void		OnPrePhysicsFrame( float fDt );
	virtual void		OnPhysicsFrame( float fDt );

	EInfractionType		CheckTrafficInfraction( CCar* car, bool checkFelony = true, bool checkSpeeding = true );

	bool				Speak( const char* soundName, bool force = false );
	void				TrySayTaunt();

	Vector3D			GetAdvancedPointByDist(int& startSeg, float distFromSegment);

	// states
	int					TrafficDrive( float fDt, EStateTransition transition );
	int					DeadState( float fDt, EStateTransition transition );
	int					PursueTarget( float fDt, EStateTransition transition );

	//-----------------------------------------------------------------------------------

	EPursuerAIType			m_type;

	struct pursuitTargetInfo_t
	{
		CCar*				target;
		int					direction;	// swne
		float				notSeeingTime;

		pathFindResult_t	path;
		int					pathTargetIdx;

		float				nextPathUpdateTime;
		float				nextCheckImpactTime;
		int					lastInfraction;
		bool				isAngry;
	};

	pursuitTargetInfo_t		m_targInfo;

	float					m_deathTime;
	float					m_blockingTime;

	bool					m_isColliding;
	Vector3D				m_lastCollidingPosition;

	ISoundController*		m_taunts;
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