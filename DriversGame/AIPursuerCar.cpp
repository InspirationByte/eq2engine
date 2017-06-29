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

const float AI_COPVIEW_FOV			= 85.0f;
const float AI_COPVIEW_FOV_WANTED	= 90.0f;

const float AI_COPVIEW_FAR_ROADBLOCK	= 10.0f;

const float AI_COPVIEW_RADIUS			= 18.0f;
const float AI_COPVIEW_RADIUS_WANTED	= 25.0f;
const float AI_COPVIEW_RADIUS_PURSUIT	= 120.0f;
const float AI_COPVIEW_RADIUS_ROADBLOCK = 15.0f;

const float AI_COP_DEATHTIME	= 3.5f;
const float AI_COP_BLOCK_DELAY			= 1.5f;
const float AI_COP_BLOCK_REALIZE_TIME	= 1.0f;

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

const float AI_MAX_DISTANCE_TO_PATH = 10.0f;

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

	m_taunts = NULL;
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
		m_taunts = ses->CreateSoundController(&ep);
	}

	SetMaxDamage( g_pAIManager->GetCopMaxDamage() );
}

void CAIPursuerCar::OnRemove()
{
	EndPursuit(true);

	if (m_taunts)
		ses->RemoveSoundController(m_taunts);

	m_taunts = NULL;

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
			BeginPursuit();
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
		m_taunts->Stop();
		m_taunts->Play();

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

	// death by water?
	if(!IsAlive() && m_inWater)
	{
		EndPursuit(true);
		AI_SetState( &CAIPursuerCar::DeadState );
	}

	m_isColliding = GetPhysicsBody()->m_collisionList.numElem() > 0;

	if(m_isColliding)
		m_lastCollidingPosition = GetPhysicsBody()->m_collisionList[0].position;

	PassiveCopState( fDt, STATE_TRANSITION_NONE );

	if(IsAlive() && m_targInfo.target)
	{
		int infraction = CheckTrafficInfraction(m_targInfo.target, false,false);

		if(infraction > INFRACTION_HAS_FELONY)
			m_targInfo.lastInfraction = infraction;
	}

}

int	CAIPursuerCar::TrafficDrive( float fDt, EStateTransition transition )
{
	int res = BaseClass::TrafficDrive( fDt, transition );

	return res;
}

int CAIPursuerCar::PassiveCopState( float fDt, EStateTransition transition )
{
	CCar* playerCar = g_pGameSession->GetPlayerCar();

	// check infraction in visible range
	if( m_targInfo.target == NULL &&
		m_gameDamage < m_gameMaxDamage &&
		CheckObjectVisibility(playerCar))
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
		BeginPursuit();
	}

	return 0;
}

void CAIPursuerCar::BeginPursuit( float delay )
{
	if (!m_targInfo.target)
		return;

	AI_SetNextState(&CAIPursuerCar::PursueTarget, delay);
}

void CAIPursuerCar::EndPursuit(bool death)
{
	if (!m_targInfo.target)
		return;

	if (!death)
	{
		m_sirenEnabled = false;
		SetLight(CAR_LIGHT_SERVICELIGHTS, false);
	}

	m_autohandbrake = false;

	if (GetCurrentStateType() != GAME_STATE_GAME)
	{
		m_targInfo.target = NULL;
		return;
	}

	if (g_pGameWorld->IsValidObject(m_targInfo.target))
	{
		if(m_targInfo.target->GetPursuedCount() == 0)
		{
			m_targInfo.target = NULL;
			return;
		}

		m_targInfo.target->DecrementPursue();

		if (m_targInfo.target->GetPursuedCount() == 0 &&
			g_pGameSession->GetPlayerCar() == m_targInfo.target &&
			g_State_Game->IsGameRunning())	// only play sound when in game, not unloading or restaring
		{
			Speak("cop.lost", true);
		}
	}

	if(m_taunts)
		m_taunts->Stop();

	m_targInfo.target = NULL;

	AI_SetState(&CAIPursuerCar::SearchForRoad);
}

bool CAIPursuerCar::InPursuit() const
{
	return m_targInfo.target && FSMGetCurrentState() == &CAIPursuerCar::PursueTarget;
}

