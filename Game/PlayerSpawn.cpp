//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera for Engine
//////////////////////////////////////////////////////////////////////////////////

#include "PlayerSpawn.h"
#include "IGameRules.h"

BEGIN_DATAMAP(CInfoPlayerStart)
	DEFINE_KEYFIELD(m_szPlayerScript, "player_script", VTYPE_STRING),
END_DATAMAP()

CInfoPlayerStart::CInfoPlayerStart()
{

}

void CInfoPlayerStart::Spawn()
{
	g_pGameRules->AddPlayerStart(this);

	// fix our angles
	Vector3D angles = GetAbsAngles();

	Vector3D forward;
	AngleVectors(angles, &forward);

	m_vecAbsAngles = VectorAngles( forward );

	BaseEntity::Spawn();
}

float CInfoPlayerStart::GetYawAngle()
{
	return GetAbsAngles().y;
}

const char* CInfoPlayerStart::GetPlayerScriptName()
{
	return m_szPlayerScript.GetData();
}

DECLARE_ENTITYFACTORY(info_player_start,CInfoPlayerStart)