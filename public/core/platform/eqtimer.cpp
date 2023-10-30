//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Perfomance measurement tools
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/platform/eqtimer.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//---------------------------------------------------------------------------

CEqTimer::CEqTimer()
{
#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&m_clockStart);
#else
	gettimeofday(&m_timeStart, nullptr);
#endif // _WIN32
}

double CEqTimer::GetTime(bool reset /*= false*/)
{
#ifdef _WIN32
	LARGE_INTEGER curr;
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);
	QueryPerformanceCounter(&curr);

	const double value = double(curr.QuadPart - m_clockStart) / double(performanceFrequency.QuadPart);

	if (reset)
		m_clockStart = curr.QuadPart;
#else
    timeval curr;
    gettimeofday(&curr, nullptr);

	double value = (float(curr.tv_sec - m_timeStart.tv_sec) + 0.000001f * float(curr.tv_usec - m_timeStart.tv_usec));

	if (reset)
		m_timeStart = curr;
#endif // _WIN32

	return value;
}


uint64 CEqTimer::GetTimeMS(bool reset)
{
#ifdef _WIN32
	LARGE_INTEGER curr;
	QueryPerformanceCounter(&curr);
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	const uint64 value = (curr.QuadPart - m_clockStart * 1000) / performanceFrequency.QuadPart;

	if (reset)
		m_clockStart = curr.QuadPart;
#else
	timeval curr;
	gettimeofday(&curr, nullptr);

	const uint64 value = 1000 * (double(curr.tv_sec - m_timeStart.tv_sec) + 0.000001 * double(curr.tv_usec - m_timeStart.tv_usec));

	if (reset)
		m_timeStart = curr;
#endif // _WIN32

	return value;
}