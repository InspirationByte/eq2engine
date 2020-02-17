//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: STD ammo type
//////////////////////////////////////////////////////////////////////////////////

#include "AmmoType.h"
#include "utils/strtools.h"

CAmmoType::CAmmoType(const char* pszName, float fMass, float fStartSpeed, float fImpulse, float fPlayerDamage, float fNPCDamage, int nMaxAmmo, int nFlags)
{
	name = pszName;

	mass = fMass;
	startspeed = fStartSpeed;
	impulse = fImpulse;

	playerdamage = fPlayerDamage;
	npcdamage = fNPCDamage;

	max_ammo = nMaxAmmo;

	flags = nFlags;
}

CAmmoType g_ammotype[] = 
{
#ifdef PIGEONGAME
	CAmmoType( "pigeon_shit", 0.08f, 700, 100, 20, 0, 0, AMMOFLAG_NO_PENETRATE | AMMOFLAG_NO_RICOCHET | AMMOFLAG_HAS_TRACER ),
#endif //PIGEONGAME

#ifdef STDGAME
	//			name		mass		i.spd		i.imp	plr.dmg		npc.dmg		max. ammo	flags
	CAmmoType( "har_bullet",	0.08f,	2700,		1800,	22,			10,			3,			0 ),
	CAmmoType( "pistol_bullet", 0.05f,	2900,		8000,	12,			8,			5,			0 ),
	CAmmoType( "wiremine",		0.25f,	2950,		1640,	100,		100,		4,			AMMOFLAG_NO_PENETRATE | AMMOFLAG_NO_RICOCHET | AMMOFLAG_HAS_TRACER ),
#endif // STDGAME

	CAmmoType(NULL, 0, 0, 0, 0, 0, 0, 0),
};

int GetAmmoTypeDef(const char* pszName)
{
	int id = 0;

	do
	{
		if(!stricmp(g_ammotype[id].name, pszName))
			return id;
	}
	while(g_ammotype[id++].name != NULL);

	return AMMOTYPE_INVALID;
}