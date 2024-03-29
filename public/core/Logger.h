//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Base core debug interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

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
	DEVMSG_CORE			= (1 << 0),
	DEVMSG_FS			= (1 << 2),
	DEVMSG_LOCALE		= (1 << 3),
	DEVMSG_MATSYSTEM	= (1 << 4),
	DEVMSG_RENDER		= (1 << 5),
	DEVMSG_SOUND		= (1 << 6),
	DEVMSG_NETWORK		= (1 << 7),
	DEVMSG_GAME			= (1 << 8),

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