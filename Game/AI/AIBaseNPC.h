//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI Base Non-player character
//				Character controls are here, and animation processing is there
// TODO:	task manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIBASENPC_H
#define AIBASENPC_H

#include "BaseActor.h"
#include "ai_navigator.h"
#include "AIMemoryBase.h"
#include "AITaskActionBase.h"
#include "AINode.h"

#include "AI_Idle.h"
#include "AI_MovementTarget.h"

enum AIVisibility_e
{
	AIVIS_VISIBLE = 0,			// visible, in view cone
	AIVIS_BEHINDVIEW,			// behind view
	AIVIS_OCCLUDED,				// in invisible room or out from portal planes, or just failed to trace
};

enum AICondtion_e
{
	COND_LOW_HEALTH = 0,
	COND_LOW_AMMO,
	COND_IN_COVER,
	COND_ALONE,				// NPC lost contact with team.
	COND_ENEMY_ALIVE,		// NPC knows about enemy, or he didn't killed enemy after spotted. He will fear or search for him.

	COND_COUNT,
};

#define MAX_CONDITION_BYTES 32

//---------------------------------------------------------------------
// AI base human
//---------------------------------------------------------------------
class CAIBaseNPCHost : public CBaseActor
{
	DEFINE_CLASS_BASE(CBaseActor);

public:
	CAIBaseNPCHost();
	~CAIBaseNPCHost();

	void							InitScriptHooks();

	void							Spawn();
	void							AliveThink();

	virtual void					OnDeath( const damageInfo_t& info );

	//---------------------------------------------------------------------
	// More AI Stuff
	//---------------------------------------------------------------------

	void							DoMovement();

	void							NPCThinkFrame();

	AIVisibility_e					GetEntityVisibility(BaseEntity* pEntity);
	AIVisibility_e					GetActorEntityVisibility(CBaseActor* pActor, BaseEntity* pEntity);

	virtual BaseEntity*				FindNearestVisibleEnemy();
	//virtual CAIHint*				FindNearestCoverHint();

	CAINavigationPath*				GetNavigationPath();	// returns it's path waypoints
	CAIMemoryBase*					GetMemoryBase();		// returns memory base
	CAITaskActionBase*				GetTaskActionList();	// returns task actions

	void							SetTask(CAITaskActionBase* pTask);

	// AI actors can select their new tasks if they selected 
	virtual AITaskActionType_e		SelectTask( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget = NULL );
	virtual	CAITaskActionBase*		CreateNewTaskAction( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget = NULL ) = 0;

	//void							SetTarget( BaseEntity* pTarget );	// t
	//void							SetEnemy( BaseEntity* pEnemy );

	//BaseEntity*					GetTarget();
	//BaseEntity*					GetEnemy();

	void							LookAtTarget(const Vector3D& target);
	void							LookByAngle(const Vector3D& angle);

	virtual void					SetCondition( int nCondition, bool bValue );
	virtual bool					HasCondition( int nCondition );

	//---------------------------------------------------------------------
	// visual stuff
	//---------------------------------------------------------------------

	virtual Activity				TranslateActivity(Activity act, int slotindex = 0);

	void							CalcView(Vector3D &origin, Vector3D &angles, float &fov);

	void							UpdateActorAnimation( float eyeYaw, float eyePitch );
	void							UpdateMoveYaw();

	void							EstimateYaw();

protected:

	DECLARE_DATAMAP();

	//BaseEntity*					m_pEnemy;
	//BaseEntity*					m_pTarget;

	CAINavigationPath				m_TargetPath;
	CAIMemoryBase*					m_pMemoryBase;
	CAITaskActionBase*				m_pTaskActionList;

	movementdata_t					m_aimovedata;

	Vector3D						m_vecViewTarget;
	Vector3D						m_vecCurViewTarget;

	char							m_pConditionBytes[MAX_CONDITION_BYTES];

	// visual stuff
	int								m_nMoveYaw;
	int								m_nLookPitch;
	float							m_flEyeYaw;
	float							m_flGaitYaw;

	float							m_flNextNPCThink;
};

#endif // AIBASENPC_H