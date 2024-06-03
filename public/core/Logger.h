//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Base core debug interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum ESpewType
{
	SPEW_NORM,
	SPEW_INFO,
	SPEW_WARNING,
	SPEW_ERROR,
	SPEW_SUCCESS,
};

enum EDevMsg
{
	DEVMSG_CORE = (1 << 0),
	DEVMSG_FS = (1 << 2),
	DEVMSG_LOCALE = (1 << 3),
	DEVMSG_MATSYSTEM = (1 << 4),
	DEVMSG_RENDER = (1 << 5),
	DEVMSG_SOUND = (1 << 6),
	DEVMSG_NETWORK = (1 << 7),
	DEVMSG_GAME = (1 << 8),

	// all developer messages
	DEVMSG_ALL = 0xFFFF
};

using SpewFunc = void (*)(ESpewType, const char*);

// console/log messages
IEXPORTS void	LogMsgV(ESpewType spewtype, char const* fmt, va_list args);
IEXPORTS void	LogMsg(ESpewType spewtype, char const* fmt, ...) ATTRIB_FORMAT_PRINTF(2, 3);

#define _LogMsg(spewtype, fmt, ...)	\
	PRINTF_FMT_CHECK(fmt, ##__VA_ARGS__) \
	LogMsg(spewtype, fmt, ##__VA_ARGS__)

#define Msg(fmt, ...)			_LogMsg(SPEW_NORM, fmt, ##__VA_ARGS__)
#define MsgInfo(fmt, ...)		_LogMsg(SPEW_INFO, fmt, ##__VA_ARGS__)
#define MsgWarning(fmt, ...)	_LogMsg(SPEW_WARNING, fmt, ##__VA_ARGS__)
#define MsgError(fmt, ...)		_LogMsg(SPEW_ERROR, fmt, ##__VA_ARGS__)
#define MsgAccept(fmt, ...)		_LogMsg(SPEW_SUCCESS, fmt, ##__VA_ARGS__)

// developer message
IEXPORTS void	DevMsgV(int level, const char* fmt, va_list args);
IEXPORTS void	DevMsg(int level, const char* fmt, ...) ATTRIB_FORMAT_PRINTF(2, 3);

IEXPORTS void	SetSpewFunction(SpewFunc newfunc);