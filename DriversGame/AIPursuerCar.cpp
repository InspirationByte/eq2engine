//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: pursuer car controller AI
//////////////////////////////////////////////////////////////////////////////////

#include "AIPursuerCar.h"
#include "session_stuff.h"

#include "AICarManager.h"

const float AI_COPVIEW_FAR			= 30.0f;
const float AI_COPVIEW_FOV			= 85.0f;

const float AI_COPVIEW_FAR_WANTED	= 70.0f;
const float AI_COPVIEW_FOV_WANTED	= 90.0f;

const float AI_COPVIEW_RADIUS			= 10.0f;
const float AI_COPVIEW_RADIUS_WANTED	= 18.0f;

const float AI_COPVIEW_RADIUS_PURSUIT	= 120.0f;

const float AI_COP_DEATHTIME	= 7.0f;
const float AI_COP_BLOCK_DELAY	= 2.0f;

const float AI_COP_CHECK_MAXSPEED = 65.0f;

const float AI_COP_MINFELONY			= 0.1f;

const float AI_COP_COLLISION_FELONY			= 0.06f;
const float AI_COP_COLLISION_FELONY_VEHICLE	= 0.1f;
const float AI_COP_COLLISION_FELONY_DEBRIS	= 0.02f;
const float AI_COP_COLLISION_FELONY_REDLIGHT = 0.005f;

const float AI_COP_COLLISION_CHECKTIME		= 0.01f;

const float AI_COP_TIME_FELONY = 0.001f;	// 0.1 percent per second

const float AI_COP_TIME_TO_LOST_TARGET = 30.0f;

const float AI_COP_TIME_TO_UPDATE_PATH = 2.0f;	// every 2 seconds

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
	1.2f,
	1.45f
};

//------------------------------------------------------------------------------------------------

IVector2D GetDirectionVec(int dirIdx);

//------------------------------------------------------------------------------------------------

CAIPursuerCar::CAIPursuerCar() : CAITrafficCar(NULL)
{
	memset(&m_targInfo, 0, sizeof(m_targInfo));
}

CAIPursuerCar::CAIPursuerCar(carConfigEntry_t* carConfig, EPursuerAIType type) : CAITrafficCar(carConfig)
{
	memset(&m_targInfo, 0, sizeof(m_targInfo));

	m_taunts = NULL;
	m_type = type;
	m_isColliding = false;
}

CAIPursuerCar::~CAIPursuerCar()
{

}

