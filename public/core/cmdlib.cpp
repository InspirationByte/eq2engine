//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base debug interface for console programs handler
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "cmdlib.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef far
#	undef far
#endif
#ifdef near
#	undef near
#endif

static unsigned short g_InitialColor = 0xFFFF;
static unsigned short g_LastColor = 0xFFFF;
static unsigned short g_BadColor = 0xFFFF;
static WORD g_BackgroundFlags = 0xFFFF;

static void GetInitialColors( )
{
	// Get the old background attributes.
	CONSOLE_SCREEN_BUFFER_INFO oldInfo;
	GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &oldInfo );
	g_InitialColor = g_LastColor = oldInfo.wAttributes & (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY);
	g_BackgroundFlags = oldInfo.wAttributes & (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY);

	g_BadColor = 0;
	if (g_BackgroundFlags & BACKGROUND_RED)
		g_BadColor |= FOREGROUND_RED;
	if (g_BackgroundFlags & BACKGROUND_GREEN)
		g_BadColor |= FOREGROUND_GREEN;
	if (g_BackgroundFlags & BACKGROUND_BLUE)
		g_BadColor |= FOREGROUND_BLUE;
	if (g_BackgroundFlags & BACKGROUND_INTENSITY)
		g_BadColor |= FOREGROUND_INTENSITY;
}

static WORD SetConsoleTextColor( int red, int green, int blue, int intensity )
{
	WORD ret = g_LastColor;

	g_LastColor = 0;
	if( red )	g_LastColor |= FOREGROUND_RED;
	if( green ) g_LastColor |= FOREGROUND_GREEN;
	if( blue )  g_LastColor |= FOREGROUND_BLUE;
	if( intensity ) g_LastColor |= FOREGROUND_INTENSITY;

	// Just use the initial color if there's a match...
	if (g_LastColor == g_BadColor)
		g_LastColor = g_InitialColor;

	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), g_LastColor | g_BackgroundFlags );
	return ret;
}

static void RestoreConsoleTextColor( WORD color )
{
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color | g_BackgroundFlags );
	g_LastColor = color;
}

CRITICAL_SECTION g_SpewCS;
bool g_bSpewCSInitted = false;

void fnConDebugSpew(ESpewType type,const char* text)
{
	// Hopefully two threads won't call this simultaneously right at the start!
	if ( !g_bSpewCSInitted )
	{
		InitializeCriticalSection( &g_SpewCS );
		g_bSpewCSInitted = true;
	}

	WORD old;
	EnterCriticalSection( &g_SpewCS );
	{
		if( type == SPEW_NORM )
		{
			old = SetConsoleTextColor( 1, 1, 1, 0 );
		}
		else if( type == SPEW_WARNING )
		{
			old = SetConsoleTextColor( 1, 1, 0, 1 );
		}
		else if( type == SPEW_SUCCESS )
		{
			old = SetConsoleTextColor( 0, 1, 0, 1 );
		}
		else if( type == SPEW_ERROR )
		{
			old = SetConsoleTextColor( 1, 0, 0, 1 );
		}
		else if( type == SPEW_INFO )
		{
			old = SetConsoleTextColor( 1, 0, 1, 1 );
		}
		else
		{
			old = SetConsoleTextColor( 1, 1, 1, 1 );
		}

		OutputDebugStringA( text );
		printf( "%s", text );

		RestoreConsoleTextColor( old );
	}
	LeaveCriticalSection( &g_SpewCS );
}


#else

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

static const char* g_InitialColor = "\033[0m";
static const char* g_NormColor = "\033[0;37m";
static const char* g_WarningColor = "\033[1;33m";
static const char* g_SuccessColor = "\033[1;32m";
static const char* g_ErrorColor = "\033[1;31m";
static const char* g_InfoColor = "\033[1;35m";

pthread_mutex_t g_SpewMutex = PTHREAD_MUTEX_INITIALIZER;
bool g_bSpewMutexInitted = false;

void GetInitialColors() 
{
}

void fnConDebugSpew(ESpewType type, const char *text)
{
    // Hopefully two threads won't call this simultaneously right at the start!
    if (!g_bSpewMutexInitted) {
        pthread_mutex_init(&g_SpewMutex, NULL);
        g_bSpewMutexInitted = true;
    }

    const char *color;
    pthread_mutex_lock(&g_SpewMutex);
    {
        if (type == SPEW_NORM) {
            color = g_NormColor;
        } else if (type == SPEW_WARNING) {
            color = g_WarningColor;
        } else if (type == SPEW_SUCCESS) {
            color = g_SuccessColor;
        } else if (type == SPEW_ERROR) {
            color = g_ErrorColor;
        } else if (type == SPEW_INFO) {
            color = g_InfoColor;
        } else {
            color = g_InitialColor;
        }

        printf("%s%s%s", color, text, g_InitialColor);
    }
    pthread_mutex_unlock(&g_SpewMutex);
}

#endif

void Install_SpewFunction()
{
	SetSpewFunction(fnConDebugSpew);
	GetInitialColors();
}
