//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI movement to target task
//////////////////////////////////////////////////////////////////////////////////

#include "AI_MovementTarget.h"

CAIMovementTarget::CAIMovementTarget( CAIBaseNPCHost* pOwner, AITaskGoal_e nGoal, BaseEntity* pGoalTarget ) :
	CAITaskActionBase(pOwner, nGoal, pGoalTarget)
{
	m_bInit = false;
}

// updates task/action.
TaskActionResult_e CAIMovementTarget::Action()
{
	if( !m_pGoalTarget )
		return TASKRESULT_FAIL;

	//Msg("target = %s\n", m_pGoalTarget->GetClassname() );

	// TODO: need time and distance checking
	if( !m_bInit )
	{
		g_pAINavigator->FindPath(GetOwner()->GetNavigationPath(), GetOwner()->GetAbsOrigin(), m_pGoalTarget->GetAbsOrigin());
		m_bInit = true;
	}

	if(GetOwner()->GetNavigationPath()->GetNumWayPoints())
	{
		Vector3D forward;
		GetOwner()->EyeVectors(&forward);

		Vector3D eyeLocal = GetOwner()->GetEyeOrigin() - GetOwner()->GetAbsOrigin();

		// look at current waypoint
		Vector3D vPos = GetOwner()->GetNavigationPath()->GetCurrentWayPoint().position + eyeLocal;

		vPos += GetOwner()->GetNavigationPath()->GetCurrentWayPoint().plane.normal * 200.0f;

		GetOwner()->LookAtTarget( vPos );

		Vector3D goal_pos = GetOwner()->GetNavigationPath()->GetGoal().position;

		if(length(GetOwner()->GetAbsOrigin() - goal_pos) < 100)
			return TASKRESULT_SUCCESS;
	}
	else
		return TASKRESULT_FAIL;

	return TASKRESULT_RUNNING;
}

// returns task type
AITaskActionType_e CAIMovementTarget::GetType()
{
	return AITASK_MOVEMENTTARGET;
}