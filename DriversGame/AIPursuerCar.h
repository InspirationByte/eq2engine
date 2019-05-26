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

	PURSUER_TYPE_COUNT,
};

const float AI_COPVIEW_FAR					= 60.0f;
const float AI_COPVIEW_FAR_WANTED			= 100.0f;
const float AI_COPVIEW_FAR_HASSTRAIGHTPATH	= 40.0f;

class CAIPursuerCar : public CAITrafficCar
{
public:

	DECLARE_CLASS(CAIPursuerCar, CAITrafficCar);

	CAIPursuerCar();

	CAIPursuerCar(vehicleConfig_t* carConfig, EPursuerAIType type);
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

	EPursuerAIType		GetPursuerType() const {return m_type;}

	void				SetMaxSpeed(float fSpeed);
	void				SetTorqueScale(float fScale);

protected:

	int					PassiveCopState( float fDt, EStateTransition transition );

	virtual void		OnPrePhysicsFrame( float fDt );
	virtual void		OnPhysicsFrame( float fDt );

	void				UpdateInfractions(CCar* checkCar, bool passive);

	EInfractionType		CheckTrafficInfraction( CCar* car, bool checkFelony = true, bool checkSpeeding = true );
	

	bool				Speak( const char* soundName, CCar* target, bool force = false );

	void				DoPoliceLoudhailer();
	bool				UpdateTarget(float fDt);

	void				SpeakTargetDirection(const char* startSoundName, bool force = false);

	// states
	int					TrafficDrive( float fDt, EStateTransition transition );
	int					DeadState( float fDt, EStateTransition transition );
	int					PursueTarget( float fDt, EStateTransition transition );

	//-----------------------------------------------------------------------------------

	EPursuerAIType			m_type;

	CCar*					m_target;
	float					m_lastSeenTargetTimer;

	float					m_alterSirenChangeTime;
	bool					m_sirenAltered;

	float					m_savedMaxSpeed;
	float					m_savedTorqueScale;

	bool					m_angry;
	float					m_angryTimer;
	float					m_pursuitTime;

	// all handling affectors, they are biased by the current behavior state
	CAIHandlingAffector<CAINavigationManipulator>			m_nav;
	CAIHandlingAffector<CAITargetChaserManipulator>			m_chaser;
	CAIHandlingAffector<CAIStabilityControlManipulator>		m_stability;
	CAIHandlingAffector<CAICollisionAvoidanceManipulator>	m_collAvoidance;
	CAIHandlingAffector<CAITargetAvoidanceManipulator>		m_targetAvoidance;

	ISoundController*		m_loudhailer;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAIPursuerCar, CAITrafficCar)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(SetPursuitTarget)
	OOLUA_MFUNC(CheckObjectVisibility)
	OOLUA_MFUNC(BeginPursuit)
	OOLUA_MFUNC(EndPursuit)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // AIPURSUERCARCONTROLLER_H
