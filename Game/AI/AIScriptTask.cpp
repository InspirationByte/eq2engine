//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Script task
//////////////////////////////////////////////////////////////////////////////////

#include "AIScriptTask.h"
#include "AIBaseNPC.h"

gmType GMTYPE_AITASK	= -1;

CAIScriptTask::CAIScriptTask( CAIBaseNPCHost* pOwner, BaseEntity* pGoalTarget ) : CAITaskActionBase(pOwner, AIGOAL_SCRIPT_GOAL, pGoalTarget)
{
	EQGMS_INIT_OBJECT();

	// set the owner
	m_pTableObject->Set(GetScriptSys()->GetMachine(), "owner", gmVariable(pOwner->GetScriptUserObject()));

	m_result = TASKRESULT_RUNNING;
}

CAIScriptTask::~CAIScriptTask()
{
	GetScriptSys()->FreeTableObject(m_pTableObject);

	if(m_pUserObject)
		GetScriptSys()->FreeUserObject(m_pUserObject);
}

// updates task/action.
TaskActionResult_e CAIScriptTask::Action()
{
	OBJ_BEGINCALL("Action")
	{
		// m_result coult be modified by interrupt

		call.End();
	}
	OBJ_CALLDONE

	return m_result;
}

void CAIScriptTask::Interrupt()
{
	m_result = TASKRESULT_SUCCESS;
}

//--------------------------------------------------------------------------


inline CAIScriptTask* GetThisTask(gmThread* a_thread)
{
	GM_ASSERT(a_thread->GetThis()->m_type == GMTYPE_AITASK); //Paranoid check for type function

	CAIScriptTask* pTask = (CAIScriptTask*)a_thread->ThisUser();

	if(!pTask)
		return NULL;

	return pTask;
}

int GM_CDECL GM_CAIScriptTask_Interrupt(gmThread* a_thread)
{
	CAIScriptTask* pTask = GetThisTask(a_thread);

	if(!pTask)
		return GM_EXCEPTION;

	pTask->Interrupt();

	return GM_OK;
}

int GM_CDECL GM_CAIScriptTask_DeferBy(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(GM_THREAD_ARG->ParamType(0) != GMTYPE_AITASK)
	{
		GM_EXCEPTION_MSG("expecting param %d as AITask", 1);
		return GM_EXCEPTION;
	} 

	CAIScriptTask* pDeferBy = (CAIScriptTask*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(1);

	CAIScriptTask* pTask = GetThisTask(a_thread);

	if(!pTask)
		return GM_EXCEPTION;

	pTask->DeferNextTaskBy(pDeferBy);

	return GM_OK;
}

int GM_CDECL GM_CAIScriptTask_PushNext(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	if(GM_THREAD_ARG->ParamType(0) != GMTYPE_AITASK)
	{
		GM_EXCEPTION_MSG("expecting param %d as AITask", 1);
		return GM_EXCEPTION;
	} 

	CAIScriptTask* pNext = (CAIScriptTask*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(1);

	CAIScriptTask* pTask = GetThisTask(a_thread);

	if(!pTask)
		return GM_EXCEPTION;

	pTask->PushNext(pNext);

	return GM_OK;
}

int GM_CDECL GM_CAIScriptTask_GetGoalTarget(gmThread* a_thread)
{
	CAIScriptTask* pTask = GetThisTask(a_thread);

	if(!pTask)
		return GM_EXCEPTION;

	BaseEntity* pGoal = pTask->GetGoalTarget();

	if(pGoal)
		GM_THREAD_ARG->PushUser(pGoal->GetScriptUserObject());
	else
		GM_THREAD_ARG->PushNull();

	return GM_OK;
}

void GM_CDECL AITask_GetDot(gmThread * a_thread, gmVariable * a_operands)
{
	//O_GETDOT = 0,       // object, "member"          (tos is a_operands + 2)
	GM_ASSERT(a_operands[0].m_type == GMTYPE_AITASK);

	gmUserObject* userObj = (gmUserObject*) GM_OBJECT(a_operands[0].m_value.m_ref);
	CAIScriptTask* pTask = (CAIScriptTask*)userObj->m_user;

	if(!pTask)
	{
		a_operands[0].Nullify();

		return;
	}

	a_operands[0] = pTask->GetScriptTableObject()->Get(a_operands[1]);
}

void GM_CDECL AITask_SetDot(gmThread * a_thread, gmVariable * a_operands)
{
	//O_SETDOT,           // object, value, "member"   (tos is a_operands + 3)
	GM_ASSERT(a_operands[0].m_type == GMTYPE_AITASK);

	gmUserObject* userObj = (gmUserObject*) GM_OBJECT(a_operands[0].m_value.m_ref);
	CAIScriptTask* pTask = (CAIScriptTask*)userObj->m_user;

	if(pTask)
	{
		pTask->GetScriptTableObject()->Set(a_thread->GetMachine(), a_operands[2], a_operands[1]);
	}
}

void gmBindAITaskLib(gmMachine * a_machine)
{
	// register entity object
	GMTYPE_AITASK = a_machine->CreateUserType("AITask");

	// Bind Get dot operator for our type
	a_machine->RegisterTypeOperator(GMTYPE_AITASK, O_GETDOT, NULL, AITask_GetDot);

	// Bind Set dot operator for our type
	a_machine->RegisterTypeOperator(GMTYPE_AITASK, O_SETDOT, NULL, AITask_SetDot);

	// the standard baseentity functions...
	// for extensions use InitScriptHooks
	static gmFunctionEntry taskTypeLib[] = 
	{ 
		{"Interrupt", GM_CAIScriptTask_Interrupt},
		{"DeferBy", GM_CAIScriptTask_DeferBy},
		{"PushNext", GM_CAIScriptTask_PushNext},
		{"GetGoalTarget", GM_CAIScriptTask_GetGoalTarget},
	};

	a_machine->RegisterTypeLibrary(GMTYPE_AITASK, taskTypeLib, elementsOf(taskTypeLib));
}