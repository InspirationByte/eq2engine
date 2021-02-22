//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium timers
//////////////////////////////////////////////////////////////////////////////////

#ifndef TIMEMEASURE_H
#define TIMEMEASURE_H

#include <stdio.h>
#include <stdlib.h>

#include "core/dktypes.h"

#ifndef _WIN32
#include <sys/time.h>
#endif // _WIN32

class CEqTimer
{
public:
			CEqTimer();

	double	GetTime(bool reset = false);

protected:
#ifdef _WIN32
	uint64			m_clockStart;
#else
	timeval			m_timeStart;
#endif // _WIN32
};


#endif //TIMEMEASURE_H
