//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Events for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "anim_events.h"

#define REGISTER_EVENT(ev) {#ev, ev}

anim_event_t anim_event_register[EV_COUNT] = 
{
	REGISTER_EVENT( EV_SOUND ),
	REGISTER_EVENT( EV_MUZZLEFLASH ),
	REGISTER_EVENT( EV_IK_WORLDATTACH ),
	REGISTER_EVENT( EV_IK_LOCALATTACH ),
	REGISTER_EVENT( EV_IK_GROUNDATTACH ),
	REGISTER_EVENT( EV_IK_DETACH),
};

AnimationEvent GetEventByName(const char* name)
{
	for(int i = 0; i < EV_COUNT; i++)
	{
		if(!anim_event_register[i].name)
			continue;

		if(!stricmp(anim_event_register[i].name, name))
			return anim_event_register[i].ev;
	}

	return EV_INVALID;
}

const char* GetEventName(AnimationEvent ev)
{
	for(int i = 0; i < EV_COUNT; i++)
	{
		if(anim_event_register[i].ev == ev)
			return anim_event_register[i].name;
	}

	return "EV_INVALID";
}