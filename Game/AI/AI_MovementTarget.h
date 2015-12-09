//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI movement to target task
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMOVEMENTTARGET_H
#define AIMOVEMENTTARGET_H

#include "AIBaseNPC.h"
#include "AITaskActionBase.h"
#include "AIMemoryBase.h"

class CAIMovementTarget : public CAITaskActionBase
{
public:
	CAIMovementTarget( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget = NULL );

	// updates task/action.
	virtual TaskActionResult_e		Action();
	
	// returns task type
	virtual AITaskActionType_e		GetType();

protected:
	bool							m_bInit;
};

#endif // AIMOVEMENTTARGET_H