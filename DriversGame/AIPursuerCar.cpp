//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: pursuer car controller AI
//////////////////////////////////////////////////////////////////////////////////

#include "AIPursuerCar.h"
#include "session_stuff.h"

#include "object_debris.h"

#include "AICarManager.h"

ConVar ai_debug_pursuer("ai_debug_pursuer", "0", NULL, CV_CHEAT);
ConVar g_railroadCops("g_railroadCops", "0", NULL, CV_CHEAT);

// TODO: make these constants initialized from Lua

#define DOVERLAY_DELAY (0.15f)

const float AI_COP_BEGINPURSUIT_ARMED_DELAY		= 0.25f;
const float AI_COP_BEGINPURSUIT_PASSIVE_DELAY	= 1.0f;

const float AI_COPVIEW_FOV			= 85.0f;
const float AI_COPVIEW_FOV_WANTED	= 90.0f;

const float AI_COPVIEW_FAR_ROADBLOCK	= 10.0f;

const float AI_COPVIEW_RADIUS			= 18.0f;
const float AI_COPVIEW_RADIUS_WANTED	= 25.0f;
const float AI_COPVIEW_RADIUS_PURSUIT	= 120.0f;
const float AI_COPVIEW_RADIUS_ROADBLOCK = 15.0f;

const float AI_COP_CHECK_MAXSPEED = 80.0f; // 80 kph and you will be pursued

const float AI_COP_MINFELONY			= 0.1f;

const float AI_COP_COLLISION_FELONY			= 0.06f;
const float AI_COP_COLLISION_FELONY_VEHICLE	= 0.1f;
const float AI_COP_COLLISION_FELONY_DEBRIS	= 0.02f;
const float AI_COP_COLLISION_FELONY_REDLIGHT = 0.005f;

const float AI_COP_COLLISION_CHECKTIME		= 0.01f;

const float AI_COP_TIME_TO_LOST_TARGET		= 30.0f;

const float AI_ALTER_SIREN_MIN_SPEED		= 65.0f;
const float AI_ALTER_SIREN_CHANGETIME		= 2.0f;

//------------------------------------------------------------------------------------------------

IVector2D GetDirectionVec(int dirIdx);

//------------------------------------------------------------------------------------------------

CAIPursuerCar::CAIPursuerCar() : CAITrafficCar(NULL)
{
	memset(&m_targInfo, 0, sizeof(m_targInfo));
}

CAIPursuerCar::CAIPursuerCar(vehicleConfig_t* carConfig, EPursuerAIType type) : CAITrafficCar(carConfig)
{
	memset(&m_targInfo, 0, sizeof(m_targInfo));

	m_loudhailer = NULL;
	m_type = type;

	m_alterSirenChangeTime = AI_ALTER_SIREN_CHANGETIME;
	m_sirenAltered = false;
}

CAIPursuerCar::~CAIPursuerCar()
{

}

void CAIPursuerCar::Spawn()
{
	BaseClass::Spawn();

	if(m_type == PURSUER_TYPE_COP)
	{
		EmitSound_t ep;
		ep.name = "cop.taunt";
		ep.pObject = this;
		m_loudhailer = ses->CreateSoundController(&ep);
	}

	SetMaxDamage( g_pAIManager->GetCopMaxDamage() );
}

void CAIPursuerCar::OnRemove()
{
	EndPursuit(true);

	if (m_loudhailer)
		ses->RemoveSoundController(m_loudhailer);

	m_loudhailer = NULL;

	BaseClass::OnRemove();
}

