//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base entity class
//////////////////////////////////////////////////////////////////////////////////

#ifndef DAMAGEINFO_H
#define DAMAGEINFO_H

#include "GameTrace.h"

class BaseEntity;

enum DamageType_e
{
	DAMAGE_TYPE_UNKNOWN = 0,	// unknown damage, may be a world hit
	DAMAGE_TYPE_BULLET,			// bullets, etc	
	DAMAGE_TYPE_BLAST,			// explosion
	DAMAGE_TYPE_FIRE,			// fire
	DAMAGE_TYPE_ELECTRICSHOCK,	// electrical shock
	DAMAGE_TYPE_COLD,			// cold places or nitro

	DAMAGE_TYPES
};

struct damageInfo_t
{
	damageInfo_t()
	{
		m_nDamagetype = DAMAGE_TYPE_UNKNOWN;
	}

	damageInfo_t(BaseEntity* pInflictor, BaseEntity* pTarget, Vector3D& origin, Vector3D& direction, float fDamage, float fImpulse, DamageType_e damagetype)
	{
		m_nDamagetype = damagetype;

		m_fDamage = fDamage;
		m_fImpulse = fImpulse;
		m_pTarget = pTarget;
		m_pInflictor = pInflictor;

		m_vOrigin = origin;
		m_vDirection = direction;
	}

	DamageType_e	m_nDamagetype;

	float			m_fDamage;
	float			m_fImpulse;
	BaseEntity*		m_pTarget;
	BaseEntity*		m_pInflictor;

	Vector3D		m_vOrigin;
	Vector3D		m_vDirection;

	// trace used for any physics object
	trace_t			trace;
};

#endif // DAMAGEINFO_H