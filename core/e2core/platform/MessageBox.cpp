//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef far
#	undef far
#endif
#ifdef near
#	undef near
#endif

#elif defined(USE_GTK)

void InitMessageBoxPlatform()
{
	gtk_init_check(nullptr, nullptr);
}

#include <gtk/gtk.h>

// This is such a hack, but at least it works.
static gboolean idle(gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

static void MessageBox(const char *string, const GtkMessageType msgType)
{
    GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, msgType, GTK_BUTTONS_OK, string);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_idle_add(idle, nullptr);
    gtk_main();
}

#endif // !_WIN32 && USE_GTK

#include "core/IDkCore.h"
#include "utils/KeyValues.h"
#include "core/platform/messagebox.h"

static int DefaultPlatformMessageBoxCallback(const char* messageStr, const char* titleStr, EMessageBoxType type)
{
#ifdef _WIN32
	switch(type)
	{
		case MSGBOX_INFO:
			MessageBoxA(GetDesktopWindow(), messageStr, titleStr, MB_OK | MB_ICONINFORMATION);
			break;
		case MSGBOX_WARNING:
			MessageBoxA(GetDesktopWindow(), messageStr, titleStr, MB_OK | MB_ICONWARNING);
			break;
		case MSGBOX_ERROR:
			MessageBoxA(GetDesktopWindow(), messageStr, titleStr, MB_OK | MB_ICONERROR);
			break;
		case MSGBOX_CRASH:
			MessageBoxA(GetDesktopWindow(), messageStr, titleStr, MB_OK | MB_ICONERROR);
			break;
		case MSGBOX_YESNO:
		{
			const int res = MessageBoxA(nullptr, messageStr, titleStr, MB_YESNO | MB_DEFBUTTON2);
			if (res == IDYES)
				return MSGBOX_BUTTON_YES;
			return MSGBOX_BUTTON_NO;
		}
		case MSGBOX_ABORTRETRYINGORE:
		{
			const int res = MessageBoxA(GetDesktopWindow(), messageStr, titleStr, MB_ABORTRETRYIGNORE);
			if (res == IDRETRY)
				return MSGBOX_BUTTON_RETRY;
			else if (res == IDIGNORE)
				return MSGBOX_BUTTON_IGNORE;
			else if (res == IDABORT)
				return MSGBOX_BUTTON_ABORT;
		}
	}
#elif USE_GTK

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
		case MSGBOX_ABORTRETRYINGORE:
		{
			GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_YES_NO, messageStr);
			gint result = gtk_dialog_run(GTK_DIALOG(dialog));

			const bool debug = (result == GTK_RESPONSE_YES);

			gtk_widget_destroy(dialog);
			g_idle_add(idle, nullptr);
			gtk_main();

			return MSGBOX_BUTTON_ABORT;
		}
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
		case MSGBOX_ABORTRETRYINGORE:
			StandardAlert(kAlertStopAlert, msg, nullptr, nullptr, &ret);
			return MSGBOX_BUTTON_ABORT;
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
		case MSGBOX_ABORTRETRYINGORE:
		{
			MsgError("%s", messageStr);
			return MSGBOX_BUTTON_ABORT;
		}
	}
#endif

	return 0;
}

static MessageBoxCb g_msgBoxCallback = DefaultPlatformMessageBoxCallback;

IEXPORTS MessageBoxCb SetMessageBoxCallback(MessageBoxCb callback)
{
	MessageBoxCb oldCb = g_msgBoxCallback;
	g_msgBoxCallback = callback ? callback : DefaultPlatformMessageBoxCallback;
	return oldCb;
}

IEXPORTS void ErrorMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];
	memset(string, 0, sizeof(string));

	va_start(argptr, fmt);
	CString::PrintFV(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	g_msgBoxCallback(string, "ERROR", MSGBOX_ERROR);
}

IEXPORTS void CrashMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];
	memset(string, 0, sizeof(string));

	va_start(argptr, fmt);
	CString::PrintFV(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	g_msgBoxCallback(string, "FATAL CRASH REPORT", MSGBOX_CRASH);
}

