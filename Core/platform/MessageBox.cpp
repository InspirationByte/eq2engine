//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#include "platform/MessageBox.h"
#include <string.h>
#include <stdarg.h>
#include "DebugInterface.h"

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

#endif // !_WIN32 && USE_GTK

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

IEXPORTS void SetPreErrorCallback(PREERRORMESSAGECALLBACK callback)
{
	g_preerror_callback = callback;
}

IEXPORTS void SetMessageBoxCallback(MESSAGECB callback)
{
	g_msgBoxCallback = callback;
}

IEXPORTS void ErrorMsg(const char* fmt, ...)
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

IEXPORTS void CrashMsg(const char* fmt, ...)
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

IEXPORTS void WarningMsg(const char* fmt, ...)
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

IEXPORTS void InfoMsg(const char* fmt, ...)
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

IEXPORTS void _InternalAssert(const char *file, int line, const char *statement)
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

#else

#include <signal.h>

IEXPORTS void _InternalAssert(const char *file, int line, const char *statement)
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

#endif //

void InitMessageBoxPlatform()
{
#if !defined(_WIN32) && defined(USE_GTK)
	gtk_init(NULL, NULL);
#endif // !_WIN32 && USE_GTK
}
