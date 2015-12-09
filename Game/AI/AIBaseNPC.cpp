//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI Base Non-player character
// TODO:	task manager
//			behavior system
//////////////////////////////////////////////////////////////////////////////////

#include "AIBaseNPC.h"
#include "BaseWeapon.h"
#include "GameInput.h"

#include "AIScriptTask.h"

BEGIN_DATAMAP(CAIBaseNPCHost)
	//DEFINE_FIELD(m_pEnemy, VTYPE_ENTITYPTR),
	//DEFINE_FIELD(m_pTarget, VTYPE_ENTITYPTR),
	DEFINE_FIELD(m_vecViewTarget, VTYPE_ORIGIN),
	DEFINE_FIELD(m_vecCurViewTarget, VTYPE_ORIGIN),
END_DATAMAP()

CAIBaseNPCHost::CAIBaseNPCHost() : BaseClass()
{
	//m_pEnemy = NULL;
	//m_pTarget = NULL;
	m_nMoveYaw = -1;
	m_nLookPitch = -1;
	m_flEyeYaw = 0.0f;
	m_flGaitYaw = 0.0f;

	m_fEyeHeight = DEFAULT_ACTOR_EYE_HEIGHT;

	m_aimovedata.forward = 0.0f;
	m_aimovedata.right = 0.0f;

	m_vecCurViewTarget = vec3_zero;
	m_vecViewTarget = vec3_zero;

	m_pTaskActionList = NULL;

	memset(m_pConditionBytes, 0, MAX_CONDITION_BYTES);
}

CAIBaseNPCHost::~CAIBaseNPCHost()
{
	
}

int GM_CDECL GM_AIBaseNPCHost_CreateNewTask(gmThread* a_thread)
{
	CAIBaseNPCHost* pThisHost = (CAIBaseNPCHost*)GetThisEntity(a_thread);

	if(!pThisHost)
		return GM_EXCEPTION;

	BaseEntity* pGoalEnt = NULL;

	if(GM_THREAD_ARG->GetNumParams() == 1)
	{
		if(GM_THREAD_ARG->ParamType(0) != GMTYPE_EQENTITY)
		{
			GM_EXCEPTION_MSG("expecting param %d as Entity", 0);
			return GM_EXCEPTION;
		} 

		pGoalEnt = (BaseEntity*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(0);
	}

	CAIScriptTask* pNewTask = new CAIScriptTask(pThisHost, pGoalEnt);

	gmUserObject* pObj = pNewTask->GetScriptUserObject();

	// return new task object
	GM_THREAD_ARG->PushUser(pObj);

	if(pThisHost->GetTaskActionList() == NULL)
		pThisHost->SetTask( pNewTask );

	return GM_OK;
}

int GM_CDECL GM_AIBaseNPCHost_NavigateToTarget(gmThread* a_thread)
{
	CAIBaseNPCHost* pThisHost = (CAIBaseNPCHost*)GetThisEntity(a_thread);

	if(!pThisHost)
		return GM_EXCEPTION;

	BaseEntity* pGoalEnt = NULL;

	GM_CHECK_NUM_PARAMS(1);

	if(GM_THREAD_ARG->ParamType(0) != GMTYPE_EQENTITY)
	{
		GM_EXCEPTION_MSG("expecting param %d as Entity", 1);
		return GM_EXCEPTION;
	} 

	pGoalEnt = (BaseEntity*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(0);

	if(!pGoalEnt)
		return GM_EXCEPTION;

	// generate navigation path
	g_pAINavigator->FindPath(pThisHost->GetNavigationPath(), pThisHost->GetAbsOrigin(), pGoalEnt->GetAbsOrigin());

	return GM_OK;
}

int GM_CDECL GM_AIBaseNPCHost_GetCurrentTask(gmThread* a_thread)
{
	CAIBaseNPCHost* pThisHost = (CAIBaseNPCHost*)GetThisEntity(a_thread);

	if(!pThisHost)
		return GM_EXCEPTION;

	CAIScriptTask* pTask = dynamic_cast<CAIScriptTask*>(pThisHost->GetTaskActionList());

	if(pTask)
		GM_THREAD_ARG->PushUser(pTask->GetScriptUserObject());
	else
		GM_THREAD_ARG->PushNull();

	return GM_OK;
}

void CAIBaseNPCHost::InitScriptHooks()
{
	// bind SelectTask and CreateNewTaskAction with scripting function using a tables
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "NewTask", GM_AIBaseNPCHost_CreateNewTask);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "Nav_MoveToTarget", GM_AIBaseNPCHost_NavigateToTarget);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "GetCurrentTask", GM_AIBaseNPCHost_GetCurrentTask);


	BaseClass::InitScriptHooks();
}

