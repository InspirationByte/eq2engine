//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium timers
//////////////////////////////////////////////////////////////////////////////////

#ifndef TIMEMEASURE_H
#define TIMEMEASURE_H

#include <stdio.h>
#include <stdlib.h>

#include "platform/Platform.h"

#ifndef _WIN32
#include <sys/time.h>
#endif // _WIN32

float	MEASURE_TIME_BEGIN();
float	MEASURE_TIME_STATS(float begintime);


class CEqTimer
{
public:
			CEqTimer();

	double	GetTime();

protected:
#ifdef _WIN32
	LARGE_INTEGER	m_PerformanceFrequency;
	//LARGE_INTEGER	m_MSPerformanceFrequency;
	LARGE_INTEGER	m_ClockStart;
#else
	timeval			m_timeStart;
#endif // _WIN32
};


#endif //TIMEMEASURE_H
