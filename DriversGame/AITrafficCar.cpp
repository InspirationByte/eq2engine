//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: traffic car controller AI
//////////////////////////////////////////////////////////////////////////////////

#include "AITrafficCar.h"
#include "session_stuff.h"
#include "heightfield.h"
#include "IDebugOverlay.h"
#include "world.h"
#include "math/Volume.h"

#include "Shiny.h"

#pragma todo("Peer2peer network model for AI cars")
#pragma todo("Teach how to brake to the line strictly - enter 'required speed' parameter")

ConVar g_traffic_maxspeed("g_trafficMaxspeed", "50.0", NULL, CV_CHEAT); // FIXME: DO NOT USE!

ConVar ai_debug_freeze("ai_debug_freeze", "0", NULL, 0);


const float AICAR_THINK_TIME = 0.15f;
const float AI_TRAFFIC_MAX_DAMAGE = 2.0f;

//----------------------------------------------------------------------------------------------------------------

CAITrafficCar::CAITrafficCar( vehicleConfig_t* carConfig ) : CCar(carConfig), CFSM_Base()
{
	m_autohandbrake = false;
	m_autogearswitch = false;
	m_hasDamage = false;

	m_thinkTime = 0.0f;
	m_refreshTime = AICAR_THINK_TIME;

	m_traffic.m_manipulator.m_maxSpeed = g_traffic_maxspeed.GetFloat();

	SetMaxDamage(AI_TRAFFIC_MAX_DAMAGE);

	m_gearboxShiftThreshold = 0.6f;
}

CAITrafficCar::~CAITrafficCar()
{

}

void CAITrafficCar::InitAI( bool isParked )
{
	m_traffic.m_manipulator.m_speedModifier = g_replayRandom.Get(0,10);

	// helps kickstart the AI from script environment
	// before the level is loaded
	AI_SetState( &CAITrafficCar::InitTrafficState );
}

int CAITrafficCar::InitTrafficState( float fDt, EStateTransition transition )
{
	if(transition == STATE_TRANSITION_NONE)
	{
		UpdateTransform();

		Vector3D carPos = GetOrigin();

		// calc steering dir
		straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPos(carPos, 32);

		if(road.direction != -1)
		{
			road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.start);

			m_traffic.m_manipulator.ChangeRoad( road );

			if(IsEnabled() && GetPhysicsBody())
				SetVelocity(GetForwardVector() * g_traffic_maxspeed.GetFloat() * KPH_TO_MPS * 0.5f);

			AI_SetState( &CAITrafficCar::TrafficDrive );
		}
		else
			AI_SetState( &CAITrafficCar::SearchForRoad );
	}

	return 0;
}

void CAITrafficCar::Spawn()
{
	BaseClass::Spawn();

	// try this, after first collision you must change it
	//GetPhysicsBody()->SetCollideMask( COLLIDEMASK_VEHICLE );

	// perform lazy collision checks
	GetPhysicsBody()->SetMinFrameTime(1.0f / 30.0f);

	// also randomize lane switching
	//m_nextSwitchLaneTime = g_replayRandom.Get(0, AI_LANE_SWITCH_DELAY);
}

int CAITrafficCar::DeadState( float fDt, EStateTransition transition )
{
	m_hornSequencer.ShutUp();

	int buttons = GetControlButtons();
	buttons &= ~(IN_HORN | IN_ACCELERATE | IN_BRAKE);

	SetControlButtons(buttons);
	return 0;
}

ConVar g_disableTrafficCarThink("g_disableTrafficCarThink", "0", "Disables traffic car thinking", CV_CHEAT);

