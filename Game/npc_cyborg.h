//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WhiteCage Cyborg
//////////////////////////////////////////////////////////////////////////////////

#ifndef NPC_CYBORG_H
#define NPC_CYBORG_H

#include "AI/AIBaseNPC.h"

class CNPC_Cyborg : public CAIBaseNPCHost
{
	DEFINE_CLASS_BASE(CAIBaseNPCHost);
public:

	CNPC_Cyborg();
	~CNPC_Cyborg();

	void					Precache();

	void					InitScriptHooks();

	void					Spawn();

	void					ApplyDamage( const damageInfo_t& info );

	void					GiveWeapon(const char *classname);

	void					CyborgPrimaryAttack();

	AITaskActionType_e		SelectTask( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget = NULL );
	CAITaskActionBase*		CreateNewTaskAction( CAITaskActionBase* pPreviousAction, TaskActionResult_e nResult, BaseEntity* pTarget = NULL );

	EntRelClass_e			Classify()	{return ENTCLASS_CYBORG;}
private:

	float					m_fNextAttackTime;
	float					m_fNextAttackEndTime;

	//DECLARE_DATAMAP();
};

#endif // NPC_CYBORG_H
