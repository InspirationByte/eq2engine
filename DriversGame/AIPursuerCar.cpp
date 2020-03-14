//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: pursuer car controller AI
//////////////////////////////////////////////////////////////////////////////////

#include "AIPursuerCar.h"
#include "session_stuff.h"

#include "object_debris.h"

#include "AIManager.h"

#include "world.h"
#include "input.h"

ConVar ai_debug_pursuer("ai_debug_pursuer", "0", NULL, CV_CHEAT);
ConVar g_railroadCops("g_railroadCops", "0", NULL, CV_CHEAT);


// TODO: make these constants initialized from Lua

const float AI_COP_BEGINPURSUIT_ARMED_DELAY		= 0.15f;
const float AI_COP_BEGINPURSUIT_PASSIVE_DELAY	= 0.4f;

const float AI_COPVIEW_FOV			= 85.0f;
const float AI_COPVIEW_FOV_WANTED	= 90.0f;

const float AI_COPVIEW_FAR_ROADBLOCK	= 10.0f;

const float AI_COPVIEW_RADIUS			= 18.0f;
const float AI_COPVIEW_RADIUS_WANTED	= 25.0f;
const float AI_COPVIEW_RADIUS_PURSUIT	= 100.0f;
const float AI_COPVIEW_RADIUS_ROADBLOCK = 15.0f;

const float AI_COP_CHECK_MAXSPEED = 80.0f; // 80 kph and you will be pursued

const float AI_COP_MINFELONY			= 0.1f;
const float AI_COP_MINFELONY_CHECK		= 0.05f;

const float AI_COP_COLLISION_FELONY			= 0.06f;
const float AI_COP_COLLISION_FELONY_VEHICLE	= 0.15f;
const float AI_COP_COLLISION_FELONY_DEBRIS	= 0.02f;
const float AI_COP_COLLISION_FELONY_REDLIGHT = 0.010f;

const float AI_COP_COLLISION_CHECKTIME		= 0.01f;

const float AI_COP_TIME_TO_LOST_TARGET		= 30.0f;
const float AI_COP_TIME_TO_LOST_TARGET_FAR	= 5.0f;
const float AI_COP_TIME_TO_TELL_DIRECTION	= 10.0f;

const float AI_COP_LOST_TARGET_FARDIST		= 160.0f;

const float AI_COP_BECOME_ANGRY_PURSUIT_TIME = 60.0f;
const float AI_COP_BECOME_ANGRY_FELONY		 = 0.65f;

const float AI_COP_ROADBLOCK_WIDTH			= 10.0f;

const float AI_ALTER_SIREN_MIN_SPEED		= 65.0f;
const float AI_ALTER_SIREN_CHANGETIME		= 2.0f;

const float AI_CHASER_MAX_DISTANCE = 50.0f;

const float AI_ANGRY_ACTIVE_TIME = 5.0f;

const float AI_ANGRY_ACTIVE_DELAY = AI_ANGRY_ACTIVE_TIME + 5.0f;

//------------------------------------------------------------------------------------------------

struct InfractionDesc
{
	const char* speech;

	float passiveFelony;
	float activeFelony;

	float timeInterval;
};

const InfractionDesc g_infractions[INFRACTION_COUNT] =
{
	//INFRACTION_NONE
	{nullptr, 0.0f, 0.0f, 1.0f},

	//INFRACTION_HAS_FELONY
	{nullptr, 0.0f, 0.0f, 0.0f},

	//INFRACTION_MINOR
	{nullptr, 0.0f, 0.0f, 0.0f},

	//INFRACTION_SPEEDING
	{nullptr, 0.1f, 0.01f, 1.0f},

	//INFRACTION_RED_LIGHT
	{"cop.redlight", 0.1f, AI_COP_COLLISION_FELONY_REDLIGHT, 1.0f},

	//INFRACTION_WRONG_LANE
	{nullptr, 0.1f, 0.02f, 5.0f},

	//INFRACTION_HIT_MINOR
	{nullptr, AI_COP_COLLISION_FELONY_DEBRIS, AI_COP_COLLISION_FELONY_DEBRIS, 0.1f},

	//INFRACTION_HIT
	{nullptr, AI_COP_COLLISION_FELONY, AI_COP_COLLISION_FELONY, 1.0f},

	//INFRACTION_HIT_VEHICLE
	{"cop.hitvehicle", AI_COP_COLLISION_FELONY_VEHICLE, AI_COP_COLLISION_FELONY_VEHICLE, 1.0f},

	//INFRACTION_HIT_SQUAD_VEHICLE
	{nullptr, AI_COP_COLLISION_FELONY_VEHICLE, 0.02f, 1.0f},
};

