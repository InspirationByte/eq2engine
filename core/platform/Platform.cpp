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
#include <fcntl.h>
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

IEXPORTS bool Platform_IsDebuggerPresent()
{
#ifdef _WIN32
	return IsDebuggerPresent();
#else
	const int statusFd = open("/proc/self/status", O_RDONLY);
    if (statusFd == -1)
        return false;

	char buf[4096];
	const ssize_t numRead = read(statusFd, buf, sizeof(buf) - 1);
	
    close(statusFd);
    if (statusFd <= 0)
        return false;

    buf[numRead] = '\0';

    constexpr char tracerPidString[] = "TracerPid:";
    const char* tracerPidPtr = strstr(buf, tracerPidString);
    if (!tracerPidPtr)
        return false;

    for (const char* characterPtr = tracerPidPtr + sizeof(tracerPidString) - 1; characterPtr <= buf + numRead; ++characterPtr)
    {
        if (isspace(*characterPtr))
            continue;
        else
            return isdigit(*characterPtr) != 0 && *characterPtr != '0';
    }

    return false;
#endif
}