//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Base core debug interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_INTERFACE_H
#define DEBUG_INTERFACE_H

#include "InterfaceManager.h"

typedef enum 
{
	SPEW_NORM,
	SPEW_INFO,
	SPEW_WARNING,
	SPEW_ERROR,
	SPEW_SUCCESS,
}SpewType_t;

typedef void (*SpewFunc_fn)(SpewType_t,const char*);

IEXPORTS void	Msg(const char *fmt,...);
IEXPORTS void	MsgInfo(const char *fmt,...);
IEXPORTS void	MsgWarning(const char *fmt,...);
IEXPORTS void	MsgError(const char *fmt,...);
IEXPORTS void	MsgAccept(const char *fmt,...);

// developer message
IEXPORTS void	DevMsg(int level,const char *fmt,...);

IEXPORTS void	SetSpewFunction(SpewFunc_fn newfunc);

//---------------------------------------------------------------------------------------------
// FIXMEs / TODOs / NOTE macros
//---------------------------------------------------------------------------------------------
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "

#define NOTE( x )  message( x )
#define FILE_LINE  message( __FILE__LINE__ )

#define TODO( x )  message( __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  TODO :   " #x "\n" \
	" -------------------------------------------------\n" )
#define FIXME( x )  message(  __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  FIXME :  " #x "\n" \
	" -------------------------------------------------\n" )
#define todo( x )  message( __FILE__LINE__" TODO :   " #x "\n" ) 
#define fixme( x )  message( __FILE__LINE__" FIXME:   " #x "\n" ) 

#endif // DEBUG_INTERFACE_H