//------------------------------------------------------------------------------------------------
// utility
CAIPursuerCar* UTIL_CastToPursuer(CCar* car)
{
	if (car->ObjType() != GO_CAR_AI)
		return nullptr;

	CAITrafficCar* trafficCar = (CAITrafficCar*)car;

	if (!trafficCar->IsPursuer())
		return nullptr;

	return (CAIPursuerCar*)trafficCar;
}

//------------------------------------------------------------------------------------------------

CAIPursuerCar::CAIPursuerCar() : CAITrafficCar(nullptr)
{
	m_target = nullptr;
}

CAIPursuerCar::CAIPursuerCar(vehicleConfig_t* carConfig, EPursuerAIType type) : CAITrafficCar(carConfig)
{
	m_target = nullptr;
	m_angry = false;
	m_angryTimer = 0.0f;
	m_pursuitTime = 0.0f;

	m_loudhailer = nullptr;
	m_type = type;

	m_alterSirenChangeTime = AI_ALTER_SIREN_CHANGETIME;
	m_sirenAltered = false;

	m_savedMaxSpeed = carConfig->physics.maxSpeed;
	m_savedTorqueScale = 1.0f;
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
		m_loudhailer = g_sounds->CreateSoundController(&ep);
	}

	SetMaxDamage( g_pAIManager->GetCopMaxDamage() );
}

void CAIPursuerCar::OnRemove()
{
	EndPursuit(true);

	if (m_loudhailer)
		g_sounds->RemoveSoundController(m_loudhailer);

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
	PrecacheScriptSound("cop.takehimup");
	PrecacheScriptSound("cop.still_in_pursuit");
	PrecacheScriptSound("cop.heading_west");
	PrecacheScriptSound("cop.heading_east");
	PrecacheScriptSound("cop.heading_south");
	PrecacheScriptSound("cop.heading_north");
	PrecacheScriptSound("cop.taunt");
	PrecacheScriptSound("cop.check");
	PrecacheScriptSound("cop.squad_car_hit");
	PrecacheScriptSound("cop.squad_car_down");
	
	

	BaseClass::Precache();
}

void CAIPursuerCar::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	BaseClass::OnCarCollisionEvent(pair, hitBy);

	if (InPursuit())
	{
		m_hornSequencer.ShutUp();

		if(m_angry && hitBy == m_target)	// reset angry timer
			m_angryTimer = AI_ANGRY_ACTIVE_TIME;
	}
		
}

bool CAIPursuerCar::Speak(const char* soundName, CCar* target, bool force, float priority)
{
	if (!soundName)
		return false;

	if (m_type == PURSUER_TYPE_GANG)
		return false;

	if (g_pGameSession->GetPlayerCar() != target)
		return false;

	return g_pAIManager->MakeCopSpeech(soundName, force, priority);
}

void CAIPursuerCar::DoPoliceLoudhailer()
{
	if (m_type == PURSUER_TYPE_GANG)
		return;

	// TODO: actually check if i'm the nearest cop in the universe
	if (g_pAIManager->IsCopsCanUseLoudhailer(this, m_target))
	{
		m_loudhailer->Stop();
		m_loudhailer->Play();

		g_pAIManager->CopLoudhailerTold();
	}
}

