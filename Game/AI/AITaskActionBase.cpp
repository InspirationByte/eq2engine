//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Task action base
//////////////////////////////////////////////////////////////////////////////////

#include "AITaskActionBase.h"
#include "AIBaseNPC.h"

CAITaskActionBase::CAITaskActionBase( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget  )
{
	m_pNext = NULL;
	m_pOwnerEntity = pOwner;
	m_nGoalType = nGoal;
	m_pGoalTarget = pGoalTarget;
}

// returns next task to be executed
CAITaskActionBase* CAITaskActionBase::GetNext()
{
	return m_pNext;
}

// returns owner character
CAIBaseNPCHost* CAITaskActionBase::GetOwner()
{
	return m_pOwnerEntity;
}

// returns goal type - the main task
AITaskGoal_e CAITaskActionBase::GetGoal()
{
	return m_nGoalType;
}

// returns goal target object. This could be NULL
BaseEntity* CAITaskActionBase::GetGoalTarget()
{
	return m_pGoalTarget;
}

// deferrs next task, and placing other after this
void CAITaskActionBase::DeferNextTaskBy( CAITaskActionBase* pOther )
{
	if(pOther)
	{
		// place next task into other
		pOther->m_pNext = m_pNext;
	}
	else
		ASSERTMSG(false, "Programmer error: it seems your entity doesn't generated task by CreateNewTaskAction. Please check");

	// place other as next
	m_pNext = pOther;
}

void CAITaskActionBase::PushNext( CAITaskActionBase* pOther )
{
	if(!pOther)
		ASSERT(!"Programmer error: it seems your entity doesn't generated task by CreateNewTaskAction. Please check");

	if(m_pNext == NULL)
	{
		m_pNext = pOther;
		return;
	}

	CAITaskActionBase* pIter = m_pNext;

	while(pIter)
	{
		if(!pIter->m_pNext)
		{
			pIter->m_pNext = pOther;
			break;
		}
		else
			pIter = pIter->m_pNext;
	}
}