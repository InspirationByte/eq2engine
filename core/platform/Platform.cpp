//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/platform/Platform.h"
#include "core/platform/MessageBox.h"

#include <time.h>
#include <errno.h>

// sleeps the execution thread and let other processes to run for a specified amount of time.
IEXPORTS void Platform_Sleep(uint32 nMilliseconds)
{
#ifdef _WIN32

	Sleep(nMilliseconds);

#else

	timespec ts;
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