void CAIPursuerCar::InitAI(CLevelRegion* reg, levroadcell_t* cell)
{
	BaseClass::InitAI(reg, cell);
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

		m_hornTime.Set(0.7f, 0.5f);

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
		m_taunts->StopSound();
		m_taunts->StartSound();

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

void CAIPursuerCar::BeginPursuit()
{
	if (!m_targInfo.target)
		return;

	m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

	// restore collision
	GetPhysicsBody()->SetCollideMask(COLLIDEMASK_VEHICLE);
	GetPhysicsBody()->SetMinFrameTime(0.0f);
	GetPhysicsBody()->Wake();

	m_autohandbrake = true;
	m_frameSkip = false;

	if(m_type == PURSUER_TYPE_COP)
	{
		if (m_conf->m_sirenType != SIREN_NONE)
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

	SetTorqueScale(g_pAIManager->GetCopAccelerationModifier());

	m_enabled = true;

	AI_SetState(&CAIPursuerCar::PursueTarget);

	Msg("BeginPursuit OK\n");
}

void CAIPursuerCar::EndPursuit(bool death)
{
	if (!m_targInfo.target)
		return;

	if (!death)
		m_sirenEnabled = false;

	m_autohandbrake = false;
	m_frameSkip = true;

	if (GetCurrentStateType() != GAME_STATE_GAME)
	{
		m_targInfo.target = NULL;
		return;
	}

	if (m_targInfo.target->GetPursuedCount() == 0)
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

	if(m_taunts)
		m_taunts->StopSound();

	m_targInfo.target = NULL;

	AI_SetState(&CAIPursuerCar::SearchForRoad);

	SetTorqueScale(1.0f);
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

	// check collision
	for(int i = 0; i < car->GetPhysicsBody()->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = car->GetPhysicsBody()->m_collisionList[i];

		if(car->m_lastCollidingObject == pair.bodyB)
			continue;

		if(pair.bodyB->m_flags & COLLOBJ_ISGHOST)
			continue;

		car->m_lastCollidingObject = pair.bodyB;

		if(pair.bodyB == GetPhysicsBody())
			continue;

		if(pair.bodyB->GetContents() == OBJECTCONTENTS_SOLID_GROUND)
			return INFRACTION_NONE;
		else if(pair.bodyB->GetContents() == OBJECTCONTENTS_VEHICLE)
			return INFRACTION_HIT_VEHICLE;
		else if(pair.bodyB->GetContents() == OBJECTCONTENTS_DEBRIS)
			return INFRACTION_HIT_MINOR;
		else
			return INFRACTION_HIT;
	}

	straight_t straight = g_pGameWorld->m_level.GetStraightAtPos(car->GetOrigin());

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

		if (distToStop < 0 && (trafficLightDir == curDir))
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

	if (obj->GetFelony() >= AI_COP_MINFELONY)
	{
		farPlane = AI_COPVIEW_FAR_WANTED;
		fov = AI_COPVIEW_FOV_WANTED;
		visibilitySphere = AI_COPVIEW_RADIUS_WANTED;
	}

	if(obj->GetPursuedCount() > 0)
		visibilitySphere = AI_COPVIEW_RADIUS_PURSUIT;

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
	// do nothing
	if(transition != STATE_TRANSITION_NONE)
		return 0;

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

	Vector3D	carPos		= GetOrigin() + GetForwardVector()*m_conf->m_body_size.z*0.5f;
	Vector3D	targetPos	= m_targInfo.target->GetOrigin() + m_targInfo.target->GetVelocity()*0.5f;
	Vector3D	carDir		= GetForwardVector();

	float		fSpeed = GetSpeedWheels();

	bool doesHaveStraightPath = true;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject( GetPhysicsBody() );
	collFilter.AddObject( m_targInfo.target->GetPhysicsBody() );

	// test for the straight path and visibility
	{
		// Trace convex car
		CollisionData_t coll;

		// if has any collision then deny
		if(g_pPhysics->TestConvexSweep(GetPhysicsBody()->GetBulletShape(), GetOrientation(), carPos, targetPos, coll, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter))
		{
			if(coll.fract < 1.0f)
				doesHaveStraightPath = false;
		}

		debugoverlay->Line3D(carPos, lerp(carPos,targetPos,coll.fract), ColorRGBA(1), ColorRGBA(1));
	}

	//-------------------------------------------------------------------------------
	// refresh the navigation path if we see the target

	m_targInfo.nextPathUpdateTime -= fDt;

	if (m_targInfo.nextPathUpdateTime < 0 && !doesHaveStraightPath)
	{
		m_targInfo.nextPathUpdateTime = AI_COP_TIME_TO_UPDATE_PATH;

		pathFindResult_t newPath;

		if(g_pGameWorld->m_level.Nav_FindPath(targetPos, carPos, newPath, 1024, true) && newPath.points.numElem() > 1)
		{
			m_targInfo.path = newPath;
			m_targInfo.pathTargetIdx = 0;

			Vector3D lastLinePos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[0]);

			for (int i = 1; i < m_targInfo.path.points.numElem(); i++)
			{
				Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[i]);

				debugoverlay->Box3D(pointPos - 0.15f, pointPos + 0.15f, ColorRGBA(1, 1, 0, 1.0f), 1.0f);
				debugoverlay->Line3D(lastLinePos, pointPos, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), 1.0f);

				lastLinePos = pointPos;
			}
		}
	}

	Vector3D steeringTargetPos = targetPos;
	Vector3D brakeTargetPos = targetPos + GetForwardVector();

	// Trace convex car
	CollisionData_t velocityColl;

	// if has any collision then deny
	g_pPhysics->TestConvexSweep(GetPhysicsBody()->GetBulletShape(), GetOrientation(), carPos, carPos+GetVelocity(), velocityColl, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter);

	// Trace convex car
	CollisionData_t frontColl;

	float frontCollDist = RemapValClamp(fSpeed, 0, 40, 1.0f, 4.0f);

	// if has any collision then deny
	g_pPhysics->TestConvexSweep(GetPhysicsBody()->GetBulletShape(), GetOrientation(), carPos, carPos+GetForwardVector()*frontCollDist, frontColl, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE, &collFilter);


	//-------------------------------------------------------------------------------
	// calculate the steering

	float lateralSlideSteerFactor = 1.0f;
	bool doesHardSteer = false;
	float lateralSlide = fabs(GetLateralSlidingAtBody());

	Vector3D steeringTargetPosB = carPos;

	if( !doesHaveStraightPath &&
		m_targInfo.path.points.numElem() > 0 &&
		m_targInfo.pathTargetIdx != -1 && 
		m_targInfo.pathTargetIdx < m_targInfo.path.points.numElem()-1)
	{
		Vector3D currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx]);
		Vector3D nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_targInfo.path.points[m_targInfo.pathTargetIdx + 1]);

		float currPosPerc = lineProjection(currPointPos, nextPointPos, carPos);

		float len = length(currPointPos - nextPointPos);

		float speedModifier = RemapValClamp(60.0f - fSpeed, 0.0f, 60.0f, 4.0f, 8.0f);

		int pathIdx = m_targInfo.pathTargetIdx;

		steeringTargetPos = GetAdvancedPointByDist(m_targInfo.pathTargetIdx, currPosPerc*len+4.0f);

		pathIdx = m_targInfo.pathTargetIdx;
		brakeTargetPos = GetAdvancedPointByDist(pathIdx, currPosPerc*len+15.0f);

		//float speedFactor = RemapValClamp(fSpeed, 0.0f, 70.0f, 0.0f, 1.0f);
		float speedFactor = RemapValClamp(fSpeed, 0.0f, 100.0f, 0.0f, 1.0f);

		steeringTargetPos = lerp(steeringTargetPos, brakeTargetPos, speedFactor);

		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosStart = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 25.0f*speedFactor);

		pathIdx = m_targInfo.pathTargetIdx;
		Vector3D hardSteerPosEnd = GetAdvancedPointByDist(pathIdx, currPosPerc*len + 55.0f*speedFactor);


		debugoverlay->Box3D(steeringTargetPos - 0.25f, steeringTargetPos + 0.25f, ColorRGBA(1, 0, 0, 1.0f), 0.0f);
		debugoverlay->Box3D(brakeTargetPos - 0.25f, brakeTargetPos + 0.25f, ColorRGBA(0, 1, 0, 1.0f), 0.0f);

		if(fabs(fSpeed) > 20.0f)
		{
			Vector3D steerDirHard = fastNormalize(hardSteerPosEnd-hardSteerPosStart);

			float cosHardSteerAngle = dot(steerDirHard, fastNormalize(GetVelocity()));

			if(cosHardSteerAngle < 0.65f)
			{
				lateralSlideSteerFactor = 1.0f - RemapValClamp(lateralSlide, 0.0f, 18.0f, 0.0f, 1.0f);

				steeringTargetPosB = hardSteerPosStart;
				steeringTargetPos = hardSteerPosEnd;
				doesHardSteer = true;
			}
		}
	}

	GetPhysicsBody()->TryWake(false);

	Matrix4x4 bodyMat;
	GetPhysicsBody()->ConstructRenderMatrix( bodyMat );

	Vector3D steerDir = fastNormalize((!bodyMat.getRotationComponent()) * fastNormalize(steeringTargetPos-steeringTargetPosB));

	FReal fSteerTarget = atan2(steerDir.x, steerDir.z);

	if(fabs(fSpeed) > 40.0f && sign(lateralSlide) != sign(fSteerTarget))
		fSteerTarget *= lateralSlideSteerFactor;

	FReal carForwardSpeed = dot(GetForwardVector(), GetVelocity());

	FReal fSteeringAngle = clamp(fSteerTarget*0.5f, -1.0f, 1.0f);

	FReal accelerator = 1.0f;
	FReal brake = 0.0f;

	FReal distToTarget = length(steeringTargetPos - carPos);
	FReal distToTargetReal = length(m_targInfo.target->GetOrigin() - carPos);

	// apply acceleration modifier if i'm too far
	if (distToTarget > 60.0f && fabs(fSteeringAngle) < 0.25f)
		accelerator = g_pAIManager->GetCopAccelerationModifier();

	int controls = IN_ACCELERATE | IN_ANALOGSTEER | IN_EXTENDTURN;

	if(m_sirenEnabled && fabs(fSpeed) > 20)
	{
		controls |= IN_HORN;
	}

	FReal distFromCollPoint = length(m_lastCollidingPosition-GetOrigin());

	if (distToTargetReal < 4.0f || distFromCollPoint < 4.0f || m_isColliding)
	{
		accelerator = 0.5f;

		Plane pl(carDir, dot(carDir, GetOrigin()));

		if (m_isColliding && carForwardSpeed < 10.0f && m_blockingTime > 1.0f)
			m_blockingTime -= fDt;

		if ( (m_blockingTime < 0.0f || frontColl.fract < 1.0f) && pl.Distance(m_lastCollidingPosition) > 0 )
		{
			brake = 1.0f;
			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;
		}
	}
	else
		m_blockingTime = AI_COP_BLOCK_DELAY;

	float pursuitTargetSpeed = m_targInfo.target->GetSpeed();

	if(	!m_targInfo.isAngry && 
		distToTargetReal < 14.0f)
	{
		float distFactor = float(distToTarget) / 14.0f;

		if(pursuitTargetSpeed > 10.0f)
		{
			accelerator -= distFactor;
			brake += (1.0f-distFactor)*0.5f;
		}
		else
			accelerator *= 0.2f;

		accelerator = max(0,accelerator);

		controls |= IN_BRAKE;
	}

	if(frontColl.fract < 1.0f)
	{
		steeringTargetPos += frontColl.normal*(1.0f-frontColl.fract)*5.0f;
	}

	if(fabs(fSpeed) > 40.0f)
	{
		Vector3D segmentDir = fastNormalize(brakeTargetPos-steeringTargetPos);

		float steerBrakeAngle = dot(segmentDir, normalize(GetVelocity()));

		if( steerBrakeAngle < 0.45f )
		{
			brake = 1.0f-clamp(steerBrakeAngle, 0.0f, 0.5f);
			controls |= IN_BRAKE;
			controls &= ~IN_ACCELERATE;
		}

		brake += 1.0f-velocityColl.fract;
		brake += fabs( GetLateralSlidingAtBody()*0.25f );
	}

	if( fabs(fSteerTarget) >= 1.0f && carForwardSpeed > 10.0f )
	{
		brake = 1.0f;
		accelerator = 0.0f;
		controls |= IN_BRAKE;
		controls &= ~IN_ACCELERATE;
	}
	else if(fSpeed > 10.0f)
	{
		accelerator -= fabs(fSteeringAngle)*0.5f;
	}

	if(GetSpeedWheels() < -1.0f)
		fSteeringAngle *= -1.0f;

	//m_autohandbrake = doesHardSteer;

	SetControlButtons( controls );
	SetControlVars(accelerator, brake, clamp(fSteeringAngle, -1.0f, 1.0f));

	return 0;
}

int	CAIPursuerCar::DeadState( float fDt, EStateTransition transition )
{
	if( m_deathTime > 0 && m_conf->m_sirenType != SIREN_NONE && m_pSirenSound )
	{
		m_pSirenSound->SetPitch( m_deathTime / AI_COP_DEATHTIME );
		m_deathTime -= fDt;
	
		if(m_deathTime <= 0.0f)
			m_sirenEnabled = false;
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

	if(obj)
		m_targInfo.isAngry = (obj->GetFelony() > 0.6f);
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