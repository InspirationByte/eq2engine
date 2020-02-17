//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game basic weapon class
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEWEAPON_H
#define BASEWEAPON_H

#include "BaseActor.h"
#include "BaseEngineHeader.h"
#include "BaseAnimating.h"
#include "GameInput.h"
#include "ParticleRenderer.h"
#include "AmmoType.h"

class CBaseActor;
/*
enum WeaponActivationState_e
{
	WPN_INACTIVE = 0,
	WPN_ACTIVE,
	WPN_SCRIPTED,
};
*/

class CBaseWeapon : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);
public:
						CBaseWeapon();
						~CBaseWeapon();

	virtual void		Spawn();
	virtual void		Activate();
	virtual void		Precache();

	virtual void		OnRemove();

	virtual void		OnPreRender();

	virtual void		Update(float decaytime);

	// weapon-specific stuff

	virtual void		SetClip(int nBullets);
	virtual void		SetAmmoType(int nAmmoType);

	virtual int			GetClip();
	virtual int			GetMaxClip() {return 30;}

	virtual int			GetBulletsPerShot() {return 1;}

	virtual void		SetButtons(int nButtons) { m_nButtons = nButtons;};
	virtual void		SetOldButtons(int nOldButtons) { m_nOldButtons = nOldButtons;};

	// get it's slot
	virtual int			GetSlot() {return WEAPONSLOT_PRIMARY;}

	// default for now
	//int					GetAmmoType();

	virtual float		GetSpreadingMultiplier()	{return 1.0f;}

	virtual Vector3D	GetDevationVector()			{return Vector3D(1,1,1);} // view kick

	virtual const char*	GetShootSound()				{return "defaultweapon.shoot";} // shoot sound script

	virtual const char*	GetReloadSound()			{return "defaultweapon.reload";} // reload sound script

	virtual const char*	GetFastReloadSound()		{return "defaultweapon.fastreload";} // reload sound script

	virtual const char*	GetEmptySound()				{return "defaultweapon.empty";} // reload sound script

	virtual const char*	GetViewmodel()				{return "models/error.egf";}
	virtual const char*	GetWorldmodel()				{return "models/error.egf";}

	virtual int			GetAmmoType()				{return m_nAmmoType;}

	virtual void		Reload();
	virtual void		BeginReload();
	virtual void		FinishReload();

	virtual void		FireBullets(int numBullets, Vector3D &origin, Vector3D &angles, float fMaxDist);

	virtual void		StartShootSound( Vector3D& origin );
	virtual void		MakeViewKick( Vector3D &kick );

	virtual void		IdleThink();
	virtual void		InactiveThink();

	virtual void		PrimaryAttack();
	virtual void		SecondaryAttack();

	virtual void		Deploy();
	virtual void		Holster();

	void				SetAiming(bool bAiming);
	bool				IsAiming();

	virtual void		Drop();

	EntType_e			EntType() {return ENTTYPE_ITEM;}

	void				PhysicsCreateObjects();

	void				SetActivity(Activity act, int slot = 0);
	Activity			TranslateActorActivity(Activity act);
	float				GetCurrentAnimationDuration();

	virtual void		HandleEvent(AnimationEvent nEvent, char* options) {}

protected:
	
	int					m_nClipCurrent;
	int					m_nAmmoType;

	float				m_fPrimaryAttackTime;
	float				m_fSecondaryAttackTime;

	bool				m_bReload;

	int					m_nShotsFired;
	float				m_fNextIdleTime;
	float				m_fIdleTime;

	int					m_nButtons;
	int					m_nOldButtons;

	bool				m_bAiming;

protected:
	DECLARE_DATAMAP();
};

#endif // BASEWEAPON_H