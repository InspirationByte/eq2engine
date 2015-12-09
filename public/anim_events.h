//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Animation event ids (declared in SC files also)
//////////////////////////////////////////////////////////////////////////////////

#ifndef ANIM_EVENTS_H
#define ANIM_EVENTS_H

typedef enum EVENT_E
{
	EV_INVALID = -1,

	EV_SOUND,
	EV_MUZZLEFLASH,

	EV_IK_WORLDATTACH = 100,
	EV_IK_LOCALATTACH,
	EV_IK_GROUNDATTACH,
	EV_IK_DETACH,

	EV_COUNT,
}AnimationEvent;

struct anim_event_t
{
	char*			name;
	AnimationEvent	ev;
};

extern anim_event_t anim_event_register[EV_COUNT];

AnimationEvent	GetEventByName(char* name);
const char*		GetEventName(AnimationEvent ev);

#endif // ANIM_EVENTS_H