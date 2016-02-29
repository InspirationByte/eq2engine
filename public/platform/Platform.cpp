//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////-

#include "Platform.h"
#include "core_base_header.h"
#include "DebugInterface.h"
#include <time.h>

#ifndef _WIN32
#include <stdarg.h>
#endif // _WIN32

#if !defined(_WIN32) && defined(USE_GTK)

#include <gtk/gtk.h>

// This is such a hack, but at least it works.
gboolean idle(gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

void MessageBox(const char *string, const GtkMessageType msgType)
{
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, msgType, GTK_BUTTONS_OK, string);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_idle_add(idle, NULL);
    gtk_main();
}

#endif // _WIN32

void emptycallback(void) {}
void DefaultPlatformMessageBoxCallback(const char* messageStr, EMessageBoxType type )
{
#ifdef _WIN32
	switch(type)
	{
		case MSGBOX_INFO:
			MessageBoxA(GetDesktopWindow(), messageStr, "INFO", MB_OK | MB_ICONINFORMATION);
			break;
		case MSGBOX_WARNING:
			MessageBoxA(GetDesktopWindow(), messageStr, "WARNING", MB_OK | MB_ICONWARNING);
			break;
		case MSGBOX_ERROR:
			MessageBoxA(GetDesktopWindow(), messageStr, "ERROR", MB_OK | MB_ICONERROR);
			break;
		case MSGBOX_CRASH:
			MessageBoxA(GetDesktopWindow(), messageStr, "FATAL ERROR", MB_OK | MB_ICONERROR);
			break;
	}
#elif defined(LINUX) && defined(USE_GTK)
	switch(type)
	{
		case MSGBOX_INFO:
			MessageBox(messageStr, GTK_MESSAGE_INFO);
			break;
		case MSGBOX_WARNING:
			MessageBox(messageStr, GTK_MESSAGE_WARNING);
			break;
		case MSGBOX_ERROR:
			MessageBox(messageStr, GTK_MESSAGE_ERROR);
			break;
		case MSGBOX_CRASH:
			MessageBox(messageStr, GTK_MESSAGE_ERROR);
			break;
	}
#elif __APPLE__
	Str255 msg;
    c2pstrcpy(msg, string);
    SInt16 ret;
	switch(type)
	{
		case MSGBOX_INFO:
			StandardAlert(kAlertNoteAlert, msg, NULL, NULL, &ret);
			break;
		case MSGBOX_WARNING:
			StandardAlert(kAlertCautionAlert, msg, NULL, NULL, &ret);
			break;
		case MSGBOX_ERROR:
			StandardAlert(kAlertStopAlert, msg, NULL, NULL, &ret);
			break;
		case MSGBOX_CRASH:
			StandardAlert(kAlertStopAlert, msg, NULL, NULL, &ret);
			break;
	}
#else
	switch(type)
	{
		case MSGBOX_INFO:
			MsgInfo("%s", messageStr);
			break;
		case MSGBOX_WARNING:
			MsgWarning("%s", messageStr);
			break;
		case MSGBOX_ERROR:
			MsgError("%s", messageStr);
			break;
		case MSGBOX_CRASH:
			MsgError("%s", messageStr);
			break;
	}
#endif
}

PREERRORMESSAGECALLBACK g_preerror_callback = emptycallback;
MESSAGECB g_msgBoxCallback = DefaultPlatformMessageBoxCallback;

void SetPreErrorCallback(PREERRORMESSAGECALLBACK callback)
{
	g_preerror_callback = callback;
}

void SetMessageBoxCallback(MESSAGECB callback)
{
	g_msgBoxCallback = callback;
}

void ErrorMsg(const char* fmt, ...)
{
	g_preerror_callback();

	va_list		argptr;

	static char	string[4096];

	memset(string, 0, 4096);

	va_start (argptr,fmt);
	vsnprintf(string, 4096, fmt,argptr);
	va_end (argptr);

	g_msgBoxCallback(string, MSGBOX_CRASH);

	MsgError("FATAL ERROR: %s\n", string);
}

void CrashMsg(const char* fmt, ...)
{
	g_preerror_callback();

	va_list		argptr;

	static char	string[4096];

	memset(string, 0, 4096);

	va_start (argptr,fmt);
	vsnprintf(string, 4096, fmt,argptr);
	va_end (argptr);

	g_msgBoxCallback(string, MSGBOX_ERROR);

	MsgError("ERROR: %s\n", string);
}

void WarningMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];

	memset(string, 0, 4096);

	va_start (argptr,fmt);
	vsnprintf(string, 4096, fmt,argptr);
	va_end (argptr);

	g_msgBoxCallback(string, MSGBOX_WARNING);

	MsgError("WARNING: %s\n", string);
}

void InfoMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];

	memset(string, 0, 4096);

	va_start (argptr,fmt);
	vsnprintf(string, 4096, fmt,argptr);
	va_end (argptr);

	g_msgBoxCallback(string, MSGBOX_INFO);
	MsgError("INFO: %s\n", string);
}


// Utility functions
#ifdef _WIN32

#ifndef _DKLAUNCHER_
#include <malloc.h> // for _heapchk

void __CheckHeap()
{
    // Check heap status
    int heapstatus = _heapchk();
    switch ( heapstatus )
    {
    case _HEAPOK:
        Msg(" OK - heap is fine\n" );
        break;
    case _HEAPEMPTY:
        Msg(" OK - heap is empty\n" );
        break;
    case _HEAPBADBEGIN:
        Msg( " ERROR - bad start of heap\n" );
        break;
    case _HEAPBADNODE:
        Msg( " ERROR - bad node in heap\n" );
        break;
    }
}

#endif // _DKLAUNCHER_

#else

void __CheckHeap()
{

}

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
void Platform_InitTime()
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
float Platform_GetCurrentTime()
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
void Platform_Sleep(uint32 nMilliseconds)
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


void AssertValidReadPtr( void* ptr, int count/* = 1*/ )
{
#ifdef _WIN32
    ASSERT(!IsBadReadPtr( ptr, count ));
#endif
}

void AssertValidStringPtr( const char* ptr, int maxchar/* = 0xFFFFFF */ )
{
#ifdef _WIN32
    ASSERT(!IsBadStringPtr( ptr, maxchar ));
#endif
}

void AssertValidWStringPtr( const wchar_t* ptr, int maxchar/* = 0xFFFFFF */ )
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

void AssertValidReadWritePtr( void* ptr, int count/* = 1*/ )
{
#ifdef _WIN32
    ASSERT(!( IsBadWritePtr(ptr, count) || IsBadReadPtr(ptr,count)));
#endif
}

#include <stdio.h>

#ifdef _WIN32

//Developer message to special log
void AssertLogMsg(const char *fmt,...)
{
    va_list		argptr;
    static char	string[2048];

    va_start (argptr,fmt);
    vsnprintf(string,sizeof(string), fmt,argptr);
    va_end (argptr);

    char time[10];
    _strtime(time);

    fprintf(stdout,"%s: %s\n",time,string);

    FILE* g_logFile = fopen("logs/Assert.log", "a");
    if (g_logFile)
    {
        fprintf(g_logFile,"%s: %s\n",time,string);
        fclose(g_logFile);
    }
}

void _InternalAssert(const char *file, int line, const char *statement)
{
    static bool debug = true;

    if (debug)
    {
        char str[1024];

        sprintf(str, "%s\n\nFile: %s\nLine: %d\n\n", statement, file, line);

#ifndef _DKLAUNCHER_
        if (GetCore()->IsInitialized())
        {
			MsgError("\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"", file, line, statement);
        }
        else
        {
			AssertLogMsg("\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"", file, line, statement);
        }

#endif //_DKLAUNCHER_
        //if (IsDebuggerPresent())
        //{
            strcat(str, "Debug?");
            int res = MessageBoxA(NULL, str, "Assertion failed", MB_YESNOCANCEL);
            if ( res == IDYES )
            {
#if _MSC_VER >= 1400
                __debugbreak();
#else
                _asm int 0x03;
#endif
            }
            else if (res == IDCANCEL)
            {
                debug = false;
                exit(0); //Exit if we have an assert and we
            }
			/*
        }
        else
        {
            strcat(str, "Display more asserts?");
            if (MessageBoxA(NULL, str, "Assertion failed", MB_YESNO | MB_DEFBUTTON2) != IDYES)
            {
                debug = false;
                exit(0); //Exit if we have no more asserts
            }
        }*/
    }
}

void outputDebugString(const char *str)
{
    OutputDebugStringA(str);
    OutputDebugStringA("\n");
}

#else

#include <signal.h>

void _InternalAssert(const char *file, int line, const char *statement)
{
    static bool debug = true;

    if (debug)
    {
        char str[1024];

        MsgError("\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"", file, line, statement);

        // make breakpoint
		raise( SIGINT );
    }
}

void outputDebugString(const char *str)
{
    printf("%s\n", str);
}

#endif // _WIN32
