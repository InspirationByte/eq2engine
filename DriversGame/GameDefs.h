//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include "dktypes.h"

// There are some traffic infraction types that cop can recognize
enum EInfractionType
{
	INFRACTION_NONE = 0,

	INFRACTION_HAS_FELONY,		// this car has a felony

	INFRACTION_MINOR,			// minor infraction ratio

	INFRACTION_SPEEDING,			// speeding
	INFRACTION_RED_LIGHT,			// he's went on red light
	INFRACTION_WRONG_LANE,			// he drives opposite lane

	INFRACTION_HIT_MINOR,			// hit the debris
	INFRACTION_HIT,					// hit the wall / building
	INFRACTION_HIT_VEHICLE,			// hit another vehicle
	INFRACTION_HIT_SQUAD_VEHICLE,	// hit a squad car

	INFRACTION_COUNT,
};

struct SpeechQueue_t
{
	SpeechQueue_t* prev;
	EqString soundName;
};

#define MAX_SPEECH_QUEUE 8

// cop data
struct PursuerData_s
{
	int		infractions[INFRACTION_COUNT];
	float	felonyRating;

	float	hitCheckTimer;
	float	loudhailerTimer;
	float	speechTimer;

	int		pursuedByCount;
	bool	announced;

	int		lastDirection;

	SpeechQueue_t speechList[MAX_SPEECH_QUEUE];
	SpeechQueue_t* speech;
};

ALIGNED_TYPE(PursuerData_s, 4) PursuerData_t;

#endif // GAMEDEFS_H