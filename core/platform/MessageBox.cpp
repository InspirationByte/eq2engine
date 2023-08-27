//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/platform/messagebox.h"

#if !defined(_WIN32) && defined(USE_GTK)

void InitMessageBoxPlatform();

#include <gtk/gtk.h>

// This is such a hack, but at least it works.
gboolean idle(gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

void MessageBox(const char *string, const GtkMessageType msgType)
{
    GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, msgType, GTK_BUTTONS_OK, string);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_idle_add(idle, nullptr);
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
			MessageBoxA(GetDesktopWindow(), messageStr, "OMG IT'S CRASHED", MB_OK | MB_ICONERROR);
			break;
	}
#elif defined(LINUX) && defined(USE_GTK)

    InitMessageBoxPlatform();

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
			StandardAlert(kAlertNoteAlert, msg, nullptr, nullptr, &ret);
			break;
		case MSGBOX_WARNING:
			StandardAlert(kAlertCautionAlert, msg, nullptr, nullptr, &ret);
			break;
		case MSGBOX_ERROR:
			StandardAlert(kAlertStopAlert, msg, nullptr, nullptr, &ret);
			break;
		case MSGBOX_CRASH:
			StandardAlert(kAlertStopAlert, msg, nullptr, nullptr, &ret);
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

	g_msgBoxCallback(string, MSGBOX_ERROR);

#ifndef _DKLAUNCHER_
	MsgError("ERROR: %s\n", string);
#endif // _DKLAUNCHER_
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

	g_msgBoxCallback(string, MSGBOX_CRASH);

#ifndef _DKLAUNCHER_
	MsgError("FATAL ERROR: %s\n", string);
#endif // _DKLAUNCHER_
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

#ifndef _DKLAUNCHER_
	MsgError("WARNING: %s\n", string);
#endif // _DKLAUNCHER_
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

#ifndef _DKLAUNCHER_
	MsgError("INFO: %s\n", string);
#endif // _DKLAUNCHER_
}

static void AssertLogMsg(SpewType_t _dummy, const char* fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	EqString formattedStr = EqString::FormatVa(fmt, argptr);
	va_end(argptr);

	fprintf(stdout, "%s\n", formattedStr.ToCString());

	FILE* assetFile = fopen("logs/Assert.log", "a");
	if (assetFile)
	{
		fprintf(assetFile, "%s\n", formattedStr.ToCString());
		fclose(assetFile);
	}
}

#ifdef _WIN32

IEXPORTS int _InternalAssertMsg(PPSourceLine sl, bool isSkipped, const char *fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	EqString formattedStr = EqString::FormatVa(fmt, argptr);
	va_end(argptr);

	const bool eqCoreInit = g_eqCore->IsInitialized();
	(eqCoreInit ? LogMsg : AssertLogMsg)(SPEW_ERROR, "\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"\n", sl.GetFileName(), sl.GetLine(), formattedStr.ToCString());

	if(isSkipped)
	{
		return _EQASSERT_SKIP;
	}

	EqString messageStr = EqString::Format("%s\n\nFile: %s\nLine: %d\n\n", formattedStr.ToCString(), sl.GetFileName(), sl.GetLine());
	if (IsDebuggerPresent())
	{
		const int res = MessageBoxA(nullptr, messageStr + "Press 'Retry' to Break the execution", "Assertion failed", MB_ABORTRETRYIGNORE);
		if (res == IDRETRY)
		{
			return _EQASSERT_SKIP;
		}
		else if (res == IDIGNORE)
		{
			return _EQASSERT_IGNORE_ALWAYS;
		}
		else if (res == IDABORT)
		{
			return _EQASSERT_BREAK;
		}
	}
	else
	{
		const int res = MessageBoxA(nullptr, messageStr + " - Display more asserts?", "Assertion failed", MB_YESNO | MB_DEFBUTTON2);
		if (res != IDYES)
		{
			return _EQASSERT_IGNORE_ALWAYS;
		}
	}

	return _EQASSERT_SKIP;
}

#else

IEXPORTS int _InternalAssertMsg(PPSourceLine sl, bool isSkipped, const char* fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	EqString formattedStr = EqString::FormatVa(fmt, argptr);
	va_end(argptr);

	const bool eqCoreInit = g_eqCore->IsInitialized();
	(eqCoreInit ? LogMsg : AssertLogMsg)(SPEW_ERROR, "\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"\n", sl.GetFileName(), sl.GetLine(), formattedStr.ToCString());

	if (isSkipped)
	{
		return _EQASSERT_SKIP;
	}

#ifndef USE_GTK
	ErrorMsg("\n*Assertion failed, file \"%s\", line %d\n*Expression \"%s\"\n", sl.GetFileName(), sl.GetLine(), formattedStr.ToCString());
#else

	EqString messageStr = EqString::Format("%s\n\nFile: %s\nLine: %d\n\nDebug?", formattedStr.ToCString(), sl.GetFileName(), sl.GetLine());

    InitMessageBoxPlatform();

    GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_YES_NO, messageStr.ToCString());
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    const bool debug = (result == GTK_RESPONSE_YES);

    gtk_widget_destroy(dialog);
    g_idle_add(idle, nullptr);
    gtk_main();

    if (debug)
#endif // USE_GTK
    {
		return _EQASSERT_BREAK;
    }

	return _EQASSERT_SKIP;
}

#endif //

void InitMessageBoxPlatform()
{
#if !defined(_WIN32) && defined(USE_GTK)
	gtk_init_check(nullptr, nullptr);
#endif // !_WIN32 && USE_GTK
}