void CAIPursuerCar::Precache()
{
	PrecacheScriptSound("cop.pursuit");
	PrecacheScriptSound("cop.pursuit_continue");
	PrecacheScriptSound("cop.lost");
	PrecacheScriptSound("cop.backup");
	PrecacheScriptSound("cop.roadblock");
	PrecacheScriptSound("cop.redlight");
	PrecacheScriptSound("cop.hitvehicle");
	PrecacheScriptSound("cop.heading");
	PrecacheScriptSound("cop.heading_west");
	PrecacheScriptSound("cop.heading_east");
	PrecacheScriptSound("cop.heading_south");
	PrecacheScriptSound("cop.heading_north");
	PrecacheScriptSound("cop.taunt");

	BaseClass::Precache();
}

void CAIPursuerCar::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(!IsAlive())
		return;

	if(pair.impactVelocity > 2.0f && (pair.bodyB->m_flags & BODY_ISCAR))
	{
		// restore collision
		GetPhysicsBody()->SetCollideMask(COLLIDEMASK_VEHICLE);
		GetPhysicsBody()->SetMinFrameTime(0.0f);

		//m_hornTime.Set(0.7f, 0.5f);

		m_hasDamage = true;

		CCar* playerCar = g_pGameSession->GetPlayerCar();

		if (m_targInfo.target == NULL &&
			(pair.impactVelocity > 1.0f) && hitBy == playerCar)
		{
			SetPursuitTarget(playerCar);
			BeginPursuit( AI_COP_BEGINPURSUIT_PASSIVE_DELAY );
		}
	}
}

bool CAIPursuerCar::Speak(const char* soundName, bool force)
{
	if (m_type == PURSUER_TYPE_GANG)
		return false;

	return g_pAIManager->MakeCopSpeech(soundName, force);
}

void CAIPursuerCar::TrySayTaunt()
{
	if (m_type == PURSUER_TYPE_GANG)
		return;

	if (g_pAIManager->IsCopCanSayTaunt())
	{
		m_loudhailer->Stop();
		m_loudhailer->Play();

		g_pAIManager->GotCopTaunt();
	}
}

void CAIPursuerCar::OnPrePhysicsFrame( float fDt )
{
	BaseClass::OnPrePhysicsFrame(fDt);

	m_targInfo.nextCheckImpactTime -= fDt;

	CCar* playerCar = g_pGameSession->GetPlayerCar();
	if(	IsAlive()&&
		!m_enabled &&
		CheckObjectVisibility(playerCar) &&
		playerCar->GetFelony() > AI_COP_MINFELONY &&
		playerCar->GetPursuedCount() == 0)
	{
		SetPursuitTarget(playerCar);
		m_targInfo.target->IncrementPursue();
	}
}

void CAIPursuerCar::OnPhysicsFrame( float fDt )
{
	BaseClass::OnPhysicsFrame(fDt);

	if(IsAlive())
	{
		// update blocking
		m_collAvoidance.m_manipulator.m_isColliding = GetPhysicsBody()->m_collisionList.numElem() > 0;

		if(m_collAvoidance.m_manipulator.m_isColliding)
			m_collAvoidance.m_manipulator.m_lastCollidingPosition = GetPhysicsBody()->m_collisionList[0].position;

		if(!g_pGameWorld->IsValidObject(m_targInfo.target))
			m_targInfo.target = NULL;

		// update target infraction
		if(m_targInfo.target)
		{
			int infraction = CheckTrafficInfraction(m_targInfo.target, false,false);

			if(infraction > INFRACTION_HAS_FELONY)
				m_targInfo.lastInfraction = infraction;
		}
		else // do passive state
		{
			PassiveCopState( fDt, STATE_TRANSITION_NONE );
		}
	}
	else
	{
		// death by water?
		if(m_inWater)
			EndPursuit(true);
	}

}

int	CAIPursuerCar::TrafficDrive( float fDt, EStateTransition transition )
{
	int res = BaseClass::TrafficDrive( fDt, transition );

	return res;
}

