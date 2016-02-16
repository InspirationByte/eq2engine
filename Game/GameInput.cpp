//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Input for game
//////////////////////////////////////////////////////////////////////////////////

#include "GameInput.h"
#include "IGameRules.h"
#include "BaseActor.h"

// registered keys
int			nClientButtons = 0;

static ConVar m_customaccel( "m_customaccel", "0", "Custom mouse acceleration (0 disable, 1 to enable, 2 enable with separate yaw/pitch rescale)."\
	"\nFormula: mousesensitivity = ( rawmousedelta^m_customaccel_exponent ) * m_customaccel_scale + sensitivity"\
	"\nIf mode is 2, then x and y sensitivity are scaled by m_pitch and m_yaw respectively.",CV_ARCHIVE );
static ConVar m_customaccel_scale( "m_customaccel_scale", "0.04", "Custom mouse acceleration value.", CV_ARCHIVE );
static ConVar m_customaccel_max( "m_customaccel_max", "0", "Max mouse move scale factor, 0 for no limit", CV_ARCHIVE );
static ConVar m_customaccel_exponent( "m_customaccel_exponent", "1", "Mouse move is raised to this power before being scaled by scale factor.",CV_ARCHIVE);

void IN_MouseMove(float x, float y)
{
	if(!g_pGameRules)
		return;

	static ConVar* m_sens = (ConVar*)g_sysConsole->FindCvar("m_sensitivity");
	float mouse_senstivity = m_sens->GetFloat();

	float mx = y;
	float my = x;

	if ( m_customaccel.GetBool() ) 
	{ 
		float raw_mouse_movement_distance = sqrt( mx * mx + my * my );
		float acceleration_scale = m_customaccel_scale.GetFloat();
		float accelerated_sensitivity_max = m_customaccel_max.GetFloat();
		float accelerated_sensitivity_exponent = m_customaccel_exponent.GetFloat();
		float accelerated_sensitivity = ( (float)pow( raw_mouse_movement_distance, accelerated_sensitivity_exponent ) * acceleration_scale + mouse_senstivity );

		if ( accelerated_sensitivity_max > 0.0001f && accelerated_sensitivity > accelerated_sensitivity_max )
		{
			accelerated_sensitivity = accelerated_sensitivity_max;
		}

		mx *= accelerated_sensitivity; 
		my *= accelerated_sensitivity; 
	}

	BaseEntity* pEnt = g_pGameRules->GetLocalPlayer();

	if(pEnt)
		pEnt->MouseMoveInput(mx*mouse_senstivity, my*mouse_senstivity);
}

DEFINE_INPUT_ACTION(primaryattack, IN_PRIMARY);
DEFINE_INPUT_ACTION(secondaryattack, IN_SECONDARY);

#ifdef STDGAME
DEFINE_INPUT_ACTION(duck, IN_DUCK)
DEFINE_INPUT_ACTION(jump, IN_JUMP)
DEFINE_INPUT_ACTION(reload, IN_RELOAD);
DEFINE_INPUT_ACTION(holster, IN_HOLSTER);
DEFINE_INPUT_ACTION(aim, IN_AIM);
DEFINE_INPUT_ACTION(use, IN_USE);

DEFINE_INPUT_ACTION(forward, IN_FORWARD)
DEFINE_INPUT_ACTION(backward, IN_BACKWARD)
DEFINE_INPUT_ACTION(strafeleft, IN_STRAFELEFT)
DEFINE_INPUT_ACTION(straferight, IN_STRAFERIGHT)
DEFINE_INPUT_ACTION(induce_speed, IN_SPD_FASTER)
DEFINE_INPUT_ACTION(reduce_speed, IN_SPD_SLOWER)
DEFINE_INPUT_ACTION(leanleft, IN_LEANLEFT)
DEFINE_INPUT_ACTION(leanright, IN_LEANRIGHT)
DEFINE_INPUT_ACTION(landprep, IN_SMOOTHLANDING)
DEFINE_INPUT_ACTION(forcerun, IN_FORCERUN);

DEFINE_INPUT_ACTION(showinventory, IN_INVERTORY);
DEFINE_INPUT_ACTION(flashlight, IN_FLASHLIGHT);
DEFINE_INPUT_ACTION(dropweapon, IN_DROPWEAPON);

DEFINE_INPUT_ACTION(slot1, IN_SLOT_PRIMARY);
DEFINE_INPUT_ACTION(slot2, IN_SLOT_SECONDARY);
DEFINE_INPUT_ACTION(slot3, IN_SLOT_GRENADE);
#endif // STDGAME

#ifdef PIGEONGAME
DEFINE_INPUT_ACTION(forward, IN_FORWARD)
DEFINE_INPUT_ACTION(backward, IN_BACKWARD)
DEFINE_INPUT_ACTION(leftwing, IN_WINGLEFT)
DEFINE_INPUT_ACTION(rightwing, IN_WINGRIGHT)

DEFINE_INPUT_ACTION(jump, IN_JUMP)
DEFINE_INPUT_ACTION(faststeer, IN_FASTSTEER)
#endif // PIGEONGAME