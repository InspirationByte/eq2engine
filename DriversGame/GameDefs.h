//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

const float INFRACTION_SKIP_REGISTER_TIME = 2.0f;

// cop data
struct PursuerData_t
{
	DECLARE_CLASS_NOBASE(PursuerData_t);
	DECLARE_NETWORK_TABLE();
	DECLARE_EMBEDDED_NETWORKVAR();

	float	lastInfractionTime[INFRACTION_COUNT];
	bool	hasInfraction[INFRACTION_COUNT];
	float	felonyRating;

	float	lastSeenTimer;
	float	hitCheckTimer;
	float	loudhailerTimer;
	float	speechTimer;

	int		pursuedByCount;
	bool	announced;

	float	lastDirectionTimer;
	int		lastDirection;		// swne
};

#endif // GAMEDEFS_H