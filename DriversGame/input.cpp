//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers input keys
//////////////////////////////////////////////////////////////////////////////////

#include "input.h"
#include "KeyBinding/InputCommandBinder.h"

ConVar in_joy_deadzone("in_joy_deadzone", "0.02", "Joystick dead zone", CV_ARCHIVE);

ConVar in_joy_steer_smooth("in_joy_steer_smooth", "1", "Joystick steering smooth motion", CV_ARCHIVE);

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
		g_nClientButtons &= ~IN_STEERLEFT;
		g_nClientButtons &= ~IN_STEERRIGHT;
		g_nClientButtons &= ~IN_ANALOGSTEER;
		g_joySteeringValue = 0.0f;
		return;
	}
		
	g_joySteeringValue = sign(g_joySteeringValue) * pow(fabs(g_joySteeringValue), in_joy_steer_linear.GetFloat());

	g_nClientButtons &= ~IN_STEERLEFT;

	if (in_joy_steer_smooth.GetBool())
	{
		g_nClientButtons |= IN_STEERRIGHT;
		g_nClientButtons &= ~IN_ANALOGSTEER;
	}
	else
	{
		g_nClientButtons &= ~IN_STEERRIGHT;
		g_nClientButtons |= IN_ANALOGSTEER;
	}
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
DECLARE_CMD_RENAME(act_accel_enable, "+accel", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_ACCELERATE;
	g_joyAccelBrakeValue = 1.0f;
}
DECLARE_CMD_RENAME(act_accel_disable ,"-accel", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_ACCELERATE;
}

// brake
DECLARE_CMD_RENAME(act_brake_enable, "+brake", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_BRAKE;
	g_joyAccelBrakeValue = 1.0f;
}

DECLARE_CMD_RENAME(act_brake_disable ,"-brake", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_BRAKE;
}

// steering
DECLARE_CMD_RENAME(act_left_enable, "+steerleft", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_STEERLEFT;
	g_nClientButtons &= ~IN_STEERRIGHT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
	g_joySteeringValue = 1.0f;
}
DECLARE_CMD_RENAME(act_left_disable ,"-steerleft", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_STEERLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}

DECLARE_CMD_RENAME(act_right_enable, "+steerright", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons |= IN_STEERRIGHT;
	g_nClientButtons &= ~IN_STEERLEFT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
	g_joySteeringValue = 1.0f;
}
DECLARE_CMD_RENAME(act_right_disable, "-steerright", nullptr, CV_CLIENTCONTROLS)
{
	g_nClientButtons &= ~IN_STEERRIGHT;
	g_nClientButtons &= ~IN_ANALOGSTEER;
}

DECLARE_ACTION( handbrake, IN_HANDBRAKE )
DECLARE_ACTION( burnout, IN_BURNOUT )
DECLARE_ACTION( faststeer, IN_FASTSTEER )

DECLARE_ACTION( horn, IN_HORN )
DECLARE_ACTION( siren, IN_SIREN )

DECLARE_ACTION( lookleft, IN_LOOKLEFT )
DECLARE_ACTION( lookright, IN_LOOKRIGHT )
DECLARE_ACTION( changecamera, IN_CHANGECAM )

DECLARE_ACTION( forward, IN_FORWARD )
DECLARE_ACTION( backward, IN_BACKWARD )
DECLARE_ACTION( strafeleft, IN_LEFT )
DECLARE_ACTION( straferight, IN_RIGHT )

DECLARE_ACTION( signal_left, IN_SIGNAL_LEFT)
DECLARE_ACTION( signal_right, IN_SIGNAL_RIGHT)
DECLARE_ACTION( signal_emergency, IN_SIGNAL_EMERGENCY)
DECLARE_ACTION( switch_beams, IN_SWITCH_BEAMS)

DECLARE_ACTION( freelook, IN_FREELOOK )