void CAIBaseNPCHost::Spawn()
{
	// fix spawning origin
	m_vecAbsOrigin.y += m_vBBox.GetSize().y;

	UpdateView();

	// fix our angles
	Vector3D angles = GetAbsAngles();

	Vector3D forward;
	AngleVectors(angles, &forward);

	angles = VectorAngles( forward );

	OnPreRender();

	SnapEyeAngles(angles);
	m_vecAbsAngles = vec3_zero;

	m_vecCurViewTarget = m_vecViewTarget = GetEyeOrigin() + forward*100.0f;

	// base processor setup, etc
	BaseClass::Spawn();

	SetActivity( ACT_IDLE );
	SetActivity( ACT_WALK, 1 );
	//SetActivity( ACT_AR_IDLE, 2 );

	m_nMoveYaw = FindPoseController("move_yaw");
	m_nLookPitch =  FindPoseController("look_pitch");

	PhysicsGetObject()->SetCollisionMask( COLLIDE_ACTOR );
	PhysicsGetObject()->SetContents( COLLISION_GROUP_ACTORS );
}

void CAIBaseNPCHost::SetTask(CAITaskActionBase* pTask)
{
	// replace current task
	if(m_pTaskActionList)
	{
		pTask->PushNext(m_pTaskActionList->GetNext());

		delete m_pTaskActionList;
	}

	m_pTaskActionList = pTask;
}

void CAIBaseNPCHost::OnDeath( const damageInfo_t& info )
{
	DropWeapon( GetActiveWeapon() );
	BaseClass::OnDeath(info);
}

void CAIBaseNPCHost::NPCThinkFrame()
{
	if(m_flNextNPCThink > gpGlobals->curtime)
		return;

	if(!m_pTaskActionList)
	{
		// try to select task and create inside
		OBJ_BEGINCALL("SelectTask")
		{
			call.End();
		}
		OBJ_CALLDONE

		m_flNextNPCThink = gpGlobals->curtime + 0.2f;

		if(!m_pTaskActionList)
			m_pTaskActionList = CreateNewTaskAction( NULL, TASKRESULT_NEW );
	}

	if(!m_pTaskActionList)
		return;

	// update task. The Update() function will prepare next action for execution.
	TaskActionResult_e taskResult = m_pTaskActionList->Action();

	if(taskResult == TASKRESULT_SUCCESS)
	{
		// set next task
		CAITaskActionBase* pSuccess = m_pTaskActionList;
		m_pTaskActionList = m_pTaskActionList->GetNext();

		// if there is no tasks
		if(!m_pTaskActionList)
		{
			// try to select task and create inside
			OBJ_BEGINCALL("SelectTask")
			{
				call.End();
			}
			OBJ_CALLDONE

			if(!m_pTaskActionList)
				m_pTaskActionList = CreateNewTaskAction( pSuccess, taskResult );
		}

		// remove successfull task
		if(pSuccess)
			delete pSuccess;
	}

	m_flNextNPCThink = gpGlobals->curtime + 0.2f;
}

AITaskActionType_e CAIBaseNPCHost::SelectTask( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget )
{
	// sorry, default means none

	// your class if AI must check:
	// - his resources (health, ammo, etc)
	// - his conditions
	// - the parameters of action

	// default is wait
	return AITASK_WAIT;
}

void CAIBaseNPCHost::LookByAngle(const Vector3D& angle)
{
	Vector3D forward;

	AngleVectors(angle, &forward);

	m_vecViewTarget = GetEyeOrigin() + forward*100.0f;
}

