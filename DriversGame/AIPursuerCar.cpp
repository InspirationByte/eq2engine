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

ConVar ai_debug_pursuer_nav("ai_debug_pursuer_nav", "0", NULL, CV_CHEAT);
ConVar ai_debug_pursuer("ai_debug_pursuer", "0", NULL, CV_CHEAT);

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

const float AI_COP_BLOCK_DELAY			= 1.5f;
const float AI_COP_BLOCK_REALIZE_TIME	= 0.8f;

const float AI_COP_CHECK_MAXSPEED = 80.0f; // 80 kph and you will be pursued

const float AI_COP_MINFELONY			= 0.1f;

const float AI_COP_COLLISION_FELONY			= 0.06f;
const float AI_COP_COLLISION_FELONY_VEHICLE	= 0.1f;
const float AI_COP_COLLISION_FELONY_DEBRIS	= 0.02f;
const float AI_COP_COLLISION_FELONY_REDLIGHT = 0.005f;

const float AI_COP_COLLISION_CHECKTIME		= 0.01f;

const float AI_COP_TIME_FELONY = 0.001f;	// 0.1 percent per second

const float AI_COP_TIME_TO_LOST_TARGET = 30.0f;

const float AI_COP_TIME_TO_UPDATE_PATH = 10.0f;			// every 5 seconds
const float AI_COP_TIME_TO_UPDATE_PATH_FORCE = 0.5f;

const float AI_COP_BRAKEDISTANCE_SCALE = 8.0f;

const float AI_COP_ANGRY_RAMMING_DISTANCE = 5.0f;

const float AI_MAX_DISTANCE_TO_PATH = 10.0f;

const float AI_OSTACLE_STEERINGCORRECTION_DIST = 4.0f;

// wheel friction modifier on diferrent weathers
static float pursuerSpeedModifier[WEATHER_COUNT] =
{
	1.0f,
	0.86f,
	0.8f
};

static float pursuerBrakeDistModifier[WEATHER_COUNT] =
{
	1.0f,
	2.5f,
	2.8f
};

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
	m_isColliding = false;
	m_blockTimeout = 0.0f;
	m_blockingTime = 0.0f;
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
		m_isColliding = GetPhysicsBody()->m_collisionList.numElem() > 0;

		if(m_isColliding)
			m_lastCollidingPosition = GetPhysicsBody()->m_collisionList[0].position;

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

	// HACK: just kill
	if (GetCurrentStateType() != GAME_STATE_GAME)
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

void DebugDisplayPath(pathFindResult_t& path, float time = -1.0f)
{
	if(ai_debug_pursuer_nav.GetBool() && path.points.numElem())
	{
		Vector3D lastLinePos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[0]);

		for (int i = 1; i < path.points.numElem(); i++)
		{
			Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[i]);

			float dispalyTime = time > 0 ? time : float(i) * 0.1f;

			debugoverlay->Box3D(pointPos - 0.15f, pointPos + 0.15f, ColorRGBA(1, 1, 0, 1.0f), dispalyTime);
			debugoverlay->Line3D(lastLinePos, pointPos, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), dispalyTime);

			lastLinePos = pointPos;
		}
	}
}

void CAIPursuerCar::SetPath(pathFindResult_t& newPath, const Vector3D& searchPos)
{
	m_targInfo.path = newPath;
	m_targInfo.pathTargetIdx = 0;
	m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

	m_targInfo.lastSuccessfulSearchPos = searchPos;
	m_targInfo.searchFails = 0;

	DebugDisplayPath(m_targInfo.path);
}

