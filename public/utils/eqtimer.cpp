//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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
	QueryPerformanceFrequency(&m_PerformanceFrequency);
	//m_PerformanceFrequency.QuadPart = m_PerformanceFrequency.QuadPart / 1000.0;

	QueryPerformanceCounter(&m_ClockStart);

#else
	 gettimeofday(&m_timeStart, NULL);
#endif // _WIN32
}

double CEqTimer::GetTime()
{
#ifdef _WIN32

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter( &CurrentTime );

	//QueryPerformanceFrequency(&m_PerformanceFrequency);
	//m_PerformanceFrequency.QuadPart = m_PerformanceFrequency.QuadPart / 1000.0;

	return (double)( CurrentTime.QuadPart - m_ClockStart.QuadPart ) / (double)(m_PerformanceFrequency.QuadPart);

#else
	// linux impl
    timeval curr;

    gettimeofday(&curr, NULL);

    return (float(curr.tv_sec - m_timeStart.tv_sec) + 0.000001f * float(curr.tv_usec - m_timeStart.tv_usec));
#endif // _WIN32
}
