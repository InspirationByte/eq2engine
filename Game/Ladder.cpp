//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ladder
//////////////////////////////////////////////////////////////////////////////////

#include "Ladder.h"
#include "BaseActor.h"

// declare data table for actor
BEGIN_DATAMAP( CInfoLadderPoint )

	DEFINE_FIELD( m_vLadderDestPoint, VTYPE_ORIGIN),
	DEFINE_FIELD( m_vLadderDestAngles, VTYPE_ANGLES),

END_DATAMAP()

CInfoLadderPoint::CInfoLadderPoint()
{

}

void CInfoLadderPoint::Spawn()
{
	BaseEntity::Spawn();

	CheckTargetEntity();

	// get a target position
	if(GetTargetEntity())
	{
		m_vLadderDestPoint = GetTargetEntity()->GetAbsOrigin();
		m_vLadderDestAngles = GetTargetEntity()->GetAbsAngles();
		SetTargetEntity(NULL);
	}
	else
		SetState(ENTITY_REMOVE);
}

// checks object for usage ray/box
bool CInfoLadderPoint::CheckUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir)
{
	Vector3D forward;
	AngleVectors(GetAbsAngles(), &forward);

	/*
	if(dot(dir, forward) > -0.25f)
	{
		Msg("must face to ladder");
		return false; // must facing to the ladder
	}
	*/

	Vector3D vLadderVector = m_vLadderDestPoint - GetAbsOrigin();

	float intrp = lineProjection(GetAbsOrigin(), m_vLadderDestPoint, origin);

	Vector3D posAtLadder = lerp(GetAbsOrigin(), m_vLadderDestPoint, intrp);

	if(length(origin - posAtLadder) < 20)
		return true;

	//Msg("Not distant\n");

	return false;
}

// onUse handler
bool CInfoLadderPoint::OnUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir)
{
	// TODO: set new move type for player!
	CBaseActor* pActor = (CBaseActor*)pActivator;

	if(pActor->GetPose() != POSE_CINEMATIC_LADDER)
	{
		Vector3D vLadderVector = m_vLadderDestPoint - GetAbsOrigin();
		float intrp = lineProjection(GetAbsOrigin(), m_vLadderDestPoint, pActivator->GetAbsOrigin());
		Vector3D posAtLadder = lerp(GetAbsOrigin(), m_vLadderDestPoint, intrp);

		if(intrp < 0)
			pActor->SetAbsOrigin(posAtLadder + Vector3D(0,20,0));
		else
			pActor->SetAbsOrigin(posAtLadder);


		//pActor->SnapEyeAngles(Vector3D(pActor->GetEyeAngles().x, GetAbsAngles().y, 0));

		pActor->SetPose(POSE_CINEMATIC_LADDER, 0.25f);

		pActor->SetLadderEntity(this);
	}
	else
	{
		//pActor->SetPose(POSE_STAND, 0.25f);
		pActor->SetLadderEntity(NULL);
	}

	return true;
}

DECLARE_ENTITYFACTORY(info_ladder_point, CInfoLadderPoint)