IEXPORTS void WarningMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];
	memset(string, 0, sizeof(string));

	va_start(argptr, fmt);
	CString::PrintFV(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	g_msgBoxCallback(string, "WARNING", MSGBOX_WARNING);
}

IEXPORTS void InfoMsg(const char* fmt, ...)
{
	va_list		argptr;

	static char	string[4096];
	memset(string, 0, sizeof(string));

	va_start (argptr,fmt);
	CString::PrintFV(string, sizeof(string), fmt,argptr);
	va_end (argptr);

	g_msgBoxCallback(string, "INFO", MSGBOX_INFO);
}

static void AssertLogMsg(ESpewType _dummy, const char* fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	EqString formattedStr = EqString::FormatV(fmt, argptr);
	va_end(argptr);

	fprintf(stdout, "%s\n", formattedStr.ToCString());

	FILE* assetFile = fopen("logs/Assert.log", "a");
	if (assetFile)
	{
		fprintf(assetFile, "%s\n", formattedStr.ToCString());
		fclose(assetFile);
	}
}

static int DefaultAssertHandler(PPSourceLine sl, const char* expression, const char* message, bool skipOnly)
{
	const bool eqCoreInit = g_eqCore->IsInitialized();

	EqString assertMessage;
	if (expression)
	{
		assertMessage.Append(expression);
		assertMessage.Append("\n\n");
	}

	if (message)
	{
		assertMessage.Append(message);
		assertMessage.Append("\n\n");
	}

	assertMessage.AppendFmt("file: %s\nline: %d", sl.GetFileName(), sl.GetLine());

	(eqCoreInit ? LogMsg : AssertLogMsg)(SPEW_ERROR, "\n***ASSERTION FAILED***\n%s\n******\n", assertMessage.ToCString());

	assertMessage.Append("\n\n");
	if (skipOnly)
	{
		if (g_eqCore->GetDebugSettings().crashOnAssert)
		{
			assertMessage.Append(" - CrashOnAssert mode enabled\n");
			
			if (g_eqCore->GetDebugSettings().fullCrashDumps)
				assertMessage.Append(" - Full Crash dump will be saved\n");
			else
				assertMessage.Append(" - Mini dump will be saved\n");

			const int res = g_msgBoxCallback(assertMessage + " - Application will be terminated", "Crash", MSGBOX_CRASH);
			int* _badptr = nullptr;
			*_badptr = 0;

			return _EQASSERT_SKIP;
		}

		const int res = g_msgBoxCallback(assertMessage + " - Display more asserts?", "Assertion failed", MSGBOX_YESNO);
		if (res != MSGBOX_BUTTON_YES)
			return _EQASSERT_IGNORE_ALWAYS;
	}

	if (g_eqCore->GetDebugSettings().assertPromptInDebugger)
	{
		const int res = g_msgBoxCallback(assertMessage + "\n -Press 'Abort' to Break the execution\n -Press 'Retry' to skip this assert\n -Press 'Ignore' to suppress this message", "Assertion failed", MSGBOX_ABORTRETRYINGORE);
		if (res == MSGBOX_BUTTON_RETRY)
			return _EQASSERT_SKIP;
		else if (res == MSGBOX_BUTTON_IGNORE)
			return _EQASSERT_IGNORE_ALWAYS;
		else if (res == MSGBOX_BUTTON_ABORT)
			return _EQASSERT_BREAK;

		return _EQASSERT_SKIP;
	}
	return _EQASSERT_BREAK;
}


static AssertHandlerFn s_assertHandler = DefaultAssertHandler;
IEXPORTS AssertHandlerFn SetAssertHandler(AssertHandlerFn newHandler)
{
	AssertHandlerFn oldHandler = s_assertHandler;
	s_assertHandler = newHandler;
	return oldHandler;
}

IEXPORTS int _InternalAssertMsg(PPSourceLine sl, bool isSkipped, const char* expression, const char *fmt, ...)
{
	if(isSkipped)
		return _EQASSERT_SKIP;

	EqString formattedStr;
	if (fmt)
	{
		va_list argptr;
		va_start(argptr, fmt);
		formattedStr = EqString::FormatV(fmt, argptr);
		va_end(argptr);
	}

	return s_assertHandler(sl, expression, formattedStr, Platform_IsDebuggerPresent() == false);
}