void DrawViewCone(const Vector3D& origin, const Vector3D& angles, float distance, float fFov, int sides)
{
	Vector3D radAngles = VDEG2RAD(angles);

	float fConeR = distance*sin(DEG2RAD(fFov));
	float fConeL = sqrt(distance*distance + fConeR*fConeR);

	Matrix4x4 dir = !rotateXYZ4(-radAngles.x,radAngles.y,radAngles.z);

	debugoverlay->Line3D((Vector3D)origin, origin+dir.rows[2].xyz()*distance, color4_white, color4_white, 0.0f);

	int nTestSides = sides / 4;

	Vector3D oldVec;

	for (int i = 0; i < nTestSides; i++)
	{
		Matrix4x4 r = rotateXYZ4<float>(0.0f,DEG2RAD(fFov*0.5f),((i * 2.0f * PI) / nTestSides))*dir;
		Vector3D vec = origin+r.rows[0].xyz() + r.rows[2].xyz()*distance;

		oldVec = vec;

		debugoverlay->Line3D((Vector3D)origin, vec, color4_white, color4_white, 0.0f);
	}


	for (int i = 0; i < sides; i++)
	{
		Matrix4x4 r = rotateXYZ4<float>(0.0f,DEG2RAD(fFov*0.5f),((i * 2.0f * PI) / sides))*dir;
		Vector3D vec = origin+r.rows[0].xyz() + r.rows[2].xyz()*distance;

		debugoverlay->Line3D(oldVec, vec, color4_white, color4_white, 0.0f);

		oldVec = vec;
	}
}

ConVar ai_visualizeview("ai_visualizeview", "0");
ConVar ai_visualizeview_dist("ai_visualizeview_dist", "256");

void CAIBaseNPCHost::AliveThink()
{
	// Task functions, behaviours, movement processor
	NPCThinkFrame();

	Vector3D view_target_dir = m_vecViewTarget-m_vecCurViewTarget;

	if(dot(view_target_dir,view_target_dir) > 0.1f)
		VectorMA(m_vecCurViewTarget, gpGlobals->frametime * 5.0f, view_target_dir, m_vecCurViewTarget);

	debugoverlay->Box3D(m_vecCurViewTarget-8.0f,m_vecCurViewTarget+8.0f, ColorRGBA(0,1,0,1), 0);
	debugoverlay->Box3D(m_vecViewTarget-16.0f,m_vecViewTarget+16.0f, ColorRGBA(1,0,0,1), 0);

	Vector3D target_dir = m_vecCurViewTarget - GetEyeOrigin();
	if(dot(target_dir,target_dir) > 0.01f)
	{
		Vector3D angles = NormalizeAngles360(VectorAngles(normalize(target_dir)));
		SnapEyeAngles( angles );
	}

	DoMovement();

	ProcessMovement( m_aimovedata );

	if(PhysicsGetObject())
		UpdateActorAnimation(GetEyeAngles().y, GetEyeAngles().x);

	m_TargetPath.DebugRender();

	if(ai_visualizeview.GetBool())
		DrawViewCone(GetEyeOrigin(), GetEyeAngles(), ai_visualizeview_dist.GetFloat(), RAD2DEG(0.5), 16);

	BaseClass::AliveThink();
}

void CAIBaseNPCHost::DoMovement()
{
	if(m_TargetPath.GetNumWayPoints() == 0)
	{
		m_aimovedata.forward = 0.0f;
		m_aimovedata.right = 0.0f;
		return;
	}

	float fDistance = m_TargetPath.GetCurrentWayPoint().plane.Distance( GetAbsOrigin() );

	if(fDistance > -5.0f)
		m_TargetPath.AdvanceWayPoint();

	Vector3D target_dir = normalize(m_TargetPath.GetCurrentWayPoint().position - GetAbsOrigin());

	Vector3D forward, right;
	AngleVectors(Vector3D(0,GetEyeAngles().y,0), &forward, &right);

	m_aimovedata.forward = dot(target_dir, forward);
	m_aimovedata.right = dot(target_dir, right);
}

