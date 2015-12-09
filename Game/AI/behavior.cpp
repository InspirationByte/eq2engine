//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: AI Behavior base and definitions
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#include "behavior.h"
#include "AIBaseNPCHuman.h"

CBasicBehavior::CBasicBehavior()
{
	m_pOuter = NULL;

	m_bEnabled = true;
}

void CBasicBehavior::RunBehavior()
{
	if(!m_bEnabled)
		return;

	if(!m_Tasks.numElem())
		return;

	if(m_Tasks[0].fTime > gpGlobals->curtime)
		return;

	AITask_t &task = m_Tasks[0];

	if(task.status == TASKSTATUS_NEW)
		StartTask( &task );

	// only if status is not changed
	if(task.status == TASKSTATUS_NEW)
		task.status = TASKSTATUS_RUN;

	// run the task if possible
	int fail_code = RunTask( &task );

	if(fail_code != TASKFAIL_NO_FAIL)
	{
		// we need to send to fail it, because if AI will want to
		// add new task
		TaskFail( &task );

		// complete it
		task.status = TASKSTATUS_COMPLETE;
	}

	// run next task
	if(task.status == TASKSTATUS_COMPLETE)
	{
		m_Tasks.removeIndex(0);
		//m_Tasks.goToNext();
	}
}

bool CBasicBehavior::IsEnabled()
{
	return m_bEnabled;
}

void CBasicBehavior::Enable()
{
	m_bEnabled = true;
}

void CBasicBehavior::Disable()
{
	m_bEnabled = false;
}

int CBasicBehavior::RunTask(AITask_t* task)
{
	return TASKFAIL_NO_FAIL;
}

void CBasicBehavior::TaskFail( AITask_t* task )
{

}

void CBasicBehavior::StartTask( AITask_t* task )
{

}

int CBasicBehavior::SelectTask( AITask_t* oldtask )
{
	return TASK_WAIT;
}

void CBasicBehavior::NewTask(TaskType_e task_type, float fDelay)
{
	AITask_t task;
	task.fTime = gpGlobals->curtime + fDelay;
	task.task_type = task_type;
	task.status = TASKSTATUS_NEW;

	// add to queue
	m_Tasks.append( task );
}

CAIBaseNPCHuman* CBasicBehavior::GetOuter()
{
	return m_pOuter;
}

void CBasicBehavior::SetOuter(CAIBaseNPCHuman* pActor)
{
	m_pOuter = pActor;
}