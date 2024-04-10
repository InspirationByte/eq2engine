//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Activities for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum Activity
{
	ACT_INVALID = -1,

	ACT_RAGDOLL,
	ACT_SCRIPTED_SEQUENCE,

	// basic
	ACT_IDLE,
	ACT_CROUCH,
	ACT_LOOKOUT,
	ACT_JUMP,
	ACT_WALK,
	ACT_RUN,
	ACT_EVADE,

	ACT_VEHICLE_IDLE,
	ACT_VEHICLE_STEER,
	ACT_VEHICLE_STEER_BACK,
	ACT_VEHICLE_CRASH,

	ACT_CAR_HEADLIGHTS,	// headlight animations for cars
	ACT_CAR_ANTENNA,	// antenna extension for cars
	ACT_CAR_SPOILER,	// spoiler extension for cars
	ACT_CAR_ROOF,		// roof opening/closing

	ACT_COUNT,
};

Activity GetActivityByName(const char* name);
const char* GetActivityName(Activity act);