int CAIPursuerCar::PassiveCopState( float fDt, EStateTransition transition )
{
	// TODO: add other player cars, not just one
	CCar* playerCar = g_pGameSession->GetPlayerCar();

	// check infraction in visible range
	if( CheckObjectVisibility(playerCar) )
	{
		int infraction = CheckTrafficInfraction(playerCar);

		if (m_type == PURSUER_TYPE_COP && infraction == INFRACTION_NONE)
			return 0;

		if( infraction == INFRACTION_MINOR || infraction == INFRACTION_HIT_MINOR || infraction == INFRACTION_HIT )
		{
			playerCar->SetFelony( playerCar->GetFelony() + AI_COP_COLLISION_FELONY_DEBRIS );
			return 0;
		}

		SetPursuitTarget(playerCar);

		float delay = playerCar->GetFelony() > AI_COP_MINFELONY ? AI_COP_BEGINPURSUIT_ARMED_DELAY : AI_COP_BEGINPURSUIT_PASSIVE_DELAY;
		BeginPursuit( delay );
	}

	return 0;
}

void CAIPursuerCar::BeginPursuit( float delay )
{
	if (!m_targInfo.target)
		return;

	if(!IsAlive())
	{
		m_targInfo.target = NULL;
		return;
	}

	AI_SetNextState(&CAIPursuerCar::PursueTarget, delay);
}

void CAIPursuerCar::EndPursuit(bool death)
{
	if (!m_targInfo.target)
		return;

	if(m_loudhailer)
		m_loudhailer->Stop();

	m_sirenAltered = false;

	// HACK: just kill
	if (EqStateMgr::GetCurrentStateType() != GAME_STATE_GAME)
	{
		m_targInfo.target = NULL;
		return;
	}

	if (!death)
	{
		m_autohandbrake = false;
		m_sirenEnabled = false;
		SetLight(CAR_LIGHT_SERVICELIGHTS, false);

		Msg("Make cop start seaching for road");
		AI_SetState(&CAIPursuerCar::SearchForRoad);
	}
	else
		AI_SetState( &CAIPursuerCar::DeadState );

	if (m_targInfo.target != NULL)
	{
		// validate
		if(!g_pGameWorld->IsValidObject(m_targInfo.target))
		{
			m_targInfo.target = NULL;
			return;
		}

		if(m_targInfo.target->GetPursuedCount() == 0)
		{
			m_targInfo.target = NULL;
			return;
		}

		m_targInfo.target->DecrementPursue();

		if (m_targInfo.target->GetPursuedCount() == 0 &&
			g_pGameSession->GetPlayerCar() == m_targInfo.target)	// only play sound when in game, not unloading or restaring
			// g_State_Game->IsGameRunning())
		{
			Speak("cop.lost", true);
		}
	}

	m_targInfo.target = NULL;
}

bool CAIPursuerCar::InPursuit() const
{
	return m_targInfo.target && FSMGetCurrentState() == &CAIPursuerCar::PursueTarget;
}

