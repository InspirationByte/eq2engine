//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers input keys
//////////////////////////////////////////////////////////////////////////////////

#ifndef INPUT_H
#define INPUT_H

#include "core_base_header.h"

#define IN_ACCELERATE		(1 << 0)
#define IN_BRAKE			(1 << 1)
#define IN_TURNLEFT			(1 << 2)
#define IN_TURNRIGHT		(1 << 3)
#define IN_HANDBRAKE		(1 << 4)
#define IN_BURNOUT			(1 << 5)
#define IN_EXTENDTURN		(1 << 6)

#define IN_ANALOGSTEER		(1 << 7)

#define IN_HORN				(1 << 8)
#define IN_SIREN			(1 << 9)

#define IN_LOOKLEFT			(1 << 10)
#define IN_LOOKRIGHT		(1 << 11)
#define IN_CHANGECAM		(1 << 12)

#define IN_FORWARD			(1 << 13)
#define IN_BACKWARD			(1 << 14)
#define IN_LEFT				(1 << 15)
#define IN_RIGHT			(1 << 16)

#define IN_FREELOOK			(1 << 17)

#define IN_MISC				(IN_LOOKLEFT | IN_LOOKRIGHT | IN_CHANGECAM | IN_FORWARD | IN_BACKWARD | IN_LEFT | IN_RIGHT | IN_FREELOOK)

extern int		g_nClientButtons;
extern float	g_joySteeringValue;
extern float	g_joyAccelBrakeValue;

void ZeroInputControls();
void RegisterInputJoysticEssentials();

#define DECLARE_ACTION(localName, bitFlag)											\
	DECLARE_CMD_RENAME(act_##localName##_enable ,"+"#localName, "Control command", CV_CLIENTCONTROLS) \
	{				\
		g_nClientButtons |= bitFlag;											\
	}																				\
	DECLARE_CMD_RENAME(act_##localName##_disable ,"-"#localName, "Control command", CV_CLIENTCONTROLS) \
	{																				\
		g_nClientButtons &= ~bitFlag;											\
	}																				\

#endif // INPUT_H