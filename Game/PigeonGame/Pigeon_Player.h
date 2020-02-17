//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: player-controlled pigeon
//////////////////////////////////////////////////////////////////////////////////

#ifndef PIGEON_PLAYER_H
#define PIGEON_PLAYER_H

#include "BasePigeon.h"

class CPlayerPigeon : public CBasePigeon
{
	DEFINE_CLASS_BASE( CBasePigeon );
public:

					CPlayerPigeon();

	void			Activate();

	void			Spawn();

	void			Precache();

	void			OnRemove();

	// entity view - computation for rendering
	// use SetActiveViewEntity to set view from this entity
	void			CalcView(Vector3D &origin, Vector3D &angles, float &fov);

	void			DecayPunchAngle();
	void			ShakeView();

	void			ViewPunch(Vector3D &angleOfs, bool bWeapon = false);
	void			ViewShake(float fMagnutude, float fTime);

	Vector3D		GetEyeOrigin();
	Vector3D		GetEyeAngles();

	void			MouseMoveInput(float x, float y);

	EntRelClass_e	Classify()			{return ENTCLASS_PLAYER;}

	void			AliveThink();
	void			DeathThink();

	// updates view of player
	void			UpdateView();

protected:

	float			m_fShakeDecayCurTime;
	float			m_fShakeMagnitude;
	Vector3D		m_vShake;

	Vector3D		m_vPunchAngleVelocity;
	Vector3D		m_vPunchAngle;

	Vector3D		m_vPunchAngle2;

	Vector3D		m_vRealEyeAngles;
	Vector3D		m_vRealEyeOrigin;

	// ground control
	float			m_fPigeonSwingVelocity;		// this is real velocity
	float			m_fPlayerSwing;				// this is controller

	Vector3D		m_vHeadPos;

	debugGraphBucket_t* pPigeonSwing;
	debugGraphBucket_t* pPlayerSwing;

	// TODO: head controller, and other functionals

	DECLARE_DATAMAP();
};


#endif // PIGEON_PLAYER_H