EInfractionType CAIPursuerCar::CheckTrafficInfraction(CCar* car, bool checkFelony, bool checkSpeeding )
{
	if (!car)
		return INFRACTION_NONE;

	float carSpeed = car->GetSpeed();

	// ho!
	if (checkFelony && car->GetFelony() >= AI_COP_MINFELONY)
		return INFRACTION_HAS_FELONY;

	if (checkSpeeding && carSpeed > AI_COP_CHECK_MAXSPEED)
		return INFRACTION_SPEEDING;

	DkList<CollisionPairData_t>& collisionList = car->GetPhysicsBody()->m_collisionList;

	// don't register other infractions if speed less than 5 km/h
	if(carSpeed < 5.0f)
		return INFRACTION_NONE;

	// check collision
	for(int i = 0; i < collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = collisionList[i];

		if(car->m_lastCollidingObject == pair.bodyB)
			continue;

		if(pair.bodyB->m_flags & COLLOBJ_ISGHOST)
			continue;

		if(pair.bodyB == car->GetHingedBody())
			continue;

		car->m_lastCollidingObject = pair.bodyB;

		if(pair.bodyB == GetPhysicsBody())
			continue;

		if(pair.bodyB->GetContents() == OBJECTCONTENTS_SOLID_GROUND)
			return INFRACTION_NONE;
		else if(pair.bodyB->GetContents() == OBJECTCONTENTS_VEHICLE)
		{
			CGameObject* obj = (CGameObject*)pair.bodyB->GetUserData();

			if(obj->ObjType() == GO_CAR_AI)
			{
				CAITrafficCar* tfc = (CAITrafficCar*)obj;
				if(tfc->IsPursuer())
					continue;
			}

			return INFRACTION_HIT_VEHICLE;
		}
		else if(pair.bodyB->GetContents() == OBJECTCONTENTS_DEBRIS)
		{
			CObject_Debris* obj = (CObject_Debris*)pair.bodyB->GetUserData();

			if(obj && obj->IsSmashed())
				continue;

			return INFRACTION_HIT_MINOR;
		}
		else
			return INFRACTION_HIT;
	}

	straight_t straight = g_pGameWorld->m_level.GetStraightAtPos(car->GetOrigin(), 2);

	// Check wrong direction
	if ( straight.direction != -1)
	{
		Vector3D	startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight.start);
		Vector3D	endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight.end);

		Vector3D	roadDir = fastNormalize(startPos - endPos);

		// make sure he's running in the right direction
		if (dot(car->GetForwardVector(), roadDir) > 0)
			return INFRACTION_WRONG_LANE;
	}

	// only if we going on junction
	levroadcell_t* cell_checkRed = g_pGameWorld->m_level.GetGlobalRoadTile(car->GetOrigin());

	Vector3D redCrossingCheckPos = car->GetOrigin() - car->GetForwardVector()*12.0f;
	straight_t straight_checkRed = g_pGameWorld->m_level.GetStraightAtPos( redCrossingCheckPos, 2 );

	// Check red light crossing
	if( cell_checkRed && cell_checkRed->type == ROADTYPE_JUNCTION &&
		straight_checkRed.direction != -1 )
	{
		Vector3D	startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight_checkRed.start);
		Vector3D	endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight_checkRed.end);

		if(ai_debug_pursuer.GetBool())
			debugoverlay->Box3D(endPos - 2, endPos + Vector3D(2,100,2), ColorRGBA(1,1,0,1), 0.1f);

		Vector3D	roadDir = fastNormalize(startPos - endPos);

		Plane		roadEndPlane(roadDir, -dot(roadDir, endPos));

		// brake on global traffic light value
		int trafficLightDir = g_pGameWorld->m_globalTrafficLightDirection;

		int curDir = straight_checkRed.direction % 2;

		float distToStop = roadEndPlane.Distance(car->GetOrigin());

		bool isAllowedToMove = straight_checkRed.hasTrafficLight ? (trafficLightDir%2 == curDir%2) : true;

		if (distToStop < 0 && !isAllowedToMove)
			return INFRACTION_RED_LIGHT;
	}

	return INFRACTION_NONE;
}

