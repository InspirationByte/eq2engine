//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Task action base
//////////////////////////////////////////////////////////////////////////////////

#ifndef AITASKACTION_H
#define AITASKACTION_H

#include "BaseEngineHeader.h"
#include "AITaskTypes.h"

// the result code
enum TaskActionResult_e
{
	TASKRESULT_NEW = -1,

	TASKRESULT_RUNNING = 0,
	TASKRESULT_SUCCESS,
	TASKRESULT_FAIL,
	
	TASKRESULT_COUNT,
};

class CAIBaseNPCHost;
class BaseEntity;

// task action base
class CAITaskActionBase
{
public:
	CAITaskActionBase( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget = NULL );
	virtual ~CAITaskActionBase() {}

	// updates task/action.
	virtual TaskActionResult_e			Action() = 0;

	// returns next task to be executed
	CAITaskActionBase*					GetNext();

	// returns owner character
	CAIBaseNPCHost*						GetOwner();
	
	virtual AITaskActionType_e			GetType() = 0;			// returns task type

	virtual AITaskGoal_e				GetGoal();				// returns goal type - the main task
	BaseEntity*							GetGoalTarget();		// returns goal target object. This could be NULL

	// deferrs next task, and placing other after this
	void								DeferNextTaskBy( CAITaskActionBase* pOther );
	void								PushNext( CAITaskActionBase* pOther );

	PPMEM_MANAGED_OBJECT();
	
protected:

	CAITaskActionBase*					m_pNext;			// next task with preserving further task list for execution

	// use owner memory instead of previous task

	CAIBaseNPCHost*						m_pOwnerEntity;		// owner to access controls, memory and navigator
	AITaskGoal_e						m_nGoalType;

	BaseEntity*							m_pGoalTarget;		
};


#endif // AITASKACTION_H