void CAIBaseNPCHost::LookAtTarget(const Vector3D& target)
{
	m_vecViewTarget = target;
}

BaseEntity* CAIBaseNPCHost::FindNearestVisibleEnemy()
{
	BaseEntity* pNearest = NULL;
	float fNearestDist = MAX_COORD_UNITS;

	// search for enemies
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity* pEnt = (BaseEntity*)ents->ptr()[i];

		if(pEnt->EntType() != ENTTYPE_ACTOR)
			continue;
				
		RelationShipState_e relstate = g_pGameRules->GetRelationshipState(Classify(), pEnt->Classify());

		if( relstate == RLS_HATE )
		{
			AIVisibility_e nVis = GetEntityVisibility( pEnt );

			float dist = length(GetAbsOrigin() - pEnt->GetAbsOrigin());

			if(nVis == AIVIS_VISIBLE && dist < fNearestDist)
			{
				fNearestDist = dist;
				pNearest = pEnt;
			}
		}
	}

	return pNearest;
}
/*
CAIHint* CAIBaseNPCHost::FindNearestCoverHint()
{
	CAIHint* pNearest = NULL;
	float fNearestDist = MAX_COORD_UNITS;

	// search for enemies
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity* pEnt = (BaseEntity*)ents->ptr()[i];

		if(pEnt->EntType() != ENTTYPE_AI_HINT)
			continue;

		CAIHint* pHint = (CAIHint*)pEnt;

		if( !IsCover( pHint->GetHintType() ) )
			continue;

		if( pHint->IsUsed() )
			continue;
				
		AIVisibility_e nVis = GetEnemyEntityVisibility( pEnt );

		float dist = length(GetAbsOrigin() - pEnt->GetAbsOrigin());

		// if it's occluded from enemy
		if(nVis == AIVIS_OCCLUDED && dist < fNearestDist)
		{
			fNearestDist = dist;
			pNearest = pHint;
		}
	}

	return pNearest;
}*/
/*
void CAIBaseNPCHost::SetTarget( BaseEntity* pTarget )
{
	m_pTarget = pTarget;
}

void CAIBaseNPCHost::SetEnemy( BaseEntity* pEnemy )
{
	m_pEnemy = pEnemy;
}

BaseEntity*	CAIBaseNPCHost::GetTarget()
{
	return m_pTarget;
}

BaseEntity*	CAIBaseNPCHost::GetEnemy()
{
	return m_pEnemy;
}
*/
void CAIBaseNPCHost::UpdateActorAnimation( float eyeYaw, float eyePitch )
{
	m_flEyeYaw = eyeYaw;

	// set player angles
	m_vecAbsAngles = Vector3D(0, m_flEyeYaw, 0);

	// fix angles
	if ( eyePitch > 180.0f )
		eyePitch -= 360.0f;
	else if ( eyePitch < -180.0f )
		eyePitch += 360.0f;

	SetPoseControllerValue(m_nLookPitch, -eyePitch);

	UpdateMoveYaw();

	Vector3D phys_vel = m_vecAbsVelocity;

	float fSpeed = length(phys_vel.xz());

	if(fSpeed > 0.05f)
	{
		float fPlaybackScale = fSpeed / m_fMaxSpeed;

		if(fPlaybackScale > 1.0f)
			fPlaybackScale = 1.0f;

		SetPlaybackSpeedScale( fPlaybackScale );
		SetSequenceBlending(1, fPlaybackScale);

		if(fSpeed > 120)
		{
			if(GetCurrentActivity(1) != TranslateActivity(ACT_RUN, 1))
				SetActivity( ACT_RUN, 1 );
		}
		else
		{
			if(GetCurrentActivity(1) != TranslateActivity(ACT_WALK, 1))
				SetActivity( ACT_WALK, 1 );
		}
	}
	else
	{
		SetPlaybackSpeedScale( 1.0f );
		SetSequenceBlending(1, 0.0f);
		SetSequenceBlending(2, 1.0f);

		if(GetCurrentActivity() != TranslateActivity(ACT_IDLE))
			SetActivity( ACT_IDLE );
	}
}

