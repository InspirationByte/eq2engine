//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Idle task for AI
//////////////////////////////////////////////////////////////////////////////////

#include "AI_Idle.h"

CAIIdleWaitPVS::CAIIdleWaitPVS( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget ) :
	CAITaskActionBase(pOwner, nGoal, pGoalTarget)
{

}

// updates task/action.
TaskActionResult_e CAIIdleWaitPVS::Action()
{
	BaseEntity* pNearest = NULL;
	float fNearestDist = MAX_COORD_UNITS;

	// search for enemies or other actors to interact
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity* pEnt = (BaseEntity*)ents->ptr()[i];

		if(pEnt->EntType() != ENTTYPE_ACTOR)
			continue;
				
		RelationShipState_e relstate = g_pGameRules->GetRelationshipState( GetOwner()->Classify(), pEnt->Classify() );

		if( relstate == RLS_HATE )
		{
			AIVisibility_e nVis = GetOwner()->GetEntityVisibility( pEnt );

			float dist = length(GetOwner()->GetAbsOrigin() - pEnt->GetAbsOrigin());

			if(nVis == AIVIS_VISIBLE && dist < fNearestDist)
			{
				fNearestDist = dist;
				pNearest = pEnt;
			}
		}
	}

	// if any nearest enemy found
	if(pNearest)
	{
		Msg( "Found you!\n" );

		// create next task defined by the entity and condition
		DeferNextTaskBy( GetOwner()->CreateNewTaskAction( this, TASKRESULT_SUCCESS, pNearest) );

		// success current
		return TASKRESULT_SUCCESS;
	}

	return TASKRESULT_RUNNING;
}

// returns task type
AITaskActionType_e CAIIdleWaitPVS::GetType()
{
	return AITASK_IDLE_WAIT_PVS;
}