//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#include "platform/Platform.h"
#include "platform/MessageBox.h"
#include "DebugInterface.h"
#include <time.h>

#ifndef _WIN32
#include <stdarg.h>
#endif // _WIN32

#ifdef _WIN32

static LARGE_INTEGER g_PerformanceFrequency;
static LARGE_INTEGER g_ClockStart;

#else

#include <sys/time.h>
#include <errno.h>
timeval start;

#endif // _WIN32

// Platform QueryPerformanceCounterkAlertCautionAlert initializer
IEXPORTS void Platform_InitTime()
{
#ifdef _WIN32
    if ( !g_PerformanceFrequency.QuadPart )
    {
        QueryPerformanceFrequency(&g_PerformanceFrequency);
        QueryPerformanceCounter(&g_ClockStart);
    }
#else
	gettimeofday(&start, NULL);
#endif // _WIN32
}

// returns current time since application is running
IEXPORTS float Platform_GetCurrentTime()
{
#ifdef _WIN32

    LARGE_INTEGER curr;
    QueryPerformanceCounter(&curr);
    return (float) (double(curr.QuadPart) / double(g_PerformanceFrequency.QuadPart));

#else

    timeval curr;
    gettimeofday(&curr, NULL);
    return (float(curr.tv_sec - start.tv_sec) + 0.000001f * float(curr.tv_usec - start.tv_usec));

#endif // _WIN32
}

// sleeps the execution thread and let other processes to run for a specified amount of time.
IEXPORTS void Platform_Sleep(uint32 nMilliseconds)
{
#ifdef _WIN32

	Sleep(nMilliseconds);

#else

	struct timespec ts;
	ts.tv_sec = (time_t) (nMilliseconds / 1000);
	ts.tv_nsec = (long) (nMilliseconds % 1000) * 1000000;

    while(nanosleep(&ts, NULL)==-1 && errno == EINTR){}
#endif //_WIN32
}


IEXPORTS void AssertValidReadPtr( void* ptr, int count/* = 1*/ )
{
#ifdef _WIN32
    ASSERT(!IsBadReadPtr( ptr, count ));
#endif
}

IEXPORTS void AssertValidStringPtr( const char* ptr, int maxchar/* = 0xFFFFFF */ )
{
#ifdef _WIN32
    ASSERT(!IsBadStringPtrA( ptr, maxchar ));
#endif
}

IEXPORTS void AssertValidWStringPtr( const wchar_t* ptr, int maxchar/* = 0xFFFFFF */ )
{
#ifdef _WIN32
    ASSERT(!IsBadStringPtrW( ptr, maxchar ));
#endif
}

void AssertValidWritePtr( void* ptr, int count/* = 1*/ )
{
#ifdef _WIN32
    ASSERT(!IsBadWritePtr( ptr, count ));
#endif
}

IEXPORTS void AssertValidReadWritePtr( void* ptr, int count/* = 1*/ )
{
#ifdef _WIN32
    ASSERT(!( IsBadWritePtr(ptr, count) || IsBadReadPtr(ptr,count)));
#endif
}

#include <stdio.h>

#ifdef _WIN32

IEXPORTS void outputDebugString(const char *str)
{
    OutputDebugStringA(str);
    OutputDebugStringA("\n");
}

#else

IEXPORTS void outputDebugString(const char *str)
{
    printf("%s\n", str);
}

#endif // _WIN32
