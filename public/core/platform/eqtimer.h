//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium timers
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _WIN32
#include <sys/time.h>
#endif // _WIN32

class CLASS_EXPORTS CEqTimer
{
public:
			CEqTimer();

	// returns time in seconds
	double	GetTime(bool reset = false);

	// returns milliseconds
	uint64	GetTimeMS(bool reset = false);

protected:
#ifdef _WIN32
	uint64			m_clockStart;
#else
	timeval			m_timeStart;
#endif // _WIN32
};