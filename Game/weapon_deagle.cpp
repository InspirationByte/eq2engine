//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Weapon AK-74 class
//////////////////////////////////////////////////////////////////////////////////

#include "BaseWeapon.h"
#include "BaseEngineHeader.h"

#define VIEWMODEL_NAME "models/weapons/v_deagle.egf"
#define WORLDMODEL_NAME "models/weapons/w_ak74.egf"

class CWeaponDEagle : public CBaseWeapon
{
	DEFINE_CLASS_BASE(CBaseWeapon);

public:
	void Spawn();
	void Precache();

	void IdleThink();

	const char*	GetViewmodel();
	const char*	GetWorldmodel();

	const char*	GetShootSound()				{return "de.shoot";} // shoot sound script

	const char*	GetReloadSound()			{return "de.reload";} // reload sound script

	const char*	GetFastReloadSound()		{return "de.fastreload";} // reload sound script

	int			GetMaxClip()				{return 7;}

	int			GetBulletsPerShot()			{return 1;}

	float		GetSpreadingMultiplier()	{return 1.0f;}

	Vector3D	GetDevationVector()			{return Vector3D(6,5,0);} // view kick

// get it's slot
	int			GetSlot() {return WEAPONSLOT_SECONDARY;}

protected:
	float		m_fLastShootTime;

};

DECLARE_ENTITYFACTORY(weapon_deagle, CWeaponDEagle)

void CWeaponDEagle::Spawn()
{
	m_fLastShootTime = 0.0f;

	m_nAmmoType = GetAmmoTypeDef("pistol_bullet");

	BaseClass::Spawn();
}

void CWeaponDEagle::Precache()
{
	BaseClass::Precache();
}

const char*	CWeaponDEagle::GetViewmodel()
{
	return VIEWMODEL_NAME;
}

const char*	CWeaponDEagle::GetWorldmodel()
{
	return WORLDMODEL_NAME;
}

void CWeaponDEagle::IdleThink()
{
	BaseClass::IdleThink();

	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(!pActor)
		return;

	SetButtons(pActor->GetButtons());

	if(m_fLastShootTime < gpGlobals->curtime)
		m_nShotsFired = 0;

	if((m_nButtons & IN_PRIMARY) && m_fPrimaryAttackTime < gpGlobals->curtime && !(m_nOldButtons & IN_PRIMARY) )
	{
		PrimaryAttack();

		m_fPrimaryAttackTime = gpGlobals->curtime+0.15f; ;

		m_fLastShootTime = gpGlobals->curtime+0.2f;
	}

	SetOldButtons(pActor->GetButtons());
}