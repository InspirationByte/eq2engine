//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI event types and data structures for them
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIEVENTTYPES_H
#define AIEVENTTYPES_H

enum EAIEventType
{
	AI_EVENT_CAR_COLLISION,					// car collision
	AI_EVENT_CAR_SIGNAL,					// signal by someone
	AI_EVENT_COP_SIREN,				
	AI_EVENT_TRAFFICLIGHT_CHANGE,			// traffic light change

	// internal events
	AI_EVENT_INFRONT_OBJECT_MOVED,			// infront object movement
	AI_EVENT_INFRONT_OBJECT_ABRUPTLY,		// abruptly appeared object?
	AI_EVENT_INFRONT_OBJECT_AHEAD,			// object goes ahead
	AI_EVENT_INFRONT_OBJECT_GONNA_CRASH,	// object wants to crash with us

	AI_EVENT_COUNT,
};

#endif // AIEVENTTYPES_H