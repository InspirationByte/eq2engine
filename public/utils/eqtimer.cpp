//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Perfomance measurement tools
//////////////////////////////////////////////////////////////////////////////////

#include "eqtimer.h"

#ifdef _WIN32
#include <Windows.h>
#endif
//---------------------------------------------------------------------------

CEqTimer::CEqTimer()
{
#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&m_clockStart);
#else
	gettimeofday(&m_timeStart, NULL);
#endif // _WIN32
}

double CEqTimer::GetTime(bool reset /*= false*/)
{
#ifdef _WIN32
	LARGE_INTEGER curr;
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);
	QueryPerformanceCounter(&curr);

	double value = double(curr.QuadPart - m_clockStart) / double(performanceFrequency.QuadPart);

	if (reset)
		m_clockStart = curr.QuadPart;
#else
    timeval curr;

    gettimeofday(&curr, NULL);

	double value = (float(curr.tv_sec - m_timeStart.tv_sec) + 0.000001f * float(curr.tv_usec - m_timeStart.tv_usec));

	if (reset)
		m_timeStart = curr;
#endif // _WIN32

	return value;
}