Activity CAIBaseNPCHost::TranslateActivity(Activity act, int slotindex)
{
	if(GetActiveWeapon())
	{
		if(slotindex == 1)
		{
			switch(act)
			{
				case ACT_WALK:
					return ACT_WALK_WPN;
				case ACT_RUN:
					return ACT_RUN_WPN;
				default:
					return act;
			}
		}
		else if(slotindex == 0)
		{
			switch(act)
			{
				case ACT_IDLE:
				{
					return ACT_IDLE_WPN;
				}
				default:
					return act;
			}
		}
		else if(slotindex == 2)
		{
			Vector3D phys_vel = m_vecAbsVelocity;

			float fSpeed = length(phys_vel.xz());

			Activity translatedAct = ACT_INVALID;

			switch(translatedAct)
			{
				case ACT_WPN_IDLE:
				{
					if(fSpeed > 0.2)
					{
						translatedAct = ACT_WPN_IDLE_MOVEMENT;
					}
				}
				default:
					translatedAct = act;
			}

			return GetActiveWeapon()->TranslateActorActivity( translatedAct );
		}
	}

	return act;
}

void CAIBaseNPCHost::UpdateMoveYaw()
{
	if(m_nMoveYaw == -1)
		return;

	// view direction relative to movement
	float flYaw;	 

	EstimateYaw();

	float ang = m_flEyeYaw;

	// fix angles
	if ( ang > 180.0f )
		ang -= 360.0f;
	else if ( ang < -180.0f )
		ang += 360.0f;

	// calc side to side turning
	flYaw = ang - m_flGaitYaw;

	if (flYaw < -180)
		flYaw = flYaw + 360;
	else if (flYaw > 180)
		flYaw = flYaw - 360;

	SetPoseControllerValue(m_nMoveYaw, flYaw);
}

void CAIBaseNPCHost::EstimateYaw()
{
	float dt = gpGlobals->frametime;

	// paused or somethink
	if ( dt == 0.0f )
		return;

	Vector3D est_velocity = PhysicsGetObject()->GetVelocity();

	if ( fabs(est_velocity.x) <= 0.01 && fabs(est_velocity.z) <= 0.01)
	{
		m_flGaitYaw = m_flEyeYaw;
	}
	else
	{
		m_flGaitYaw = (atan2(est_velocity.z, est_velocity.x) * 180.0f / PI_F) - 90.0f;

		if (m_flGaitYaw > 180)
			m_flGaitYaw -= 360;
		else if (m_flGaitYaw < -180)
			m_flGaitYaw += 360;
	}
}

#define AI_VIS_COLLIDE (COLLISION_GROUP_WORLD | COLLISION_GROUP_ACTORS | COLLISION_GROUP_PLAYER | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES)

AIVisibility_e CAIBaseNPCHost::GetEntityVisibility(BaseEntity* pEntity)
{
	Vector3D forward;
	AngleVectors(GetEyeAngles(), &forward);

	// AI WILL ABLE TO CHECK AROUND FOR THE ENEMY!!!
	if(!IsPointInCone(pEntity->GetEyeOrigin(), GetEyeOrigin(), forward, 0.5f, 8192.0f))
	{
		return AIVIS_BEHINDVIEW;
	}

	// check visibility
	if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
	{
		int view_rooms[2] = {pEntity->m_pRooms[0], pEntity->m_pRooms[1]};

		int cntr = 0;

		for(int i = 0; i < m_nRooms; i++)
		{
			if(m_pRooms[i] != -1)
			{
				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
				{
					cntr++;
					continue;
				}

				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, 16.0f))
				{
					cntr++;
					continue;
				}
			}
		}

		if(cntr == m_nRooms)
			return AIVIS_OCCLUDED;
	}

	// trace object
	trace_t tr;

	BaseEntity* pEntIgnore = this;

	UTIL_TraceLine(GetEyeOrigin(), pEntity->GetEyeOrigin(), AI_VIS_COLLIDE, &tr, &pEntIgnore, 1);

	if(tr.hitEnt != pEntity)
		return AIVIS_OCCLUDED;

	return AIVIS_VISIBLE;
}

