//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers input keys
//////////////////////////////////////////////////////////////////////////////////

#include "input.h"
#include "KeyBinding/InputCommandBinder.h"

ConVar in_joy_deadzone("in_joy_deadzone", "0.02", "Joystick dead zone", CV_ARCHIVE);

ConVar in_joy_steer_linear("in_joy_steer_linear", "1.0", "Joystick steering linearity", CV_ARCHIVE);
ConVar in_joy_accel_linear("in_joy_accel_linear", "1.0", "Joystick acceleration linearity", CV_ARCHIVE);
ConVar in_joy_brake_linear("in_joy_brake_linear", "1.0", "Joystick acceleration linearity", CV_ARCHIVE);

int		g_nClientButtons = 0;
float	g_joySteeringValue = 1.0f;
float	g_joyAccelBrakeValue = 1.0f;

void ZeroInputControls()
{
	g_nClientButtons = 0;
	g_joySteeringValue = 1.0f;
	g_joyAccelBrakeValue = 1.0f;
}

void JoyAction_Steering( short value )
{
	g_joySteeringValue = float(value) / float(SHRT_MAX);

	if(fabs(g_joySteeringValue) < in_joy_deadzone.GetFloat())
	{
		g_nClientButtons &= ~IN_TURNLEFT;
		g_nClientButtons &= ~IN_TURNRIGHT;
		g_joySteeringValue = 0.0f;
		return;
	}
		

	g_joySteeringValue = sign(g_joySteeringValue) * pow(fabs(g_joySteeringValue), in_joy_steer_linear.GetFloat());

	g_nClientButtons &= ~IN_TURNLEFT;
	g_nClientButtons |= IN_TURNRIGHT;
}

void JoyAction_Accel_Brake( short value )
{
	g_joyAccelBrakeValue = float(value) / float(SHRT_MAX);

	if(fabs(g_joyAccelBrakeValue) < in_joy_deadzone.GetFloat())
	{
		g_nClientButtons &= ~IN_BRAKE;
		g_nClientButtons &= ~IN_ACCELERATE;
		g_joyAccelBrakeValue = 0.0f;
		return;
	}

	if(g_joyAccelBrakeValue > 0)
	{
		g_nClientButtons |= IN_BRAKE;
		g_nClientButtons &= ~IN_ACCELERATE;
	}
	else
	{
		g_nClientButtons &= ~IN_BRAKE;
		g_nClientButtons |= IN_ACCELERATE;
	}

	// post-apply of linearity
	g_joyAccelBrakeValue = pow(fabs(g_joyAccelBrakeValue), in_joy_accel_linear.GetFloat());
}

void RegisterInputJoysticEssentials()
{
	g_inputCommandBinder->RegisterJoyAxisAction("steering", JoyAction_Steering);
	g_inputCommandBinder->RegisterJoyAxisAction("accel_brake", JoyAction_Accel_Brake);
}

// acceleration
DECLARE_CMD_RENAME(act_accel_enable, "+accel", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_ACCELERATE;
	g_nClientButtons &= ~IN_BRAKE;
	g_joyAccelBrakeValue = 1.0f;
}
DECLARE_CMD_RENAME(act_accel_disable ,"-accel", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_ACCELERATE;
}

// brake
DECLARE_CMD_RENAME(act_brake_enable, "+brake", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_BRAKE;
	g_nClientButtons &= ~IN_ACCELERATE;
	g_joyAccelBrakeValue = 1.0f;
}

DECLARE_CMD_RENAME(act_brake_disable ,"-brake", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_BRAKE;
}

// steering
DECLARE_CMD_RENAME(act_left_enable, "+left", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_TURNLEFT;
	g_nClientButtons &= ~IN_TURNRIGHT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
	g_joySteeringValue = 1.0f;
}
DECLARE_CMD_RENAME(act_left_disable ,"-left", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_TURNLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}

DECLARE_CMD_RENAME(act_right_enable, "+right", "Control command", CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_TURNRIGHT;
	g_nClientButtons &= ~IN_TURNLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
	g_joySteeringValue = 1.0f;
}
DECLARE_CMD_RENAME(act_right_disable, "-right", "Control command", CV_CLIENTCONTROLS)
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

DECLARE_ACTION( freelook, IN_FREELOOK )