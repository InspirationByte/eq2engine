//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/platform/platform.h"

#ifndef _WIN32
#include <time.h>
#include <errno.h>
#endif

// sleeps the execution thread and let other processes to run for a specified amount of time.
IEXPORTS void Platform_Sleep(uint32 nMilliseconds)
{
#ifdef _WIN32
	Sleep(nMilliseconds);
#else
	timespec ts;
	ts.tv_sec = (time_t) (nMilliseconds / 1000);
	ts.tv_nsec = (long) (nMilliseconds % 1000) * 1000000;

    while(nanosleep(&ts, nullptr)==-1 && errno == EINTR){}
#endif //_WIN32
}
