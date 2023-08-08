//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Activities for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "anim_activity.h"

struct actnamegroup_t
{
	char*		name;
	Activity	act;
};

#define MAKE_ACTIVITY(activity) \
	{#activity, activity}

actnamegroup_t anim_activities[ACT_COUNT + 1] = 
{
	// basic
	MAKE_ACTIVITY( ACT_RAGDOLL ),
	MAKE_ACTIVITY( ACT_SCRIPTED_SEQUENCE ),

	MAKE_ACTIVITY( ACT_IDLE ),
	MAKE_ACTIVITY( ACT_CROUCH ),
	MAKE_ACTIVITY( ACT_LOOKOUT ),
	MAKE_ACTIVITY( ACT_JUMP ),
	MAKE_ACTIVITY( ACT_WALK ),
	MAKE_ACTIVITY( ACT_RUN ),
	MAKE_ACTIVITY( ACT_EVADE ),

	MAKE_ACTIVITY( ACT_VEHICLE_IDLE ),
	MAKE_ACTIVITY( ACT_VEHICLE_STEER ),

	MAKE_ACTIVITY( ACT_CAR_HEADLIGHTS ),
	MAKE_ACTIVITY( ACT_CAR_ANTENNA ),
	MAKE_ACTIVITY( ACT_CAR_SPOILER ),
	MAKE_ACTIVITY( ACT_CAR_ROOF ),

	// don't loose this
	{ nullptr, ACT_INVALID }
};

Activity GetActivityByName(const char* name)
{
	if(!stricmp("ACT_INVALID", name))
		return ACT_INVALID;

	for(int i = 0; i < ACT_COUNT; i++)
	{
		if(anim_activities[i].name == nullptr)
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