void CAITrafficCar::OnPrePhysicsFrame( float fDt )
{
	if(ai_debug_freeze.GetBool())
	{
		GetPhysicsBody()->m_flags |= BODY_FORCE_PRESERVEFORCES;
		GetPhysicsBody()->Freeze();
	}
	else
		GetPhysicsBody()->Wake();

	BaseClass::OnPrePhysicsFrame(fDt);

	PROFILE_BEGIN(AITrafficCar_Think);

	if( !g_disableTrafficCarThink.GetBool())
	{
		m_thinkTime -= fDt;

		if(m_thinkTime <= 0.0f)
		{
			if( IsAlive() )
				UpdateLightsState();

			int res = DoStatesAndEvents( m_refreshTime+fDt );
			m_thinkTime = m_refreshTime;
		}

		if(!m_traffic.m_manipulator.HasRoad())
		{
			MsgError("No road!\n");
			if(FSMGetCurrentState() == &CAITrafficCar::TrafficDrive)
			{
				AI_SetState( &CAITrafficCar::SearchForRoad );
			}
		}

		int controls = m_controlButtons;

		if (m_hornSequencer.Update(fDt))
			controls |= IN_HORN;
		else
			m_controlButtons &= ~IN_HORN;

		m_controlButtons = controls;
	}

	// frame skip. For cop think
	if( g_pGameSession->GetLeadCar() &&
		g_pGameSession->GetLeadCar() == g_pGameSession->GetPlayerCar() )
	{
		if(IsAlive())
			m_refreshTime = AICAR_THINK_TIME;
	}
	else if(!IsAlive())
		m_refreshTime = 0.0f;

	GetPhysicsBody()->UpdateBoundingBoxTransform();

	PROFILE_END();
}

void CAITrafficCar::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(FSMGetCurrentState() == &CAITrafficCar::DeadState)
		return;

	if(pair.impactVelocity > 2.0f && (pair.bodyA->m_flags & BODY_ISCAR))
	{
		// restore collision
		GetPhysicsBody()->SetCollideMask(COLLIDEMASK_VEHICLE);
		GetPhysicsBody()->SetMinFrameTime(0.0f);

		if(IsAlive())
			m_hornSequencer.SignalNoSequence(0.7f, 0.5f);
	}
}

int	CAITrafficCar::SearchForRoad(float fDt, EStateTransition transition)
{
	if(transition != STATE_TRANSITION_NONE)
		return 0;

	Vector3D carPos = GetOrigin();

	// calc steering dir
	straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPos(carPos, 32);

	if(road.direction != -1)
	{
		road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.start);

		m_traffic.m_manipulator.ChangeRoad( road );

		AI_SetState( &CAITrafficCar::TrafficDrive );
	}

	SetControlButtons( 0 );
	SetControlVars(1.0f, 1.0f, 1.0f);

	return 0;
}


int CAITrafficCar::TrafficDrive(float fDt, EStateTransition transition)
{
	if(!m_enabled)
		return 0;

	if(transition != EStateTransition::STATE_TRANSITION_NONE)
		return 0;

	// if this car was flipped, so put it to death state
	// but only if it's not a pursuer type
	if(!IsPursuer())
	{
		if( IsFlippedOver() || !IsAlive() )
		{
			AI_SetState( &CAITrafficCar::DeadState );
			m_hornSequencer.ShutUp();
		}
	}

	// manipulator code
	m_traffic.Update(this, fDt);

	if (m_traffic.m_manipulator.m_triggerHorn > 0)
	{
		if (m_traffic.m_manipulator.m_triggerHornDuration > 0)
			m_hornSequencer.SignalNoSequence(m_traffic.m_manipulator.m_triggerHornDuration, m_traffic.m_manipulator.m_triggerHorn);
		else
			m_hornSequencer.SignalRandomSequence(m_traffic.m_manipulator.m_triggerHorn);

		m_traffic.m_manipulator.m_triggerHorn = -1.0f;
		m_traffic.m_manipulator.m_triggerHornDuration = -1.0f;
	}

	ai_handling_t handling = m_traffic.m_handling;

	int controls = IN_ACCELERATE | IN_EXTENDTURN | IN_BRAKE;

	if (m_traffic.m_manipulator.m_emergencyEscape)
		controls |= IN_ANALOGSTEER;
	else
		controls |= IN_TURNRIGHT;

	SetControlButtons(controls);
	SetControlVars(handling.acceleration,
		handling.braking,
		handling.steering);

	return 0;
}



OOLUA_EXPORT_FUNCTIONS(
	CAITrafficCar,
	InitAI
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CAITrafficCar,
	IsPursuer
)
