//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Input for game
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEINPUT_H
#define GAMEINPUT_H

#include "DebugInterface.h"
#include "math\Vector.h"

#define IN_DUCK				(1 << 1)
#define IN_JUMP				(1 << 2)
#define IN_PRIMARY			(1 << 3)
#define IN_SECONDARY		(1 << 4)
#define IN_RELOAD			(1 << 5)
#define IN_HOLSTER			(1 << 6)
#define IN_AIM				(1 << 7)
#define IN_FORCERUN			(1 << 8)
#define IN_USE				(1 << 9)

#ifdef STDGAME
#define IN_FORWARD			(1 << 10)
#define IN_BACKWARD			(1 << 11)
#define IN_STRAFELEFT		(1 << 12)
#define IN_STRAFERIGHT		(1 << 13)

#define IN_SPD_FASTER		(1 << 14)
#define IN_SPD_SLOWER		(1 << 15)
#define IN_LEANLEFT			(1 << 16)
#define IN_LEANRIGHT		(1 << 17)
#define IN_SMOOTHLANDING	(1 << 18)

#define IN_INVERTORY		(1 << 19)
#define IN_FLASHLIGHT		(1 << 20)
#define IN_DROPWEAPON		(1 << 21)

#define IN_SLOT_PRIMARY		(1 << 22)
#define IN_SLOT_SECONDARY	(1 << 23)
#define IN_SLOT_GRENADE		(1 << 24)
#endif // STDGAME

#ifdef PIGEONGAME
#define IN_FORWARD			(1 << 10)
#define IN_BACKWARD			(1 << 11)
#define IN_WINGLEFT			(1 << 12)
#define IN_WINGRIGHT		(1 << 13)
#define IN_FASTSTEER		(1 << 14)
#endif // PIGEONGAME

#define DEFINE_INPUT_ACTION(localName, bitFlag)											\
	DECLARE_CMD_STRING(act_##localName##_enable ,"+"#localName, "Control command Enable", CV_CLIENTCONTROLS) \
	{																				\
		nClientButtons |= bitFlag;											\
	}																				\
	DECLARE_CMD_STRING(act_##localName##_disable ,"-"#localName, "Control command Disable", CV_CLIENTCONTROLS) \
	{																				\
		nClientButtons &= ~bitFlag;											\
	}																				\

extern int	nClientButtons;

void IN_MouseMove(float x, float y);

#endif //GAMEINPUT_H