//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
};

typedef void	(*PREERRORMESSAGECALLBACK)( void );
typedef void 	(*MESSAGECB)( const char* str, EMessageBoxType type );

IEXPORTS void	SetPreErrorCallback(PREERRORMESSAGECALLBACK callback);
IEXPORTS void	SetMessageBoxCallback(MESSAGECB callback);

IEXPORTS void	CrashMsg(const char* fmt, ...);
IEXPORTS void	ErrorMsg(const char* fmt, ...);
IEXPORTS void	WarningMsg(const char* fmt, ...);
IEXPORTS void	InfoMsg(const char* fmt, ...);
