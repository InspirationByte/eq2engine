//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers input keys
//////////////////////////////////////////////////////////////////////////////////

#ifndef INPUT_H
#define INPUT_H

#include "core_base_header.h"

enum EInputActionFlags
{
	IN_ACCELERATE			= (1 << 0),
	IN_BRAKE				= (1 << 1),
	IN_STEERLEFT			= (1 << 2),
	IN_STEERRIGHT			= (1 << 3),
	IN_HANDBRAKE			= (1 << 4),
	IN_BURNOUT				= (1 << 5),
	IN_FASTSTEER			= (1 << 6),

	IN_ANALOGSTEER			= (1 << 7),

	IN_HORN					= (1 << 8),
	IN_SIREN				= (1 << 9),

	IN_SIGNAL_LEFT			= (1 << 10),
	IN_SIGNAL_RIGHT			= (1 << 11),
	IN_SIGNAL_EMERGENCY		= (1 << 12),
	IN_SWITCH_BEAMS			= (1 << 13),

	IN_LOOKLEFT				= (1 << 14),
	IN_LOOKRIGHT			= (1 << 15),
	IN_CHANGECAM			= (1 << 16),

	IN_FREELOOK				= (1 << 17),

	// free camera controls
	IN_FORWARD				= (1 << 18),
	IN_BACKWARD				= (1 << 19),
	IN_LEFT					= (1 << 20),
	IN_RIGHT				= (1 << 21),

	IN_MISC					= (IN_LOOKLEFT | IN_LOOKRIGHT | IN_CHANGECAM | IN_FORWARD | IN_BACKWARD | IN_LEFT | IN_RIGHT | IN_FREELOOK)
};

extern int			g_nClientButtons;
extern float		g_joySteeringValue;
extern float		g_joyAccelBrakeValue;

extern Vector2D		g_joyFreecamMove;
extern Vector2D		g_joyFreecamLook;

void ZeroInputControls();
void RegisterInputJoysticEssentials();

#define DECLARE_ACTION(localName, bitFlag)											\
	DECLARE_CMD_RENAME(act_##localName##_enable ,"+"#localName, nullptr, CV_CLIENTCONTROLS) \
	{				\
		g_nClientButtons |= bitFlag;											\
	}																				\
	DECLARE_CMD_RENAME(act_##localName##_disable ,"-"#localName, nullptr, CV_CLIENTCONTROLS) \
	{																				\
		g_nClientButtons &= ~bitFlag;											\
	}																				\

#endif // INPUT_H