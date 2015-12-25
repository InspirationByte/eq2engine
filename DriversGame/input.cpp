//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers input keys
//////////////////////////////////////////////////////////////////////////////////

#include "input.h"

int g_nClientButtons = 0;

DECLARE_ACTION( accel, IN_ACCELERATE )
DECLARE_ACTION( brake, IN_BRAKE )
//DECLARE_ACTION(left, (IN_TURNLEFT | (~IN_ANALOGSTEER)))
//DECLARE_ACTION(right, (IN_TURNRIGHT | (~IN_ANALOGSTEER)))

DECLARE_CMD_STRING(act_left_enable, "+left", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_TURNLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}
DECLARE_CMD_STRING(act_left_disable ,"-left", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_TURNLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}

DECLARE_CMD_STRING(act_right_enable, "+right", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_TURNRIGHT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}
DECLARE_CMD_STRING(act_right_disable, "-right", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_TURNRIGHT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}


DECLARE_ACTION( handbrake, IN_HANDBRAKE )
DECLARE_ACTION( burnout, IN_BURNOUT )
DECLARE_ACTION( extendturn, IN_EXTENDTURN )

DECLARE_ACTION( horn, IN_HORN )
DECLARE_ACTION( siren, IN_SIREN )

DECLARE_ACTION( lookleft, IN_LOOKLEFT )
DECLARE_ACTION( lookright, IN_LOOKRIGHT )
DECLARE_ACTION( changecamera, IN_CHANGECAM )

DECLARE_ACTION( forward, IN_FORWARD )
DECLARE_ACTION( backward, IN_BACKWARD )
DECLARE_ACTION( strafeleft, IN_LEFT )
DECLARE_ACTION( straferight, IN_RIGHT )