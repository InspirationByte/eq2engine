//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Idle task for AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef AI_IDLE_H
#define AI_IDLE_H

#include "AIBaseNPC.h"
#include "AITaskActionBase.h"
#include "AIMemoryBase.h"

class CAIIdleWaitPVS : public CAITaskActionBase
{
public:
	CAIIdleWaitPVS( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget = NULL );

	// updates task/action.
	virtual TaskActionResult_e		Action();
	
	// returns task type
	virtual AITaskActionType_e		GetType();

protected:

};

#endif // AI_IDLE_H