bool CAIPursuerCar::CheckObjectVisibility(CCar* obj)
{
	if (!obj)
		return false;

	float farPlane = AI_COPVIEW_FAR;
	float fov = AI_COPVIEW_FOV;
	float visibilitySphere = AI_COPVIEW_RADIUS;

	if( obj->GetFelony() >= AI_COP_MINFELONY )
	{
		farPlane = AI_COPVIEW_FAR_WANTED;
		fov = AI_COPVIEW_FOV_WANTED;
		visibilitySphere = AI_COPVIEW_RADIUS_WANTED;
	}

	if(obj->GetPursuedCount() > 0)
		visibilitySphere = AI_COPVIEW_RADIUS_PURSUIT;

	if(!m_enabled)
	{
		farPlane = AI_COPVIEW_FAR_ROADBLOCK;
		visibilitySphere = AI_COPVIEW_RADIUS_ROADBLOCK;
	}

	float distToCar = length(obj->GetOrigin() - GetOrigin());

	//debugoverlay->Box3D(GetOrigin() - visibilitySphere, GetOrigin() + visibilitySphere, ColorRGBA(1,1,0,1), 0.1f);

	//if(distToCar > visibilitySphere)
	//	return false;

	Vector3D carAngle = VectorAngles(GetForwardVector());
	carAngle = -VDEG2RAD(carAngle);

	Matrix4x4 proj = perspectiveMatrixY(DEG2RAD(fov), 1.0f, 1.0f, 1.0f, farPlane);
	Matrix4x4 view = rotateZXY4(carAngle.x, carAngle.y, carAngle.z);
	view.translate(-GetOrigin());

	Matrix4x4 viewprojection = proj * view;

	Volume frustum;
	frustum.LoadAsFrustum(viewprojection);

	// do frustum checks for player cars
	if(	distToCar < visibilitySphere ||
		frustum.IsPointInside(obj->GetOrigin()))
	{
		// Trace convex car
		CollisionData_t coll;

		eqPhysCollisionFilter collFilter;
		collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
		collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
		collFilter.AddObject( GetPhysicsBody() );
		collFilter.AddObject( obj->GetPhysicsBody() );

		// if has any collision then deny

		bool hasColl = g_pPhysics->TestLine(GetOrigin() + Vector3D(0,1,0), obj->GetOrigin(), coll, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_SOLID_GROUND, &collFilter);
		return !hasColl;
	}

	return false;
}

