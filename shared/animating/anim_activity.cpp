//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Activities for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "anim_activity.h"

struct ActivityName
{
	EqStringRef	name;
	Activity	act;
};

#define MAKE_ACTIVITY(activity) \
	{#activity, activity}

static ActivityName activityNames[ACT_COUNT + 1] =
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
	MAKE_ACTIVITY( ACT_VEHICLE_STEER_BACK ),
	MAKE_ACTIVITY( ACT_VEHICLE_CRASH ),

	MAKE_ACTIVITY( ACT_CAR_HEADLIGHTS ),
	MAKE_ACTIVITY( ACT_CAR_ANTENNA ),
	MAKE_ACTIVITY( ACT_CAR_SPOILER ),
	MAKE_ACTIVITY( ACT_CAR_ROOF ),

	// don't loose this
	{ nullptr, ACT_INVALID }
};

Activity GetActivityByName(const char* name)
{
	if(!CString::CompareCaseIns("ACT_INVALID", name))
		return ACT_INVALID;

	for(int i = 0; i < ACT_COUNT; i++)
	{
		if(!activityNames[i].name.IsValid())
			break;

		if(activityNames[i].name == name)
			return activityNames[i].act;
	}

	return ACT_INVALID;
}

const char* GetActivityName(Activity act)
{
	for(int i = 0; i < ACT_COUNT; i++)
	{
		if(activityNames[i].act == act)
			return activityNames[i].name;
	}

	return "ACT_INVALID";
}