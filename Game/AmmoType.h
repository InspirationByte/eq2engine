//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: STD ammo type
//////////////////////////////////////////////////////////////////////////////////

#ifndef AMMOTYPE_H
#define AMMOTYPE_H

#define AMMOTYPE_INVALID -1

#define AMMOFLAG_NO_PENETRATE	0x1 // doesn't penetrates anything
#define AMMOFLAG_NO_RICOCHET	0x2 // has no ricochet
#define AMMOFLAG_HAS_TRACER		0x4 // has tracer line (bullet visualization)

#define AMMOTYPE_MAX			5	// the max count of ammo types

struct CAmmoType
{
public:

	CAmmoType(const char* pszName, float fMass, float fStartSpeed, float fImpulse, float fPlayerDamage, float fNPCDamage, int nMaxAmmo, int nFlags);

	const char*	name;		// ammo type name.

	float	mass;			// bullet mass
	float	startspeed;		// bullet start speed
	float	impulse;		// bullet power, ideal for tuning penetration and power loss

	float	playerdamage;	// player damage
	float	npcdamage;		// npc damage

	int		max_ammo;		// max ammo
	
	int		flags;			// bullet flags
};

extern CAmmoType g_ammotype[]; // main ammo type structure

int GetAmmoTypeDef(const char* name); // returns ammo type index.

#endif // AMMOTYPE_H