//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Perfomance measurement tools
//////////////////////////////////////////////////////////////////////////////////

#include "eqtimer.h"
#include <math.h>

float MEASURE_TIME_BEGIN()
{
	return Platform_GetCurrentTime();
}

float MEASURE_TIME_STATS(float begintime)
{
	return fabs(Platform_GetCurrentTime() - begintime)*1000.0f;
}

//---------------------------------------------------------------------------

CEqTimer::CEqTimer()
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_performanceFrequency);
#else
	gettimeofday(&m_timeStart, NULL);
#endif // _WIN32
}

double CEqTimer::GetTime(bool reset /*= false*/)
{
#ifdef _WIN32
	LARGE_INTEGER curr;

	QueryPerformanceCounter( &curr);

	double value = double(curr.QuadPart - m_clockStart.QuadPart) / double(m_performanceFrequency.QuadPart);

	if (reset)
		m_clockStart = curr;
#else
    timeval curr;

    gettimeofday(&curr, NULL);

	double value = (float(curr.tv_sec - m_timeStart.tv_sec) + 0.000001f * float(curr.tv_usec - m_timeStart.tv_usec));

	if (reset)
		m_timeStart = curr;
#endif // _WIN32

	return value;
}
