//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: White cage player
//////////////////////////////////////////////////////////////////////////////////

#include "BaseActor.h"

#ifndef ACTOR_ASLAN_H
#define ACTOR_ASLAN_H

class CBaseViewmodel;

class CWhiteCagePlayer : public CBaseActor
{
	DEFINE_CLASS_BASE(CBaseActor);
public:

					CWhiteCagePlayer();

	void			Activate();
	void			Spawn();

	void			Precache();

	void			Update(float decaytime);
	void			OnRemove();

	// updates view of player
	void			CheckLanding();
	void			UpdateView();

	void			OnPreRender();
	void			OnPostRender();

	void			ApplyDamage( const damageInfo_t& info );

	void			MakeHit(const damageInfo_t& info, bool sound = true);

	// thinking functions
	void			AliveThink();

	void			GiveWeapon(const char *classname);

	void			DeathThink();

	void			SetHealth(int nHealth);

	Vector3D		GetEyeOrigin();
	Vector3D		GetEyeAngles();

	// entity view - computation for rendering
	// use SetActiveViewEntity to set view from this entity
	void			CalcView(Vector3D &origin, Vector3D &angles, float &fov);

	void			MouseMoveInput(float x, float y);

	void			DecayPunchAngle();
	void			ShakeView();

	void			ViewPunch(Vector3D &angleOfs, bool bWeapon = false);
	void			ViewShake(float fMagnutude, float fTime);
	bool			ProcessBob(Vector3D &p, Vector3D &a);

	EntRelClass_e	Classify()			{return ENTCLASS_PLAYER;}

	CBaseViewmodel*	GetViewmodel() {return m_pViewmodel;}

	void			SetActiveWeapon(CBaseWeapon* pWpn);

	void			SetZoomMultiplier(float fZoomMultiplier);
	float			GetZoomMultiplier();

protected:
	DECLARE_DATAMAP();

	bool			m_bFlashlight;
	float			m_fZoomMultiplier;

	float			m_fBobTime;
	float			m_fBobYAmplitude;
	float			m_fBobSpeed;

	float			m_fBobReminderFactor;
	float			m_fBobAmplitudeRun;

	float			m_fBobSpeedRun;

	//--------------------------------------------------

	float			m_fNextPickupCheckTime;

	float			m_fShakeDecayCurTime;
	float			m_fShakeMagnitude;
	Vector3D		m_vShake;

	Vector3D		m_vPunchAngleVelocity;
	Vector3D		m_vPunchAngle;

	Vector3D		m_vPunchAngle2;

	Vector3D		m_vRealEyeAngles;
	Vector3D		m_vRealEyeOrigin;

	float			flashlight_glitch_time;
	float			flashlight_glitch_time2;

	float			m_interpSignedSpeed;

	float			m_fNextHealthAdd;
	float			m_fAddEffect;

	CBaseViewmodel*	m_pViewmodel;
};

#endif // ACTOR_ASLAN_H