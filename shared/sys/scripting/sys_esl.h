//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: system ESL Lua binding
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "scripting/esl.h"
#include "scripting/esl_bind.h"
#include "scripting/esl_luaref.h"

namespace esl { class ScriptState; }

namespace eslSys
{

esl::ScriptState GetScriptState();
void DestroyScriptState();

void ClearLuaErrorState();
bool GetLuaErrorState();
void SetLuaErrorState();

const char*	GetLastLuaError();

esl::LuaTable GetOrCreateGlobalTable(lua_State* state, const char* name);

template<typename ResultType>
bool CheckCallResult(ResultType& result, PPSourceLine sl, const char* fmt, ...)
{
	if (!result)
	{
		va_list argptr;
		va_start(argptr, fmt);
		EqString newString = EqString::FormatV(fmt, argptr);
		va_end(argptr);

#ifdef _RETAIL
		MsgError("***Lua error during %s***\n%s\n", newString.ToCString(), result.errorMessage.ToCString());
#else
		MsgError("***Lua error during %s ***\nCaller: %s:%d\n%s\n", newString.ToCString(), sl.GetFileName(), sl.GetLine(), result.errorMessage.ToCString());
#endif
		SetLuaErrorState();
	}
	return result;
}

};

// DEPRECATED
#define LUA_SET_GLOBAL_ENUMCONST_3(state, constName, constStrName)	\
		auto l_##constStrName = constName;			\
		state.SetGlobal(#constStrName, l_##constStrName)

// DEPRECATED
#define LUA_SET_GLOBAL_CONST(state, constName) \
	LUA_SET_GLOBAL_ENUMCONST_3(state, constName, constName)

#define LUA_CHECK_CALL( result, fmt, ... ) \
	eslSys::CheckCallResult(result, PP_SL, fmt, ##__VA_ARGS__)

#define ESL_SYS_INIT(initFunc) \
	if(!initFunc(state)) { return false; } \

#define ESL_SYS_INIT_EXT(initFunc) { \
	extern bool initFunc(const esl::ScriptState & state); \
		if(!initFunc(state)) { return false; } \
	}

bool eslSysInit(const esl::ScriptState& state);
void eslSysTerm();