EInfractionType CAIPursuerCar::CheckTrafficInfraction(CCar* car, bool checkFelony, bool checkSpeeding )
{
	if (!car)
		return INFRACTION_NONE;

	// ho!
	if (checkFelony && car->GetFelony() >= AI_COP_MINFELONY)
		return INFRACTION_HAS_FELONY;

	if (checkSpeeding && car->GetSpeed() > AI_COP_CHECK_MAXSPEED)
		return INFRACTION_SPEEDING;

	DkList<CollisionPairData_t>& collisionList = car->GetPhysicsBody()->m_collisionList;

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
	if(transition == STATE_TRANSITION_PROLOG)
	{
		m_gearboxShiftThreshold = 1.0f;

		if(!IsAlive() || !g_pGameWorld->IsValidObject(m_targInfo.target))
		{
			AI_SetState(&CAIPursuerCar::SearchForRoad);
			return 0;
		}

		m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

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
		m_deathTime = AI_COP_DEATHTIME;
		m_refreshTime = 0.0f;
		return 0;
	}

	if (!m_targInfo.target)
		return 0;

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

			if (m_taunts)
			{
				m_taunts->SetOrigin(GetOrigin());
				m_taunts->SetVelocity(GetVelocity());

				TrySayTaunt();
			}

			if(m_targInfo.target->GetSpeed() > 50.0f)
			{
				float targetAngle = VectorAngles(normalize(m_targInfo.target->GetVelocity())).y + 45;

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

	// movement prediction factor is distant
	float velocityDistOffsetFactor = length(GetOrigin() - m_targInfo.target->GetOrigin());
	velocityDistOffsetFactor = RemapValClamp(velocityDistOffsetFactor, 0.0f, 6.0f, 0.0f, 1.0f);
	velocityDistOffsetFactor = 1.0f - pow(velocityDistOffsetFactor, 2.0f);

	Vector3D carForwardDir	= GetForwardVector();
	Vector3D carPos			= GetOrigin() + carForwardDir * m_conf->physics.body_size.z;
	Vector3D carLinearVel	= GetVelocity();

	Vector3D targetVelocity = m_targInfo.target->GetVelocity();
	Vector3D targetPos	= m_targInfo.target->GetOrigin() + targetVelocity*velocityDistOffsetFactor*0.75f;
	Vector3D targetForward = m_targInfo.target->GetForwardVector();

	float fSpeed = GetSpeedWheels();
	float speedMPS = (fSpeed*KPH_TO_MPS);

	float velocity = GetSpeed();

	float distToPursueTarget = length(m_targInfo.target->GetOrigin() - GetOrigin());

	float brakeDistancePerSec = m_conf->GetBrakeEffectPerSecond()*0.5f;
	float brakeToStopTime = speedMPS / brakeDistancePerSec*2.0f;
	float brakeDistAtCurSpeed = brakeDistancePerSec*brakeToStopTime;

	float weatherBrakeDistModifier = pursuerBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType];

	bool doesHaveStraightPath = true;

	// test for the straight path and visibility
	if(distToPursueTarget < AI_COPVIEW_FAR_HASSTRAIGHTPATH)
	{
		// Trace convex car
		CollisionData_t coll;

		//Vector3D targetDir = normalize(m_targInfo.target->GetOrigin()-GetOrigin());

		//Vector3D traceTarget = GetOrigin()+targetDir*min(distToPursueTarget, AI_COPVIEW_FAR_HASSTRAIGHTPATH);

		// so the obstacle forces us to use NAV grid path
		//g_pPhysics->TestConvexSweep(GetPhysicsBody()->GetBulletShape(), GetOrientation(), GetOrigin(), traceTarget, coll, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter);

		//if(coll.fract < 1.0f)

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
		Vector3D pathTarget = (lastPoint != -1) ? g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[lastPoint]) : targetPos;

		float pathCompletionPercentage = (float)lastPoint / (float)m_targInfo.path.points.numElem();

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
			if( g_pGameWorld->m_level.Nav_FindPath(targetPos, searchStart, newPath, 1024, false))
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
	Vector3D steeringTargetPos = targetPos;

	Vector3D steeringTargetPosB = carPos + targetForward*targetVelocity*velocityDistOffsetFactor;
	Vector3D brakeTargetPos = targetPos + targetForward*targetVelocity*velocityDistOffsetFactor;//carForwardDir*5.0f;
	/*
	if(dot(carForwardDir, targetVelocity) < 0.0f)
	{
		brakeTargetPos = targetPos + carForwardDir*5.0f;
		steeringTargetPosB = carPos;
	}*/

	if(m_targInfo.isAngry)
		brakeTargetPos = targetPos+targetVelocity*5.0f;

	CollisionData_t velocityColl;
	CollisionData_t frontColl;

	float frontCollDist = clamp(fSpeed, 1.0f, 8.0f);

	btBoxShape carBoxShape(btVector3(m_conf->physics.body_size.x, m_conf->physics.body_size.y, 0.25f));

	// trace in the front direction
	g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(),
		GetOrigin(), GetOrigin()+carForwardDir*frontCollDist, frontColl,
		OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter);

	// trace the car body in velocity direction
	g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(),
		GetOrigin(), GetOrigin()+carLinearVel, velocityColl,
		OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE,
		&collFilter);

	//-------------------------------------------------------------------------------
	// calculate the steering

	bool doesHardSteer = false;

	float lateralSlideSigned = GetLateralSlidingAtBody();
	float lateralSlide = fabs(lateralSlideSigned);

	float speedFactor = RemapValClamp(fSpeed, 0.0f, 50.0f, 0.0f, 1.0f);

	if( !doesHaveStraightPath &&
		m_targInfo.path.points.numElem() > 0 &&
		m_targInfo.pathTargetIdx != -1 &&
		m_targInfo.pathTargetIdx < m_targInfo.path.points.numElem()-2)
	{
		Vector3D currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx]);
		Vector3D nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx + 1]);

		float currPosPerc = lineProjection(currPointPos, nextPointPos, carPos);

		float len = length(currPointPos - nextPointPos);

		float speedModifier = RemapValClamp(60.0f - fSpeed, 0.0f, 60.0f, 4.0f, 8.0f);

		int pathIdx = m_targInfo.pathTargetIdx;

		steeringTargetPos = GetAdvancedPointByDist(m_targInfo.pathTargetIdx, currPosPerc*len+8.0f);

		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosStart = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 4.0f + brakeDistAtCurSpeed*2.0f*speedFactor*weatherBrakeDistModifier);

		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosEnd = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 5.0f + brakeDistAtCurSpeed*2.0f*speedFactor*weatherBrakeDistModifier);

		Vector3D steerDirHard = fastNormalize(hardSteerPosEnd-hardSteerPosStart);

		float cosHardSteerAngle = dot(steerDirHard, fastNormalize(carLinearVel));

		float distanceToSteer = length(hardSteerPosStart - carPos);

		if(fSpeed > 20.0f && cosHardSteerAngle < 0.65f)
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
	/*
	float traceShapeRadius = 2.0f + fSpeed*0.02f;

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
		Vector3D newSteeringTargetPos = steeringTargetColl.position + steeringTargetColl.normal*3.0f; // 4 meters is enough to be safe
		
		if(dot(normalize(newSteeringTargetPos-carPos), normalize(carForwardDir)) > 0.5f)
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
	}*/

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

	Vector3D steerDir = fastNormalize((!bodyMat.getRotationComponent()) * fastNormalize(steeringTargetPos-steeringTargetPosB));

	FReal fSteerTarget = atan2(steerDir.x, steerDir.z);
	FReal fSteeringAngle = clamp(fSteerTarget , FReal(-1.0f), FReal(1.0f));

	if(lateralSlide < 1.0f)
		fSteeringAngle = sign(fSteeringAngle) * powf(fabs(fSteeringAngle), 1.25f);

	if(fSpeed < -0.1f)
		fSteeringAngle *= -1.0f;
	
	if(lateralSlide > 4.0f && sign(lateralSlideSigned)+sign(fSteeringAngle) < 0.5f)
	{
		float lateralSlideSpeedSteerModifier = 1.0f - fabs(dot(steerDir, fastNormalize(carLinearVel)));
		float lateralSlideCorrectionSpeedModifier = RemapValClamp(fSpeed, 0, 40, 0.0f, 1.0f);
		float lateralSlideSteerFactor = 1.0f - RemapValClamp(lateralSlide, 0.0f, 10.0f, 0.0f, 1.0f);

		fSteeringAngle *= lateralSlideSpeedSteerModifier * lateralSlideCorrectionSpeedModifier;
		doesHardSteer = true;
	}

	FReal accelerator = 1.0f;
	FReal brake = 0.0f;

	FReal distToTarget = length(steeringTargetPos - carPos);
	FReal distToBrakeTarget = length(brakeTargetPos - carPos);
	FReal distToTargetReal = length(m_targInfo.target->GetOrigin() - GetOrigin());

	int controls = IN_ACCELERATE | IN_ANALOGSTEER | IN_EXTENDTURN;

	if(m_sirenEnabled && fabs(fSpeed) > 20)
	{
		controls |= IN_HORN;
	}

	/*
	float pursuitTargetSpeed = m_targInfo.target->GetSpeed();
	
	if(	!m_targInfo.isAngry && distToTargetReal < 14.0f)
	{
		float distFactor = float(distToTarget) / 14.0f;
		
		if(pursuitTargetSpeed > 10.0f)
		{
			accelerator -= distFactor;
			brake += (1.0f-distFactor)*0.5f;
		}
		else
			accelerator *= 0.2f;
			
		accelerator = max(FReal(0),accelerator);

		controls |= IN_BRAKE;
	}*/

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
		float distFromCollPoint = length(m_lastCollidingPosition-GetOrigin());

		if(distFromCollPoint > 5.0f)
			m_blockTimeout = 0.0f;

		m_blockTimeout -= fDt;

		if(m_blockTimeout > 0.0f)
		{
			brake = 1.0f;
			accelerator = 0.0f;
			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;

			//if(frontColl.fract < 1.0f)
			//	steeringTargetPos = frontColl.position * frontColl.normal * 5.0f;
		}
	}

	if(fSpeed > 1.0f)
	{
		Vector3D segmentDir = fastNormalize(steeringTargetPos-steeringTargetPosB);

		float velocityToTargetFactor = dot(segmentDir, fastNormalize(carLinearVel));
		velocityToTargetFactor = pow(fabs(velocityToTargetFactor), 0.5f);

		float lateralSlideSpd = lateralSlide*speedFactor;

		float distToTargetDiff = fSpeed*KPH_TO_MPS * 2.0f - distToBrakeTarget;
		float brakeDistantFactor = RemapValClamp(distToTargetDiff, 0.0f, 10.0f, 0.0f, 1.0f) * weatherBrakeDistModifier;
	
		if((doesHardSteer || speedFactor > 0.05f && (velocityToTargetFactor < 0.85f || speedFactor >= 0.5f && brakeDistantFactor > 0.0f)) )
		{
			//brake = (1.0f - velocityToTargetFactor)*4.0f;

			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;
		}

		doesHardSteer = doesHardSteer || speedFactor >= 0.25f && lateralSlideSpd < 2.2f;

		brake += 1.0f-velocityColl.fract;

		if(lateralSlideSpd > 1.0f)
			brake += (FReal)lateralSlideSpd;

		brake *= FReal( powf(speedFactor, 0.25f)* 6.0f * brakeDistantFactor * pow(1.0f-min(velocityToTargetFactor, 1.0f), 0.25f)*weatherBrakeDistModifier );

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
	if( m_deathTime > 0 && m_conf->visual.sirenType != SERVICE_LIGHTS_NONE && m_pSirenSound )
	{
		m_pSirenSound->SetPitch( m_deathTime / AI_COP_DEATHTIME );
		m_deathTime -= fDt;

		if(m_deathTime <= 0.0f)
		{
			m_sirenEnabled = false;
			SetLight(CAR_LIGHT_SERVICELIGHTS, false);
		}
	}

	int buttons = IN_ANALOGSTEER;
	SetControlButtons( buttons );

	//m_refreshTime = 0.0f;
	//m_thinkTime = 0.0f;

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
