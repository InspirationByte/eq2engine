//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animation event ids (declared in SC files also)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum AnimationEvent
{
	EV_INVALID = -1,

	EV_SOUND,
	EV_MUZZLEFLASH,

	EV_IK_WORLDATTACH = 100,
	EV_IK_LOCALATTACH,
	EV_IK_GROUNDATTACH,
	EV_IK_DETACH,

	EV_COUNT,
};

struct anim_event_t
{
	char*			name;
	AnimationEvent	ev;
};

extern anim_event_t anim_event_register[EV_COUNT];

AnimationEvent	GetEventByName(const char* name);
const char*		GetEventName(AnimationEvent ev);