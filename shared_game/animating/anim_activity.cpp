//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Activities for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#include "anim_activity.h"
#include "utils/strtools.h"

struct actnamegroup_t
{
	char*		name;
	Activity	act;
};

#define REGISTER_ACTIVITY(activity) \
	{#activity, activity}

actnamegroup_t anim_activities[ACT_COUNT] = 
{
	// basic
	REGISTER_ACTIVITY( ACT_RAGDOLL ),

	REGISTER_ACTIVITY( ACT_IDLE ),
	REGISTER_ACTIVITY( ACT_WALK ),
	REGISTER_ACTIVITY( ACT_RUN ),

	REGISTER_ACTIVITY( ACT_FLY ),
	REGISTER_ACTIVITY( ACT_FLY_WINGS ),

	REGISTER_ACTIVITY( ACT_IDLE_COMBATSTATE ),

	REGISTER_ACTIVITY( ACT_IDLE_WPN ),
	REGISTER_ACTIVITY( ACT_WALK_WPN ),
	REGISTER_ACTIVITY( ACT_RUN_WPN ),

	REGISTER_ACTIVITY( ACT_IDLE_CROUCH ),
	REGISTER_ACTIVITY( ACT_WALK_CROUCH ),
	REGISTER_ACTIVITY( ACT_RUN_CROUCH ),

	REGISTER_ACTIVITY( ACT_IDLE_CROUCH_WPN ),
	REGISTER_ACTIVITY( ACT_WALK_CROUCH_WPN ),
	REGISTER_ACTIVITY( ACT_RUN_CROUCH_WPN ),

	REGISTER_ACTIVITY( ACT_JUMP ),
	REGISTER_ACTIVITY( ACT_JUMP_WPN ),

	// Weapon viewmodel activities
	REGISTER_ACTIVITY( ACT_VM_DEPLOY ),
	REGISTER_ACTIVITY( ACT_VM_IDLE ),
	REGISTER_ACTIVITY( ACT_VM_PRIMARYATTACK ),
	REGISTER_ACTIVITY( ACT_VM_PRIMARYATTACK_EMPTY ),
	REGISTER_ACTIVITY( ACT_VM_RELOAD_SHORT ),
	REGISTER_ACTIVITY( ACT_VM_RELOAD_FULL),
	REGISTER_ACTIVITY( ACT_VM_MELEE ),
	REGISTER_ACTIVITY( ACT_VM_SPRINT ),
	REGISTER_ACTIVITY( ACT_VM_AIM_IDLE ),
	REGISTER_ACTIVITY( ACT_VM_AIM_SHOOT ),

	REGISTER_ACTIVITY( ACT_GRN_PULLPIN ),
	REGISTER_ACTIVITY( ACT_GRN_THROW ),

#ifdef STDGAME

	// third person animations
	REGISTER_ACTIVITY(ACT_AR_IDLE),
	REGISTER_ACTIVITY(ACT_AR_IDLE_COMBATSTATE),
	REGISTER_ACTIVITY(ACT_AR_IDLE_MOVEMENT),
	REGISTER_ACTIVITY(ACT_AR_SHOOT),
	REGISTER_ACTIVITY(ACT_AR_RELOAD),

#endif // STDGAME

	// don't loose this
	{NULL, ACT_INVALID}
};

Activity GetActivityByName(const char* name)
{
	if(!stricmp("ACT_INVALID", name))
		return ACT_INVALID;

	for(int i = 0; i < ACT_COUNT; i++)
	{
		if(anim_activities[i].name == NULL)
			break;

		if(!stricmp(anim_activities[i].name, name))
			return anim_activities[i].act;
	}

	return ACT_INVALID;
}

const char* GetActivityName(Activity act)
{
	for(int i = 0; i < ACT_COUNT; i++)
	{
		if(anim_activities[i].act == act)
			return anim_activities[i].name;
	}

	return "ACT_INVALID";
}