void CAIPursuerCar::OnPrePhysicsFrame( float fDt )
{
	BaseClass::OnPrePhysicsFrame(fDt);

	UpdateTarget(fDt);

	if(	IsAlive() && !m_enabled)
	{
		DkList<CGameObject*> nearestCars;
		g_pGameWorld->QueryObjects(nearestCars, AI_COPVIEW_RADIUS_PURSUIT, GetOrigin(), nullptr, [](CGameObject* x, void*) {
			return ((x->ObjType() == GO_CAR));
		});

		nearestCars.fastRemove(this);

		for (int i = 0; i < nearestCars.numElem(); i++)
		{
			CCar* checkCar = (CCar*)nearestCars[i];

			if (CheckObjectVisibility(checkCar) &&
				checkCar->GetFelony() > AI_COP_MINFELONY &&
				checkCar->GetPursuedCount() == 0)
			{
				SetPursuitTarget(checkCar);
				BeginPursuit(0.0f);
			}
		}
	}
}

void CAIPursuerCar::OnPhysicsFrame( float fDt )
{
	BaseClass::OnPhysicsFrame(fDt);

	if(IsAlive())
	{
		// update blocking
		m_collAvoidance.m_manipulator.m_isColliding = m_collisionList.numElem() > 0;

		if (!m_collAvoidance.m_manipulator.m_enabled && m_collAvoidance.m_manipulator.m_isColliding && !m_collAvoidance.m_manipulator.m_collidingPositionSet)
		{
			m_collAvoidance.m_manipulator.m_lastCollidingPosition = m_collisionList[0].position;
			m_collAvoidance.m_manipulator.m_collidingPositionSet = true;
		}
		else if(!m_collAvoidance.m_manipulator.m_enabled)
		{
			m_collAvoidance.m_manipulator.m_collidingPositionSet = false;
		}

		if (!g_pGameWorld->IsValidObject(m_target))
			m_target = nullptr;

		// update target infraction
		if(!m_target)
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
	DkList<CGameObject*> nearestCars;
	g_pGameWorld->QueryObjects(nearestCars, AI_COPVIEW_RADIUS_PURSUIT, GetOrigin(), nullptr, [](CGameObject* x, void*) {
		return ((x->ObjType() == GO_CAR));
	});

	nearestCars.fastRemove(this);

	for (int i = 0; i < nearestCars.numElem(); i++)
	{
		// TODO: add other player cars, not just one
		CCar* checkCar = (CCar*)nearestCars[i];

		if (m_target == checkCar)
			continue;

		// check infraction in visible range
		if (!CheckObjectVisibility(checkCar))
			continue;

		float pursuitStartDelay = checkCar->GetFelony() > AI_COP_MINFELONY ? AI_COP_BEGINPURSUIT_ARMED_DELAY : AI_COP_BEGINPURSUIT_PASSIVE_DELAY;

		// gang just starts pursuing
		if (m_type == PURSUER_TYPE_GANG)
		{
			if (checkCar->IsEnabled())
			{
				SetPursuitTarget(checkCar);
				BeginPursuit(pursuitStartDelay);
			}
			return 0;
		}

		// register infractions
		UpdateInfractions(checkCar, true);

		PursuerData_t& pursuerData = checkCar->GetPursuerData();

		float newFelony = checkCar->GetFelony();

		// go through all infraction types
		for (int j = 0; j < INFRACTION_COUNT; j++)
		{
			if (!pursuerData.hasInfraction[j])
				continue;

			pursuerData.hasInfraction[j] = false;

			const InfractionDesc& infractionDesc = g_infractions[j];

			newFelony += infractionDesc.passiveFelony;
		}

		if (newFelony >= AI_COP_MINFELONY)
		{
			SetPursuitTarget(checkCar);
			BeginPursuit(pursuitStartDelay);
		}
		else if (newFelony >= AI_COP_MINFELONY_CHECK)
		{
			// speak shit
			if (!pursuerData.copsHasAttention && !pursuerData.announced)
			{
				pursuerData.copsHasAttention = true;
				Speak("cop.check", checkCar, false, 0.5f);
			}
		}

		checkCar->SetFelony(newFelony);
	}

	return 0;
}

void CAIPursuerCar::BeginPursuit( float delay )
{
	if (!m_target)
		return;

	if(!IsAlive() || !m_target->m_conf->flags.isCar)
	{
		SetPursuitTarget(nullptr);
		return;
	}

	m_lastSeenTargetTimer = 0.0f;

	PursuerData_t& targetPursuerData = m_target->GetPursuerData();

	// announce pursuit for cops
	if (m_type == PURSUER_TYPE_COP && 
		!targetPursuerData.announced)
	{
		float targetFelony = m_target->GetFelony();

		if (targetFelony >= AI_COP_MINFELONY)
		{
			int val = RandomInt(0, 1);

			if (val)
				Speak("cop.pursuit_continue", m_target, true);
			else
				SpeakTargetDirection("cop.takehimup", true);
		}
		else
		{
			Speak("cop.pursuit", m_target, true);
		}
	}

	targetPursuerData.announced = true;

	AI_SetNextState(&CAIPursuerCar::PursueTarget, delay);
}

void CAIPursuerCar::EndPursuit(bool death)
{
	if (!m_target)
		return;

	if(m_loudhailer)
		m_loudhailer->Stop();

	m_sirenAltered = false;

	// HACK: just kill
	if (EqStateMgr::GetCurrentStateType() != GAME_STATE_GAME)
	{
		SetPursuitTarget(nullptr);
		return;
	}

	if (!death)
	{
		m_autohandbrake = false;
		m_sirenEnabled = false;
		SetLight(CAR_LIGHT_SERVICELIGHTS, false);

		AI_SetState(&CAIPursuerCar::SearchForRoad);
	}
	else
	{
		AI_SetState(&CAIPursuerCar::DeadState);
	}

	if (m_target != NULL)
	{
		// validate and remove
		if (g_pGameWorld->IsValidObject(m_target) && m_target->GetPursuedCount() == 1)
		{
			Speak("cop.lost", m_target, true);
		}

		SetPursuitTarget(nullptr);
	}	
}

bool CAIPursuerCar::InPursuit() const
{
	return m_target && FSMGetCurrentState() == &CAIPursuerCar::PursueTarget;
}

EInfractionType CAIPursuerCar::CheckTrafficInfraction(CCar* car, bool checkFelony, bool checkSpeeding )
{
	if (!car)
		return INFRACTION_NONE;

	if(!car->IsEnabled())
		return INFRACTION_NONE;

	if(!car->m_conf->flags.isCar)
		return INFRACTION_NONE;

	float carSpeed = car->GetSpeed();

	// ho!
	if (checkFelony && car->GetFelony() >= AI_COP_MINFELONY)
	{
		return INFRACTION_HAS_FELONY;
	}

	if (checkSpeeding && carSpeed > AI_COP_CHECK_MAXSPEED)
	{
		return INFRACTION_SPEEDING;
	}

	DkList<CollisionPairData_t>& collisionList = car->m_collisionList;

	// don't register other infractions if speed less than 5 km/h
	if(carSpeed < 5.0f)
		return INFRACTION_NONE;

	Vector3D car_vel = car->GetVelocity();
	Vector3D car_forward = car->GetForwardVector();
	float car_wheelSpeed = car->GetSpeedWheels();

	// check collision
	for(int i = 0; i < collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = collisionList[i];

		if (pair.flags & COLLPAIRFLAG_USER_PROCESSED2)
			continue;

		pair.flags |= COLLPAIRFLAG_USER_PROCESSED2;

		CEqCollisionObject* bodyB = pair.GetOppositeTo(car->GetPhysicsBody());

		if(bodyB->m_flags & COLLOBJ_ISGHOST)
			continue;

		int contents = bodyB->GetContents();

		if(contents == OBJECTCONTENTS_SOLID_GROUND)
			return INFRACTION_NONE;
		else if(contents == OBJECTCONTENTS_VEHICLE)
		{
			CCar* hitCar = (CCar*)pair.GetOppositeTo(car->GetPhysicsBody())->GetUserData();

			if(InPursuit() && hitCar == this)
				return INFRACTION_NONE;

			if(hitCar == car->GetHingedVehicle())
				return INFRACTION_NONE;

			float speedDiff = hitCar->GetSpeed() - carSpeed;

			// There is no infraction if bodyB has inflicted this damage to us
			//if(carSpeed < hitCar->GetSpeed())
			//	return INFRACTION_NONE;

			if (hitCar->ObjType() == GO_CAR_AI)
			{
				CAITrafficCar* tfc = (CAITrafficCar*)hitCar;
				if (tfc->IsPursuer() && tfc->m_conf->flags.isCop && !tfc->m_assignedRoadblock)	// don't check collision with me pls
				{
					return INFRACTION_HIT_SQUAD_VEHICLE;
				}
			}

			return INFRACTION_HIT_VEHICLE;
		}
		else if(contents == OBJECTCONTENTS_DEBRIS)
		{
			CObject_Debris* obj = (CObject_Debris*)bodyB->GetUserData();

			if(obj && obj->IsSmashed())
				continue;

			return INFRACTION_HIT_MINOR;
		}
		else
		{
			return INFRACTION_HIT;
		}
			
	}

	straight_t straight = g_pGameWorld->m_level.Road_GetStraightAtPos(car->GetOrigin(), 2);

	// Check wrong direction
	if ( straight.direction != -1)
	{
		Vector3D	startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight.start);
		Vector3D	endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(straight.end);

		Vector3D	roadDir = fastNormalize(startPos - endPos);

		// make sure he's running in the right direction
		if (dot(car->GetForwardVector(), roadDir) > 0)
		{
			return INFRACTION_WRONG_LANE;
		}
	}

	// only if we going on junction
	levroadcell_t* cell_checkRed = g_pGameWorld->m_level.Road_GetGlobalTile(car->GetOrigin());

	Vector3D redCrossingCheckPos = car->GetOrigin() - car->GetForwardVector()*12.0f;
	straight_t straight_checkRed = g_pGameWorld->m_level.Road_GetStraightAtPos( redCrossingCheckPos, 2 );

	// Check red light crossing
	if( cell_checkRed && IsJunctionOrPavementType((ERoadType)cell_checkRed->type) &&
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
		{
			return INFRACTION_RED_LIGHT;
		}
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

void CAIPursuerCar::UpdateInfractions(CCar* car, bool passive)
{
	int infraction = CheckTrafficInfraction(car, passive, passive);

	PursuerData_t& pursuer = car->GetPursuerData();

	float gameTime = g_pGameSession->GetGameTime();

	if (infraction > INFRACTION_HAS_FELONY)
	{
		const InfractionDesc& infractionDesc = g_infractions[infraction];

		if (gameTime > pursuer.lastInfractionTime[infraction])
		{
			pursuer.lastInfractionTime[infraction] = gameTime + infractionDesc.timeInterval;
			pursuer.hasInfraction[infraction] = true;
		}
	}
}

bool CAIPursuerCar::UpdateTarget(float fDt)
{
	if (!m_target)
		return false;

	PursuerData_t& pursuerData = m_target->GetPursuerData();

	float targetSpeed = m_target->GetSpeed();

	bool isVisible = CheckObjectVisibility(m_target);

	// check the visibility
	if (isVisible)
	{
		// update felony and shit
		if (m_type == PURSUER_TYPE_COP)
		{
			// register infractions
			UpdateInfractions(m_target, false);

			float newFelony = m_target->GetFelony();

			// go through all infraction types
			for (int i = 0; i < INFRACTION_COUNT; i++)
			{
				if (!pursuerData.hasInfraction[i])
					continue;

				pursuerData.hasInfraction[i] = false;

				const InfractionDesc& infractionDesc = g_infractions[i];

				newFelony += infractionDesc.activeFelony;
				Speak(infractionDesc.speech, m_target, false, 0.8f);
			}

			// tell about running a roadblock
			for (int i = 0; i < g_pAIManager->m_roadBlocks.numElem(); i++)
			{
				RoadBlockInfo_t* rblock = g_pAIManager->m_roadBlocks[i];

				if (rblock && !rblock->runARoadblock)
				{
					Vector3D roadBlockDir = normalize(rblock->roadblockPosA - rblock->roadblockPosB);
					roadBlockDir = cross(roadBlockDir, vec3_up);

					Plane roadBlockPlane(roadBlockDir, -dot(roadBlockDir, rblock->roadblockPosA));

					float distToRoadBlock = roadBlockPlane.Distance(m_target->GetOrigin());

					float sidePositionFac = lineProjection(rblock->roadblockPosA, rblock->roadblockPosB, m_target->GetOrigin());

					// only half of width of roadblock
					if (sidePositionFac >= -0.5f && sidePositionFac <= 1.5f)
					{
						if (!rblock->targetEnteredRoadblock && fabs(distToRoadBlock) < AI_COP_ROADBLOCK_WIDTH)
						{
							rblock->targetEnteredRoadblock = m_target;
							rblock->targetEnteredSign = sign(distToRoadBlock);
						}
						else if (rblock->targetEnteredRoadblock && fabs(distToRoadBlock) > AI_COP_ROADBLOCK_WIDTH)
						{
							if (sign(distToRoadBlock) + rblock->targetEnteredSign)
								rblock->runARoadblock = true;

							rblock->runARoadblock = true;

							if (targetSpeed > 4.0f)
								Speak("cop.roadblock", m_target, true);
						}
					}
				}
			}

			m_target->SetFelony(newFelony);

			if (m_loudhailer)
			{
				m_loudhailer->SetOrigin(GetOrigin());
				m_loudhailer->SetVelocity(GetVelocity());

				DoPoliceLoudhailer();
			}


			if (!IsFlippedOver())
			{
				bool isAboutToLoose = (pursuerData.lastSeenTimer > AI_COP_TIME_TO_LOST_TARGET_FAR*0.25f);

				if (targetSpeed > 50.0f)
				{
					const char* speech = isAboutToLoose ? "cop.takehimup" : "cop.heading";
					SpeakTargetDirection(speech, m_target);
				}

				if (isAboutToLoose)
					Speak("cop.pursuit_continue", m_target, false, 0.5f);
				else if (m_pursuitTime > AI_COP_BECOME_ANGRY_PURSUIT_TIME * 0.5f)
					Speak("cop.still_in_pursuit", m_target, false, 0.0f);
			}

			if (newFelony > AI_COP_BECOME_ANGRY_FELONY)
				m_angry = true;
		}

		// for gangs and cops it's equal. Too much pursuit time makes me angry.
		if (m_pursuitTime > AI_COP_BECOME_ANGRY_PURSUIT_TIME)
			m_angry = true;

		pursuerData.lastSeenTimer = 0.0f;
		m_lastSeenTargetTimer = 0.0f;
	}
	else // check the timers
	{
		m_lastSeenTargetTimer += fDt;

		float distToTarget = length(m_target->GetOrigin() - GetOrigin());

		float timeToLostTarget = (distToTarget > AI_COP_LOST_TARGET_FARDIST) ? AI_COP_TIME_TO_LOST_TARGET_FAR : AI_COP_TIME_TO_LOST_TARGET;

		if (pursuerData.lastSeenTimer > timeToLostTarget || m_lastSeenTargetTimer > timeToLostTarget)
		{
			EndPursuit(false);
			return false;
		}
	}

	return true;
}

int	CAIPursuerCar::PursueTarget( float fDt, EStateTransition transition )
{
	bool targetIsValid = (m_target != NULL);

	if(transition == STATE_TRANSITION_PROLOG)
	{
		if(!targetIsValid)
		{
			AI_SetState(&CAIPursuerCar::SearchForRoad);
			return 0;
		}

		if (m_type == PURSUER_TYPE_COP)
		{
			if (m_conf->visual.sirenType != SERVICE_LIGHTS_NONE)
			{
				SetLight(CAR_LIGHT_SERVICELIGHTS, true);
				m_sirenEnabled = true;
			}
		}

		// reset the emergency lights
		SetLight(CAR_LIGHT_EMERGENCY, false);

		// wake physics body and restore update rate
		GetPhysicsBody()->SetMinFrameTime(0.0f);

		GetPhysicsBody()->Wake();

		// apply neat cheat
		if(g_railroadCops.GetBool())
			SetInfiniteMass(true);

		m_enabled = true;
		m_autogearswitch = true;
		m_pursuitTime = 0.0f;
		m_angryTimer = 0.0f;

		m_slipParams = &(slipAngleCurveParams_t&)GetAISlipCurveParams();
		GetPhysicsBody()->SetCenterOfMass(m_conf->physics.virtualMassCenter + Vector3D(0.0f,-0.15f, 0.0f));

		// set desired torque and speed
		m_maxSpeed = m_savedMaxSpeed;
		m_torqueScale = m_savedTorqueScale;
		m_gearboxShiftThreshold = 1.0f;

		return 0;
	}
	else if(transition == STATE_TRANSITION_EPILOG)
	{
		// deactivate neat cheat
		if (g_railroadCops.GetBool())
			SetInfiniteMass(false);

		m_slipParams = &(slipAngleCurveParams_t&)GetDefaultSlipCurveParams();
		GetPhysicsBody()->SetCenterOfMass(m_conf->physics.virtualMassCenter);

		m_autogearswitch = false;

		// restore torque and speed
		m_maxSpeed = m_conf->physics.maxSpeed;
		m_torqueScale = 1.0f;
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

	m_pursuitTime += fDt;

	if(!ai_debug_pursuer.GetBool())
	{
		// don't update AI in replays, unless we have debugging enabled
		if(g_replayTracker->m_state == REPL_PLAYING)
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

	const Vector3D& carPos = GetOrigin();

	const Vector3D& targetPos = m_target->GetOrigin();
	const Vector3D& targetVelocity = m_target->GetVelocity();

	// update navigation affector parameters
	m_nav.m_manipulator.m_driveTarget = targetPos;
	m_nav.m_manipulator.m_driveTargetVelocity = targetVelocity;
	m_nav.m_manipulator.m_excludeColl = m_target->GetPhysicsBody();

	m_chaser.m_manipulator.m_driveTarget = targetPos;
	m_chaser.m_manipulator.m_driveTargetVelocity = targetVelocity;
	m_chaser.m_manipulator.m_excludeColl = m_target->GetPhysicsBody();

	if (m_angry)
	{
		m_angryTimer -= fDt;
		if (m_angryTimer < 0)
			m_angryTimer = AI_ANGRY_ACTIVE_DELAY;
	}

	// update target avoidance affector parameters
	m_targetAvoidance.m_manipulator.m_avoidanceRadius = length(m_target->m_bbox.GetSize());
	m_targetAvoidance.m_manipulator.m_enabled = (m_angryTimer < AI_ANGRY_ACTIVE_TIME);
	m_targetAvoidance.m_manipulator.m_targetPosition = targetPos;
	m_targetAvoidance.m_manipulator.m_targetVelocity = targetVelocity;

	float distToTarget = length(targetPos - GetOrigin());

	bool doesHaveStraightPath = true;

	// test for the straight path and visibility
	if (distToTarget < AI_CHASER_MAX_DISTANCE)
	{
		float tracePercentage = g_pGameWorld->m_level.Nav_TestLine(carPos, targetPos, false);

		if (tracePercentage < 1.0f)
			doesHaveStraightPath = false;
	}
	else
	{
		doesHaveStraightPath = false;
	}

	// do regular updates of ai navigation
	m_nav.Update(this, fDt);
	m_chaser.Update(this, fDt);

	ai_handling_t handling = m_nav.m_handling;
	Vector3D hintTargetPos = m_nav.m_manipulator.m_outSteeringTargetPos;

	if (doesHaveStraightPath)
	{
		m_nav.m_manipulator.ForceUpdatePath();
		handling = m_chaser.m_handling;
		hintTargetPos = m_chaser.m_manipulator.m_outSteeringTargetPos;
		//handling.autoHandbrake = true;
	}

	// make stability control
	m_stability.m_manipulator.m_hintTargetPosition = hintTargetPos;
	m_stability.m_manipulator.m_initialHandling = handling;
	m_stability.Update(this, fDt);

	// collision avoidance
	// take the initial handing for avoidance from navigator
	m_collAvoidance.m_manipulator.m_initialHandling = m_nav.m_handling;
	m_collAvoidance.Update(this, fDt);

	// target avoidance
	m_targetAvoidance.Update(this, fDt);

	// TODO: apply stability control handling amounts to the final handling
	// based on the game difficulty
	handling += m_stability.m_handling;
	handling += m_targetAvoidance.m_handling;

	if (m_collAvoidance.m_manipulator.m_enabled)
		handling = m_collAvoidance.m_handling;

	if(!m_stability.m_handling.autoHandbrake)
		handling.autoHandbrake = false;

	int controls = IN_ACCELERATE | IN_ANALOGSTEER;

	// alternating siren sound by speed
	if(	m_type == PURSUER_TYPE_COP && m_conf->visual.sirenType > 0 )
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
		controls |= IN_FASTSTEER;

	m_autohandbrake = handling.autoHandbrake;

	float steering = sign(handling.steering) * pow(fabs(handling.steering), 1.35f);	// for stabilizing

	SetControlButtons( controls );
	SetControlVars(	handling.acceleration,
					handling.braking,
					steering);

	if(handling.acceleration > 0.05f)
		GetPhysicsBody()->TryWake(false);

	return 0;
}

void CAIPursuerCar::SpeakTargetDirection(const char* startSoundName, bool force)
{
	Vector3D targetVelocity = m_target->GetVelocity();

	float targetAngle = VectorAngles(fastNormalize(targetVelocity)).y + 45;

#pragma todo("North direction as second argument")
	targetAngle = NormalizeAngle360(-targetAngle /*g_pGameWorld->GetLevelNorthDirection()*/);

	int targetDir = targetAngle / 90.0f;

	if (targetDir < 0)
		targetDir += 4;

	if (targetDir > 3)
		targetDir -= 4;

	PursuerData_t& targetPursuerData = m_target->GetPursuerData();

	if (targetPursuerData.lastDirectionTimer > AI_COP_TIME_TO_TELL_DIRECTION)
	{
		if (targetPursuerData.lastDirection != targetDir && targetDir < 4)
		{
			if (Speak(startSoundName, m_target, force))
			{
				if (targetDir == 0)
					Speak("cop.heading_west", m_target, true);
				else if (targetDir == 1)
					Speak("cop.heading_north", m_target, true);
				else if (targetDir == 2)
					Speak("cop.heading_east", m_target, true);
				else if (targetDir == 3)
					Speak("cop.heading_south", m_target, true);

				targetPursuerData.lastDirectionTimer = 0.0f;
			}

			targetPursuerData.lastDirection = targetDir;
		}
	}
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
	if (m_target && g_pGameWorld->IsValidObject(m_target)) 
		m_target->DecrementPursue();

	// restrict to player cars and lead cars
	if (obj && obj->ObjType() != GO_CAR)
	{
		m_target = nullptr;
		return;
	}

	m_target = obj;

	if (m_target && g_pGameWorld->IsValidObject(m_target))
		m_target->IncrementPursue();
}

void CAIPursuerCar::SetMaxSpeed(float fSpeed)
{
	m_savedMaxSpeed = fSpeed;
}

void CAIPursuerCar::SetTorqueScale(float fScale)
{
	m_savedTorqueScale = fScale;
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