Vector3D CAIPursuerCar::GetAdvancedPointByDist(int& startSeg, float distFromSegment)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	while (startSeg < m_targInfo.path.points.numElem()-2)
	{
		currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[startSeg]);
		nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[startSeg + 1]);

		float segLen = length(currPointPos - nextPointPos);

		if(distFromSegment > segLen)
		{
			distFromSegment -= segLen;
			startSeg++;
		}
		else
			break;
	}

	currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[startSeg]);
	nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[startSeg + 1]);

	Vector3D dir = fastNormalize(nextPointPos - currPointPos);

	return currPointPos + dir*distFromSegment;
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

		m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

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

	// don't control the car
	if(IsFlippedOver())
	{
		// keep only the analog steering
		SetControlButtons( IN_ANALOGSTEER );
		return 0;
	}

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

	if(g_replayData->m_state == REPL_PLAYING)
		return 0;

	//--------------------------------------------------------------------------------------------------

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject( GetPhysicsBody() );
	collFilter.AddObject( m_targInfo.target->GetPhysicsBody() );

	Vector3D targetOrigin = m_targInfo.target->GetOrigin();

	// movement prediction factor is distant
	FReal velocityDistOffsetFactor = length(GetOrigin() - targetOrigin);
	velocityDistOffsetFactor = RemapValClamp(velocityDistOffsetFactor, FReal(0.0f), FReal(6.0f), FReal(0.0f), FReal(1.0f));
	velocityDistOffsetFactor = FReal(1.0f - pow((float)velocityDistOffsetFactor, 2.0f));

	Vector3D carForwardDir	= GetForwardVector();
	Vector3D carPos			= GetOrigin() + carForwardDir * m_conf->physics.body_size.z;
	Vector3D carLinearVel	= GetVelocity();

	FReal fSpeed = GetSpeedWheels();
	FReal speedMPS = (fSpeed*KPH_TO_MPS);

	FReal velocity = GetSpeed();

	FReal distToPursueTarget = length(m_targInfo.target->GetOrigin() - GetOrigin());

	FReal brakeDistancePerSec = m_conf->GetBrakeEffectPerSecond()*0.5f;
	FReal brakeToStopTime = speedMPS / brakeDistancePerSec*2.0f;
	FReal brakeDistAtCurSpeed = brakeDistancePerSec*brakeToStopTime;

	FReal weatherBrakeDistModifier = pursuerBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType];

	bool doesHaveStraightPath = true;

	// test for the straight path and visibility
	if(distToPursueTarget < AI_COPVIEW_FAR_HASSTRAIGHTPATH)
	{
		float tracePercentage = g_pGameWorld->m_level.Nav_TestLine(GetOrigin(), m_targInfo.target->GetOrigin(), false);

		if(ai_debug_pursuer_nav.GetBool())
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path trace percentage: %d", (int)(tracePercentage*100.0f));

		if(tracePercentage < 1.0f)
			doesHaveStraightPath = false;
	}
	else
	{
		doesHaveStraightPath = false;
	}


	if(ai_debug_pursuer_nav.GetBool())
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "has straight path: %d", doesHaveStraightPath);

	//-------------------------------------------------------------------------------
	// refresh the navigation path if we don't see the target

	// update last successful position by checking cell which at carPos
	ubyte navPoint = g_pGameWorld->m_level.Nav_GetTileAtPosition(GetOrigin());

	if(navPoint >= 0x4)
		m_targInfo.lastSuccessfulSearchPos = GetOrigin();

	//
	// only if we have made most of the path
	//
	if (!doesHaveStraightPath)
	{
		//
		// Get the last point on the path
		//
		int lastPoint = m_targInfo.pathTargetIdx;
		Vector3D pathTarget = (lastPoint != -1) ? g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[lastPoint]) : targetOrigin;

		FReal pathCompletionPercentage = (float)lastPoint / (float)m_targInfo.path.points.numElem();

		if(length(pathTarget-carPos) > AI_MAX_DISTANCE_TO_PATH)
			pathCompletionPercentage = 1.0f;

		// if we have old path, try continue moving
		if(pathCompletionPercentage < 0.5f && m_targInfo.path.points.numElem() > 1)
		{
			m_targInfo.nextPathUpdateTime -= fDt;
		}
		else
		{
			if(m_targInfo.nextPathUpdateTime > AI_COP_TIME_TO_UPDATE_PATH_FORCE )
				m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH_FORCE;

			m_targInfo.nextPathUpdateTime -= fDt;
		}

		DebugDisplayPath(m_targInfo.path, m_refreshTime);

		if(ai_debug_pursuer_nav.GetBool())
		{
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "pathCompletionPercentage: %d (length=%d)", (int)(pathCompletionPercentage*100.0f), m_targInfo.path.points.numElem());
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "nextPathUpdateTime: %g", m_targInfo.nextPathUpdateTime);
		}

		if( m_targInfo.nextPathUpdateTime <= 0.0f)
		{
			Vector3D searchStart(carPos);

			int trials = m_targInfo.searchFails;

			if(m_targInfo.searchFails > 0)
			{
				searchStart = m_targInfo.lastSuccessfulSearchPos;
			}

			pathFindResult_t newPath;
			if( g_pGameWorld->m_level.Nav_FindPath(targetOrigin, searchStart, newPath, 1024, true))
			{
				SetPath(newPath, carPos);
			}
			else
			{
				m_targInfo.searchFails++;

				if(m_targInfo.searchFails > 1)
					m_targInfo.searchFails = 0;

				//debugoverlay->Box3D(m_targInfo.lastSuccessfulSearchPos-1.0f,m_targInfo.lastSuccessfulSearchPos+1.0f, color4_white, 1.0f);

				m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH_FORCE;
			}

			if(ai_debug_pursuer_nav.GetBool())
				debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path search result: %d points, tries: %d", newPath.points.numElem(), trials);
		}
	}
	else
		m_targInfo.pathTargetIdx = -1;

	//
	// Introduce steering target and brake position
	//
	Vector3D steeringTargetPos = targetOrigin;

	Vector3D steeringTargetPosB = carPos + targetForward*velocityDistOffsetFactor;
	Vector3D brakeTargetPos = targetOrigin + lerp(targetForward*targetSpeedMPS, targetVelocity, 0.25f) * velocityDistOffsetFactor;

	if(m_targInfo.isAngry)
	{
		// FIXME: should be this more careful and timed?
		if(distToPursueTarget > AI_COP_ANGRY_RAMMING_DISTANCE)
			brakeTargetPos = targetOrigin + targetVelocity*10.0f;
	}

	CollisionData_t velocityColl;
	CollisionData_t frontColl;

	float frontCollDist = clamp((float)fSpeed, 1.0f, 8.0f);

	btBoxShape carBoxShape(btVector3(m_conf->physics.body_size.x, m_conf->physics.body_size.y, 0.25f));

	// trace in the front direction
	g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(),
		GetOrigin(), GetOrigin()+carForwardDir*frontCollDist, frontColl,
		OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter);

	// trace the car body in velocity direction
	g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(),
		GetOrigin(), GetOrigin()+carLinearVel*2.0f, velocityColl,
		OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE,
		&collFilter);

	//-------------------------------------------------------------------------------
	// calculate the steering

	bool doesHardSteer = false;

	FReal lateralSlideSigned = GetLateralSlidingAtBody();
	FReal lateralSlide = FPmath::abs(lateralSlideSigned);

	FReal speedFactor = RemapValClamp(fSpeed, FReal(0.0f), FReal(50.0f), FReal(0.0f), FReal(1.0f));

	FReal speedToSteering = fSpeed - lateralSlideSigned*10.0f;
	FReal steeringSpeedFactor = pow( (float)RemapValClamp(speedToSteering, FReal(0.0f), FReal(180.0f), FReal(1.0f), FReal(0.45f)), 0.5f);

	if( !doesHaveStraightPath &&
		m_targInfo.path.points.numElem() > 0 &&
		m_targInfo.pathTargetIdx != -1 &&
		m_targInfo.pathTargetIdx < m_targInfo.path.points.numElem()-2)
	{
		Vector3D currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx]);
		Vector3D nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx + 1]);

		FReal currPosPerc = lineProjection(currPointPos, nextPointPos, carPos);

		FReal len = length(currPointPos - nextPointPos);

		FReal speedModifier = RemapValClamp(FReal(60.0f) - fSpeed, FReal(0.0f), FReal(60.0f), FReal(4.0f), FReal(8.0f));

		int pathIdx = m_targInfo.pathTargetIdx;

		// calculate steering target position and advance current point
		steeringTargetPos = GetAdvancedPointByDist(m_targInfo.pathTargetIdx, currPosPerc*len+8.0f);

		// also get hardsteering angle to calculate braking
		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosStart = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 6.0f + brakeDistAtCurSpeed*2.0f*speedFactor*weatherBrakeDistModifier);

		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosEnd = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 7.0f + brakeDistAtCurSpeed*2.0f*speedFactor*weatherBrakeDistModifier);

		Vector3D steerDirHard = fastNormalize(hardSteerPosEnd-hardSteerPosStart);

		FReal cosHardSteerAngle = dot(steerDirHard, fastNormalize(carLinearVel));

		FReal distanceToSteer = length(hardSteerPosStart - carPos);

		if(fSpeed > FReal(20.0f) && cosHardSteerAngle < FReal(0.65f))
		{
			pathIdx = m_targInfo.pathTargetIdx;
			brakeTargetPos = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 16.0f + brakeDistAtCurSpeed*speedFactor*weatherBrakeDistModifier);

			steeringTargetPos = lerp(steeringTargetPos, brakeTargetPos, speedFactor);

			if(distanceToSteer < 2.0f*brakeDistAtCurSpeed)
			{
				// make hard steer
				steeringTargetPosB = hardSteerPosStart;
				steeringTargetPos = hardSteerPosEnd;
				doesHardSteer = true;
			}
		}

		if(ai_debug_pursuer_nav.GetBool())
		{
			debugoverlay->Box3D(hardSteerPosStart - 0.25f, hardSteerPosStart + 0.25f, ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
			debugoverlay->Line3D(hardSteerPosStart, hardSteerPosEnd, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
			debugoverlay->Box3D(hardSteerPosEnd - 0.25f, hardSteerPosEnd + 0.25f, ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
		}
	}

	bool hasCarObstacleInFront = false;

	// get rid of obstacles by tracing sphere forwards
	{
		float traceShapeRadius = 1.0f + fSpeed*0.01f;

		CollisionData_t steeringTargetColl;
		btSphereShape sphereTraceShape(traceShapeRadius);

		// trace the car body in velocity direction
		g_pPhysics->TestConvexSweep(&sphereTraceShape, GetOrientation(),
			carPos, steeringTargetPos, steeringTargetColl,
			OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE,
			&collFilter);

		debugoverlay->Line3D(carPos, steeringTargetColl.position, ColorRGBA(0, 1, 0, 1.0f), ColorRGBA(1, 1, 0, 1.0f), DOVERLAY_DELAY);

		// correct the steering to prevent car damage if we moving towards obstacle
		if(steeringTargetColl.fract < 1.0f)
		{
			float distanceScale = (1.0f-steeringTargetColl.fract);

			float newPositionDist = distanceScale * AI_OSTACLE_STEERINGCORRECTION_DIST + FPmath::abs(speedMPS)* distanceScale * 0.25f;

			Vector3D newSteeringTargetPos = steeringTargetColl.position + steeringTargetColl.normal * newPositionDist; // 4 meters is enough to be safe

			Vector3D checkSteeringDir = fastNormalize(steeringTargetPos-steeringTargetPosB);

			//if(dot(normalize(newSteeringTargetPos-carPos), normalize(carForwardDir)) > 0.5f)
			if(fabs(dot(checkSteeringDir, carForwardDir)) > 0.5f)
			{
				steeringTargetPosB = carPos;
				steeringTargetPos = newSteeringTargetPos;

				//if(ai_debug_pursuer_nav.GetBool())
				{
					debugoverlay->Box3D(steeringTargetColl.position - traceShapeRadius, steeringTargetColl.position + traceShapeRadius, ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
					debugoverlay->Line3D(steeringTargetColl.position, steeringTargetPos, ColorRGBA(1, 1, 0, 1.0f), ColorRGBA(1, 1, 0, 1.0f), DOVERLAY_DELAY);
					debugoverlay->Box3D(steeringTargetPos - traceShapeRadius, steeringTargetPos + traceShapeRadius, ColorRGBA(0, 1, 0, 1.0f), DOVERLAY_DELAY);
				}
			}
		}

		if(!m_sirenEnabled && frontColl.fract < 1.0f && frontColl.hitobject)
		{
			if(frontColl.hitobject->m_flags & BODY_ISCAR)
			{
				CCar* car = (CCar*)frontColl.hitobject->GetUserData();

				hasCarObstacleInFront = car->IsAlive() && car->IsEnabled() && !(car->GetPursuedCount() > 0);
			}
		}
	}

	if(ai_debug_pursuer_nav.GetBool() || ai_debug_pursuer.GetBool())
	{
		debugoverlay->Line3D(carPos, carPos+carLinearVel, ColorRGBA(1, 1, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
		debugoverlay->Line3D(carPos, carPos+carForwardDir*10.0f, ColorRGBA(1, 1, 0, 1.0f), ColorRGBA(1, 0, 1, 1.0f), DOVERLAY_DELAY);
		debugoverlay->Box3D(steeringTargetPos - 0.25f, steeringTargetPos + 0.25f, ColorRGBA(1, 0, 1, 1.0f), DOVERLAY_DELAY);
		debugoverlay->Box3D(steeringTargetPosB - 0.25f, steeringTargetPosB + 0.25f, ColorRGBA(1, 0, 0, 1.0f), DOVERLAY_DELAY);
		debugoverlay->Box3D(brakeTargetPos - 0.25f, brakeTargetPos + 0.25f, ColorRGBA(0, 1, 0, 1.0f), DOVERLAY_DELAY);
	}

	GetPhysicsBody()->TryWake(false);

	Matrix4x4 bodyMat;
	GetPhysicsBody()->ConstructRenderMatrix( bodyMat );

	Vector3D steeringTargetDir = fastNormalize(steeringTargetPos-steeringTargetPosB);
	Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringTargetDir);

	FReal fSteerTarget = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z);
	FReal fSteeringAngle = clamp(fSteerTarget , FReal(-1.0f), FReal(1.0f)) * steeringSpeedFactor;

	//if(lateralSlide < 1.0f)
	//	fSteeringAngle = sign(fSteeringAngle) * powf(fabs(fSteeringAngle), 1.25f);

	// inverese steering angle if we're going backwards
	if(fSpeed < -0.1f)
		fSteeringAngle *= -1.0f;

	if(lateralSlide > 4.0f && sign(lateralSlideSigned)+sign(fSteeringAngle) < 0.5f)
	{
		//FReal lateralSlideSpeedSteerModifier = 1.0f - fabs(dot(relateiveSteeringDir, fastNormalize(carLinearVel)));
		//FReal lateralSlideCorrectionSpeedModifier = RemapValClamp(fSpeed, 0, 40, 0.0f, 1.0f);

		//fSteeringAngle *= lateralSlideSpeedSteerModifier * lateralSlideCorrectionSpeedModifier;
		doesHardSteer = false;
	}

	Vector3D velocityDir = fastNormalize(carLinearVel+carForwardDir);

	FReal steeringToTargetDot = dot(steeringTargetDir, velocityDir);
	FReal steeringBrakeFactor = pow(1.0f - max(FReal(0.0f), steeringToTargetDot), 2.0f);

	FReal accelerator = 1.0f;
	FReal brake = 0.0f;

	FReal distToTarget = length(steeringTargetPos - carPos);
	FReal distToBrakeTarget = length(brakeTargetPos - carPos);
	FReal distToTargetReal = length(m_targInfo.target->GetOrigin() - GetOrigin());

	int controls = IN_ACCELERATE | IN_ANALOGSTEER | IN_EXTENDTURN;

	// make fast siren sound
	if(	m_sirenEnabled && fabs(fSpeed) > 20 || 
		!m_sirenEnabled && hasCarObstacleInFront)
		controls |= IN_HORN;

	//if(frontColl.fract < 1.0f)
	//	steeringTargetPos = frontColl.position + carForwardDir*5.0f + frontColl.normal*(1.0f-frontColl.fract)*16.0f;

	if (m_blockTimeout <= 0.0f && (frontColl.fract < 1.0f || m_isColliding) && fSpeed < 5.0f)
	{
		m_blockingTime += fDt;

		if(m_blockingTime > AI_COP_BLOCK_REALIZE_TIME)
		{
			m_blockTimeout = AI_COP_BLOCK_DELAY;
			m_blockingTime = 0.0f;
		}
	}
	else
	{
		FReal distFromCollPoint = length(m_lastCollidingPosition-GetOrigin());

		if(distFromCollPoint > 5.0f)
			m_blockTimeout = 0.0f;

		m_blockTimeout -= fDt;

		if(m_blockTimeout > 0.0f)
		{
			brake = 1.0f;
			accelerator = 0.0f;
			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;
		}
	}

	if(fSpeed > 1.0f)
	{
		FReal velocityToTargetFactor = pow(max(FReal(0.0f), steeringToTargetDot), 0.5f);

		FReal lateralSlideSpd = lateralSlide*speedFactor;

		FReal distToTargetDiff = fSpeed*KPH_TO_MPS * 2.0f - distToBrakeTarget;
		FReal brakeDistantFactor = RemapValClamp(distToTargetDiff, FReal(0.0f), FReal(AI_COP_BRAKEDISTANCE_SCALE), FReal(0.0f), FReal(1.0f)) * weatherBrakeDistModifier;

		if(doesHardSteer || velocityColl.fract < 0.5f || speedFactor > 0.05f && (steeringToTargetDot < 0.85f || speedFactor >= 0.5f && brakeDistantFactor > 0.0f) )
		{
			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;
		}

		doesHardSteer = doesHardSteer || speedFactor >= 0.25f && lateralSlideSpd < 2.2f;

		float directionBrakeFac = fabs(dot(velocityColl.normal, carForwardDir));

		brake += (1.0f-velocityColl.fract) * directionBrakeFac + steeringBrakeFactor*brakeDistantFactor*0.5f;

		if(lateralSlideSpd > 1.0f)
			brake += (FReal)lateralSlideSpd;

		//brake *= FReal( powf(speedFactor, 0.25f)* 6.0f * brakeDistantFactor * pow(1.0f-min(velocityToTargetFactor, 1.0f), 0.25f)*weatherBrakeDistModifier );

		if(ai_debug_pursuer.GetBool())
		{
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "-------------------");
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "velocityToTargetFactor: %.2f speedFactor: %.2f lateralSlide: %.2f brakeDistantFactor: %.2f", velocityToTargetFactor, speedFactor, lateralSlide, brakeDistantFactor);
			debugoverlay->TextFadeOut(0, ColorRGBA(1,1,0,1), 10.0f, "result brake: %.2f hasToBrake: %d hardSteer: %d", (float)brake, (controls & IN_BRAKE) > 0, doesHardSteer);
		}
	}

	if(fSpeed > 10.0f)
	{
		accelerator -= (FReal)fabs(fSteeringAngle)*0.25f*speedFactor;
	}

	if((controls & IN_ACCELERATE) && fSpeed < 50.0f && lateralSlide < 1.0f && accelerator >= 1.0f)
		controls |= IN_BURNOUT;

	m_autohandbrake = doesHardSteer;

	SetControlButtons( controls );
	SetControlVars(accelerator, brake, clamp(fSteeringAngle, FReal(-1.0f), FReal(1.0f)));

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
	m_targInfo.path.points.clear();
	m_targInfo.pathTargetIdx = -1;
	m_targInfo.nextCheckImpactTime = AI_COP_COLLISION_CHECKTIME;
	m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

	m_targInfo.lastSuccessfulSearchPos = GetOrigin();
	m_targInfo.searchFails = 0;

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
