//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EMessageBoxType
{
	MSGBOX_INFO = 0,
	MSGBOX_WARNING,
	MSGBOX_ERROR,
	MSGBOX_CRASH,

	MSGBOX_YESNO,
	MSGBOX_ABORTRETRYINGORE
};

enum EMessageBoxButton
{
	MSGBOX_BUTTON_OK = 0,

	MSGBOX_BUTTON_YES,
	MSGBOX_BUTTON_NO,

	MSGBOX_BUTTON_ABORT,
	MSGBOX_BUTTON_RETRY,
	MSGBOX_BUTTON_IGNORE,
};

using AssertHandlerFn = int(*) (PPSourceLine sl, const char* expression, const char* message, bool skipOnly);
using MessageBoxCb = int (*)(const char* messageStr, const char* titleStr, EMessageBoxType type);

IEXPORTS AssertHandlerFn	SetAssertHandler(AssertHandlerFn newHandler);
IEXPORTS MessageBoxCb		SetMessageBoxCallback(MessageBoxCb callback);

IEXPORTS void	CrashMsg(const char* fmt, ...);
IEXPORTS void	ErrorMsg(const char* fmt, ...);
IEXPORTS void	WarningMsg(const char* fmt, ...);
IEXPORTS void	InfoMsg(const char* fmt, ...);