int	CAIPursuerCar::PursueTarget( float fDt, EStateTransition transition )
{
	bool targetIsValid = (m_targInfo.target != NULL);

	if(transition == STATE_TRANSITION_PROLOG)
	{
		m_gearboxShiftThreshold = 1.0f;

		if(!targetIsValid)
		{
			AI_SetState(&CAIPursuerCar::SearchForRoad);
			return 0;
		}

		// reset the emergency lights
		SetLight(CAR_LIGHT_EMERGENCY, false);

		// restore collision
		GetPhysicsBody()->SetCollideMask(COLLIDEMASK_VEHICLE);
		GetPhysicsBody()->SetMinFrameTime(0.0f);
		GetPhysicsBody()->Wake();

		if(m_type == PURSUER_TYPE_COP)
		{
			if (m_conf->visual.sirenType != SERVICE_LIGHTS_NONE)
				m_sirenEnabled = true;

			if (m_targInfo.target->GetPursuedCount() == 0 &&
				g_pGameSession->GetPlayerCar() == m_targInfo.target)
			{
				if (m_targInfo.target->GetFelony() >= AI_COP_MINFELONY)
				{
					Speak("cop.pursuit_continue", true);
				}
				else
				{
					m_targInfo.target->SetFelony(AI_COP_MINFELONY);

					Speak("cop.pursuit", true);
				}
			}
		}

		if(g_railroadCops.GetBool())
		{
			// make automatically angry
			SetInfiniteMass(true);

			m_targInfo.isAngry = true;
		}

		m_targInfo.target->IncrementPursue();

		m_enabled = true;

		return 0;
	}
	else if(transition == STATE_TRANSITION_EPILOG)
	{
		m_gearboxShiftThreshold = 0.6f;
	}
	else
	{
		if(!targetIsValid)
		{
			EndPursuit(!IsAlive());
			return 0;
		}
	}

	// do nothing
	if(transition != STATE_TRANSITION_NONE)
		return 0;

	// dead?
	if(!IsAlive())
	{
		EndPursuit(true);

		AI_SetState( &CAIPursuerCar::DeadState );
		m_refreshTime = 0.0f;
		return 0;
	}

	if (!m_targInfo.target)
		return 0;

	Vector3D targetVelocity = m_targInfo.target->GetVelocity();
	Vector3D targetForward = m_targInfo.target->GetForwardVector();

	float targetSpeed = m_targInfo.target->GetSpeed();
	float targetSpeedMPS = (targetSpeed*KPH_TO_MPS);

	bool isVisible = CheckObjectVisibility( m_targInfo.target );

	// check the visibility
	if ( !isVisible )
	{
		m_targInfo.notSeeingTime += fDt;

		if (m_targInfo.notSeeingTime  > AI_COP_TIME_TO_LOST_TARGET)
		{
			EndPursuit(false);
			return 0;
		}
	}
	else
	{
		if (m_targInfo.notSeeingTime  > AI_COP_TIME_TO_LOST_TARGET*0.5f &&
			g_pGameSession->GetPlayerCar() == m_targInfo.target)
		{
			Speak("cop.pursuit_continue");
		}

		if (m_type == PURSUER_TYPE_COP)
		{
			if(	m_targInfo.lastInfraction > INFRACTION_HAS_FELONY &&
				m_targInfo.nextCheckImpactTime < 0 )
			{
				m_targInfo.nextCheckImpactTime = AI_COP_COLLISION_CHECKTIME;

				float newFelony = m_targInfo.target->GetFelony();

				switch( m_targInfo.lastInfraction )
				{
					case INFRACTION_HIT_VEHICLE:
					{
						newFelony += AI_COP_COLLISION_FELONY_VEHICLE;

						Speak("cop.hitvehicle");
						break;
					}
					case INFRACTION_HIT:
					{
						newFelony += AI_COP_COLLISION_FELONY;
						break;
					}
					case INFRACTION_HIT_MINOR:
					{
						newFelony += AI_COP_COLLISION_FELONY_DEBRIS;
						break;
					}
					case INFRACTION_RED_LIGHT:
					{
						newFelony += AI_COP_COLLISION_FELONY_REDLIGHT;
						Speak("cop.redlight");
						break;
					}
				}

				if (newFelony > 1.0f)
					newFelony = 1.0f;

				m_targInfo.lastInfraction = INFRACTION_HAS_FELONY;
				m_targInfo.target->SetFelony(newFelony);
			}

			if (m_loudhailer)
			{
				m_loudhailer->SetOrigin(GetOrigin());
				m_loudhailer->SetVelocity(GetVelocity());

				TrySayTaunt();
			}

			if(targetSpeed > 50.0f)
			{
				float targetAngle = VectorAngles( fastNormalize(targetVelocity) ).y + 45;

				#pragma todo("North direction as second argument")
				targetAngle = NormalizeAngle360( -targetAngle /*g_pGameWorld->GetLevelNorthDirection()*/);

				int targetDir = targetAngle / 90.0f;

				if (targetDir < 0)
					targetDir += 4;

				if (targetDir > 3)
					targetDir -= 4;

				if( m_targInfo.direction != targetDir && targetDir < 4)
				{
					if(Speak("cop.heading"))
					{
						if (targetDir == 0)
							Speak("cop.heading_west", true);
						else if (targetDir == 1)
							Speak("cop.heading_north", true);
						else if (targetDir == 2)
							Speak("cop.heading_east", true);
						else if (targetDir == 3)
							Speak("cop.heading_south", true);
					}

					m_targInfo.direction = targetDir;
				}
			}
		}

		m_targInfo.notSeeingTime = 0.0f;
	}

	if(!ai_debug_pursuer.GetBool())
	{
		// don't update AI in replays, unless we have debugging enabled
		if(g_replayData->m_state == REPL_PLAYING)
			return 0;
	}

	// don't try to control the car
	if(IsFlippedOver())
	{
		// keep only the analog steering
		SetControlButtons( IN_ANALOGSTEER );
		return 0;
	}

	float mySpeed = GetSpeed();

	// update navigation affector parameters
	m_navAffector.m_manipulator.m_driveTarget = m_targInfo.target->GetOrigin();
	m_navAffector.m_manipulator.m_driveTargetVelocity = m_targInfo.target->GetVelocity();
	m_navAffector.m_manipulator.m_excludeColl = m_targInfo.target->GetPhysicsBody();

	// update target avoidance affector parameters
	m_targetAvoidance.m_manipulator.m_avoidanceRadius = 10.0f;
	m_targetAvoidance.m_manipulator.m_enabled = true;
	m_targetAvoidance.m_manipulator.m_targetPosition = m_targInfo.target->GetOrigin();

	// do regular updates of ai navigation
	m_navAffector.Update(this, fDt);

	// make stability control
	m_stability.m_manipulator.m_initialHandling = m_navAffector.m_handling;
	m_stability.Update(this, fDt);

	// collision avoidance
	// take the initial handing for avoidance from navigator
	m_collAvoidance.m_manipulator.m_initialHandling = m_navAffector.m_handling;
	m_collAvoidance.Update(this, fDt);

	// target avoidance
	m_targetAvoidance.Update(this, fDt);

	ai_handling_t handling = m_navAffector.m_handling;

	// TODO: apply stability control handling amounts to the final handling
	// based on the game difficulty
	handling += m_stability.m_handling;
	handling *= m_targetAvoidance.m_handling;

	if(m_collAvoidance.m_manipulator.m_enabled)
		handling = m_collAvoidance.m_handling;

	if(!m_stability.m_handling.autoHandbrake)
		handling.autoHandbrake = false;

	int controls = IN_ACCELERATE | IN_ANALOGSTEER;

	// alternating siren sound by speed
	if(	m_type == PURSUER_TYPE_COP )
	{
		bool newAlterState = (mySpeed > AI_ALTER_SIREN_MIN_SPEED);

		if(newAlterState != m_sirenAltered)
		{
			m_alterSirenChangeTime -= fDt;

			if(m_alterSirenChangeTime <= 0.0f)
			{
				m_sirenAltered = (mySpeed > AI_ALTER_SIREN_MIN_SPEED);
				m_alterSirenChangeTime = AI_ALTER_SIREN_CHANGETIME;
			}
		}
		else
			m_alterSirenChangeTime = AI_ALTER_SIREN_CHANGETIME;

		if(m_sirenAltered)
			controls |= IN_HORN;
	}

	if(handling.braking > 0.5f)
	{
		controls &= ~IN_ACCELERATE;
		controls |= IN_BRAKE;
	}

	if(fabs(handling.steering) > 0.7f)
		controls |= IN_EXTENDTURN;

	m_autohandbrake = handling.autoHandbrake;

	SetControlButtons( controls );
	SetControlVars(	handling.acceleration,
					handling.braking,
					handling.steering);

	if(handling.acceleration > 0.05f)
		GetPhysicsBody()->TryWake(false);

	return 0;
}

int	CAIPursuerCar::DeadState( float fDt, EStateTransition transition )
{
	int buttons = IN_ANALOGSTEER;
	SetControlButtons( buttons );

	SetLight(CAR_LIGHT_SERVICELIGHTS, m_sirenEnabled);

	return 0;
}

void CAIPursuerCar::SetPursuitTarget(CCar* obj)
{
	m_targInfo.target = obj;
	m_targInfo.direction = -1;

	m_targInfo.lastInfraction = INFRACTION_HAS_FELONY;
	m_targInfo.nextCheckImpactTime = AI_COP_COLLISION_CHECKTIME;

	if(obj)
		m_targInfo.isAngry = (obj->GetFelony() > 0.6f) || m_type == PURSUER_TYPE_GANG;
}

OOLUA_EXPORT_FUNCTIONS(
	CAIPursuerCar,
	SetPursuitTarget,
	CheckObjectVisibility,
	BeginPursuit,
	EndPursuit
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CAIPursuerCar
)