#define ENEMY_VIS_COLLIDE (COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES)

AIVisibility_e CAIBaseNPCHost::GetActorEntityVisibility(CBaseActor* pActor, BaseEntity* pEntity)
{
	if(!pActor)
		return AIVIS_OCCLUDED;

	Vector3D forward;
	AngleVectors(pActor->GetEyeAngles(), &forward);

	// AI WILL ABLE TO CHECK AROUND FOR THE ENEMY!!!
	if(!IsPointInCone(pEntity->GetEyeOrigin(), pActor->GetEyeOrigin(), forward, 0.5f, 8192.0f))
		return AIVIS_BEHINDVIEW;

	// check visibility
	if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
	{
		int view_rooms[2] = {pEntity->m_pRooms[0], pEntity->m_pRooms[1]};

		int cntr = 0;

		for(int i = 0; i < m_nRooms; i++)
		{
			if(m_pRooms[i] != -1)
			{
				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
				{
					cntr++;
					continue;
				}

				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(pActor->GetAbsOrigin(), 16.0f))
				{
					cntr++;
					continue;
				}
			}
		}

		if(cntr == m_nRooms)
			return AIVIS_OCCLUDED;
	}

	// trace object
	trace_t tr;

	BaseEntity* pEntIgnore = pActor;

	UTIL_TraceLine( pActor->GetEyeOrigin(), pEntity->GetAbsOrigin(), ENEMY_VIS_COLLIDE, &tr, &pEntIgnore, 1 );

	if(tr.fraction < 1.0f)
		return AIVIS_OCCLUDED;

	return AIVIS_VISIBLE;
}

CAINavigationPath* CAIBaseNPCHost::GetNavigationPath()
{
	return &m_TargetPath;
}

CAIMemoryBase* CAIBaseNPCHost::GetMemoryBase()
{
	return m_pMemoryBase;
}

CAITaskActionBase* CAIBaseNPCHost::GetTaskActionList()
{
	return m_pTaskActionList;
}

void CAIBaseNPCHost::SetCondition( int nCondition, bool bValue )
{
	int cond_byte = floor((float)(nCondition / 8)); // it could be buggy
	int cond_bit = nCondition - cond_byte*8;

	if(bValue)
		m_pConditionBytes[cond_byte] |= (1 << cond_bit);
	else
		m_pConditionBytes[cond_byte] &= ~(1 << cond_bit);
}

bool CAIBaseNPCHost::HasCondition( int nCondition )
{
	int cond_byte = floor((float)(nCondition / 8)); // it could be buggy
	int cond_bit = nCondition - cond_byte*8;

	return (m_pConditionBytes[cond_byte] & (1 << cond_bit));
}

// entity view - computation for rendering
// use SetActiveViewEntity to set view from this entity
void CAIBaseNPCHost::CalcView(Vector3D &origin, Vector3D &angles, float &fov)
{
	origin = GetEyeOrigin();
	angles = GetEyeAngles();
	fov = 90;
}

DECLARE_CMD(npc_create, "Creates NPC", CV_CHEAT)
{
	if(!viewrenderer->GetView())
		return;

	if(args && args->numElem() == 0)
	{
		MsgInfo("usage: npc_create <npc_name>");
		return;
	}

	internaltrace_t tr;

	Vector3D forward;
	AngleVectors(g_pViewEntity->GetEyeAngles(), &forward);

	Vector3D pos = g_pViewEntity->GetEyeOrigin();

	physics->InternalTraceBox(pos, pos+forward*1000.0f, Vector3D(32,64,32), COLLISION_GROUP_WORLD, &tr);

	if(tr.fraction == 1.0f)
		return;

	BaseEntity* pNPC = (BaseEntity*)entityfactory->CreateEntityByName( args->ptr()[0].GetData() );
	if(pNPC)
	{
		pNPC->SetAbsOrigin( tr.traceEnd );

		pNPC->Precache();
		pNPC->Spawn();
		pNPC->Activate();
	}
}