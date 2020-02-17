//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Script task
//////////////////////////////////////////////////////////////////////////////////

#ifndef AISCRIPTTASK_H
#define AISCRIPTTASK_H

#include "BaseEngineHeader.h"
#include "AI/AITaskActionBase.h"

class CAIScriptTask : public CAITaskActionBase
{
public:
	CAIScriptTask();
	CAIScriptTask( CAIBaseNPCHost* pOwner, BaseEntity* pGoalTarget );

	~CAIScriptTask();

	// updates task/action.
	TaskActionResult_e		Action();

	void					Interrupt();
	
	// returns task type
	AITaskActionType_e		GetType() {return AITASK_SCRIPT;}
	AITaskGoal_e			GetGoal() {return AIGOAL_SCRIPT_GOAL;}

	EQGMS_DEFINE_SCRIPTOBJECT( GMTYPE_AITASK );

protected:
	TaskActionResult_e		m_result;
};

//------------------------------------------------------------------------------

// registrator
void gmBindAITaskLib(gmMachine * a_machine);

#endif // AISCRIPTTASK_H