//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "InterfaceManager.h"

#ifdef _DKLAUNCHER_
#define IEXPORTS_LAUNCHER
#else
#define IEXPORTS_LAUNCHER IEXPORTS
#endif

enum EMessageBoxType
{
	MSGBOX_INFO = 0,
	MSGBOX_WARNING,
	MSGBOX_ERROR,
	MSGBOX_CRASH,
};

typedef void	(*PREERRORMESSAGECALLBACK)( void );
typedef void 	(*MESSAGECB)( const char* str, EMessageBoxType type );

IEXPORTS_LAUNCHER void	SetPreErrorCallback(PREERRORMESSAGECALLBACK callback);
IEXPORTS_LAUNCHER void	SetMessageBoxCallback(MESSAGECB callback);

IEXPORTS_LAUNCHER void	CrashMsg(const char* fmt, ...);
IEXPORTS_LAUNCHER void	ErrorMsg(const char* fmt, ...);
IEXPORTS_LAUNCHER void	WarningMsg(const char* fmt, ...);
IEXPORTS_LAUNCHER void	InfoMsg(const char* fmt, ...);

//------------------------------------------------------------------------------------------------

IEXPORTS_LAUNCHER void	outputDebugString(const char *str);
IEXPORTS_LAUNCHER void	_InternalAssert(const char *file, int line, const char *statement);
IEXPORTS_LAUNCHER void	AssertLogMsg(const char *fmt,...);

#define	ASSERT(b) if (!(b)) _InternalAssert(__FILE__, __LINE__, #b)
#define	ASSERTMSG(b, msg) if (!(b)) _InternalAssert(__FILE__, __LINE__, msg)

IEXPORTS_LAUNCHER void	AssertValidReadPtr( void* ptr, int count = 1 );
IEXPORTS_LAUNCHER void	AssertValidWritePtr( void* ptr, int count = 1 );
IEXPORTS_LAUNCHER void	AssertValidStringPtr( const char* ptr, int maxchar = 0xFFFFFF  );
IEXPORTS_LAUNCHER void	AssertValidWStringPtr( const wchar_t* ptr, int maxchar = 0xFFFFFF );
IEXPORTS_LAUNCHER void	AssertValidReadWritePtr( void* ptr, int count = 1 );

#endif // MESSAGEBOX_H
