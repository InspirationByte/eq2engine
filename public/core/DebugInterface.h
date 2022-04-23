//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Base core debug interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_INTERFACE_H
#define DEBUG_INTERFACE_H

#include "InterfaceManager.h"
#include <stdarg.h>

typedef enum 
{
	SPEW_NORM,
	SPEW_INFO,
	SPEW_WARNING,
	SPEW_ERROR,
	SPEW_SUCCESS,
}SpewType_t;

enum EDevMsg
{
	DEVMSG_CORE = (1 << 0),		// eqCore and engine
	DEVMSG_FS = (1 << 2),		// filesystem-related
	DEVMSG_LOCALE = (1 << 3),		// localization
	DEVMSG_MATSYSTEM = (1 << 4),		// material system
	DEVMSG_SHADERAPI = (1 << 5),		// render api
	DEVMSG_SOUND = (1 << 6),		// sound
	DEVMSG_NETWORK = (1 << 7),		// network
	DEVMSG_GAME = (1 << 8),		// game debug

	// all developer messages
	DEVMSG_ALL = 0xFFFF
};

typedef void (*SpewFunc_fn)(SpewType_t,const char*);

// console/log messages
IEXPORTS void	LogMsgV(SpewType_t spewtype, char const* pMsgFormat, va_list args);
IEXPORTS void	LogMsg(SpewType_t spewtype, char const* fmt, ...);

#define Msg(fmt, ...)			LogMsg(SPEW_NORM, fmt, ##__VA_ARGS__)
#define MsgInfo(fmt, ...)		LogMsg(SPEW_INFO, fmt, ##__VA_ARGS__)
#define MsgWarning(fmt, ...)	LogMsg(SPEW_WARNING, fmt, ##__VA_ARGS__)
#define MsgError(fmt, ...)		LogMsg(SPEW_ERROR, fmt, ##__VA_ARGS__)
#define MsgAccept(fmt, ...)		LogMsg(SPEW_SUCCESS, fmt, ##__VA_ARGS__)

// developer message
IEXPORTS void	DevMsgV(int level, const char* fmt, va_list args);
IEXPORTS void	DevMsg(int level, const char *fmt,...);

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