//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "InterfaceManager.h"

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

//------------------------------------------------------------------------------------------------

IEXPORTS void	outputDebugString(const char *str);
IEXPORTS void	_InternalAssert(const char *file, int line, const char *statement);
IEXPORTS void	AssertLogMsg(const char *fmt,...);

#define			ASSERT(b) if (!(b)) _InternalAssert(__FILE__, __LINE__, #b)
#define			ASSERTMSG(b, msg) if (!(b)) _InternalAssert(__FILE__, __LINE__, msg)

IEXPORTS void	AssertValidReadPtr( void* ptr, int count = 1 );
IEXPORTS void	AssertValidWritePtr( void* ptr, int count = 1 );
IEXPORTS void	AssertValidStringPtr( const char* ptr, int maxchar = 0xFFFFFF  );
IEXPORTS void	AssertValidWStringPtr( const wchar_t* ptr, int maxchar = 0xFFFFFF );
IEXPORTS void	AssertValidReadWritePtr( void* ptr, int count = 1 );

#endif // MESSAGEBOX_H
