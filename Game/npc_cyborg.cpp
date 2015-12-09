//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: WhiteCage Cyborg
//////////////////////////////////////////////////////////////////////////////////

#include "npc_cyborg.h"
#include "BaseWeapon.h"

/*
BEGIN_DATAMAP(CNPC_Cyborg)

END_DATAMAP()
*/

#define CYBORG_MODEL "models/characters/cyborg.egf"
#define CYBORG_HEIGHT 137

CNPC_Cyborg::CNPC_Cyborg() : BaseClass()
{
	m_nMaxHealth = 100;
	m_fMaxSpeed = WALK_SPEED;
	m_fNextAttackTime = 0.0f;
	m_fNextAttackEndTime = 0.0f;
	m_fActorHeight = CYBORG_HEIGHT;
}

CNPC_Cyborg::~CNPC_Cyborg()
{

}

void CNPC_Cyborg::InitScriptHooks()
{
	//OBJ_BINDSCRIPT("scripts/cyborg_test.gms", this);

	BaseClass::InitScriptHooks();
}

void CNPC_Cyborg::ApplyDamage( const damageInfo_t& info )
{
	// cyborgs are human, and if they are hit by the other creatures, they will kill em
	if(info.m_pInflictor && info.m_pInflictor != this)
	{
		//SetEnemy( info.m_pInflictor );
		//NewTask(TASK_FACE_ENEMY, 0.1f);
	}

	BaseClass::ApplyDamage( info );
}

void CNPC_Cyborg::Precache()
{
	PrecacheStudioModel(CYBORG_MODEL);
	PrecacheEntity(weapon_ak74);

	BaseClass::Precache();
}

void CNPC_Cyborg::Spawn()
{
	SetModel(CYBORG_MODEL);

	BaseClass::Spawn();

	//NewTask( TASK_WAIT, 0.05f );

	GiveWeapon("weapon_ak74");
}

void CNPC_Cyborg::GiveWeapon(const char *classname)
{
	CBaseWeapon* pWeapon = (CBaseWeapon*)entityfactory->CreateEntityByName( classname );

	if(!pWeapon)
		return;

	if(pWeapon->EntType() == ENTTYPE_ITEM)
	{
		pWeapon->Spawn();
		pWeapon->Activate();

		if(!PickupWeapon(pWeapon))
		{
			pWeapon->SetState( ENTITY_REMOVE );
			return;
		}

		if(GetActiveWeapon() == NULL)
		{
			SetActiveWeapon(pWeapon);
		}
	}
	else
	{
		delete pWeapon;
	}
}


void CNPC_Cyborg::CyborgPrimaryAttack()
{
	/*
	if( GetActiveWeapon() && GetEntityVisibility( m_pEnemy ) == AIVIS_VISIBLE )
	{
		LookAtTarget( m_pEnemy->GetAbsOrigin() );

		if(m_fNextAttackTime < gpGlobals->curtime)
		{
			if(m_fNextAttackEndTime > gpGlobals->curtime)
			{
				m_nActorButtons |= IN_PRIMARY;
			}
			else
			{
				m_nActorButtons &= ~IN_PRIMARY;
				m_fNextAttackTime = gpGlobals->curtime + 0.4f;
				m_fNextAttackEndTime = m_fNextAttackTime+RandomFloat(0.04f, 0.2f);
			}
		}
		else
			m_nActorButtons &= ~IN_PRIMARY;
	}
	*/
}

CAITaskActionBase* CNPC_Cyborg::CreateNewTaskAction( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget )
{
	/*
	// if there is nothing, we generate any standard task
	if(!pPreviousAction)
	{
		CAIIdleWaitPVS* pWaitPVS = new CAIIdleWaitPVS( this, AIGOAL_NONE, pTarget );

		return pWaitPVS;
	}

	// select task type
	AITaskActionType_e newActionType = SelectTask( pPreviousAction, nResult, pTarget );

	if( newActionType == AITASK_MOVEMENTTARGET )
	{
		CAIMovementTarget* pTask = new CAIMovementTarget( this, AIGOAL_KILL, pTarget );
		return pTask;
	}*/

	return NULL;
}

AITaskActionType_e CNPC_Cyborg::SelectTask( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget )
{
	if(!pPreviousAction)
	{
		// create idle for this class
		return AITASK_IDLE_WAIT_PVS;
	}

	switch(nResult)
	{
		case TASKRESULT_SUCCESS:
		{
			if( pPreviousAction->GetType() == AITASK_IDLE_WAIT_PVS )
			{
				//CAIMovementTarget* pTask = new CAIMovementTarget( this, AIGOAL_KILL, pTarget );
				//return pTask;
				return AITASK_MOVEMENTTARGET;
			}

			break;
		}
		case TASKRESULT_FAIL:
		{
			break;
		}
	}

	return AITASK_IDLE_WAIT_PVS;
}

DECLARE_ENTITYFACTORY( npc_cyborg, CNPC_Cyborg )