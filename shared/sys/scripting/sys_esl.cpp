//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua binding
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "sys_esl.h"
#include "scripting/esl_event.h"

#include "sys_esl_math.h"
#include "sys_esl_keyvalues.h"
#include "sys_esl_core.h"
#include "sys_esl_movie.h"
#include "sys_esl_animating.h"
#include "sys_esl_engine.h"
#include "sys_esl_matsystem.h"
#include "sys_esl_equi.h"

namespace eslSys
{

class CLuaStateSingleton : public CSingletonAbstract<lua_State>
{
public:
	void		Initialize();
	void		Destroy();
	bool		CheckCurrentThreadIsValid() const;
	lua_State*	GetInstancePtr();

private:
	uintptr_t	m_threadId;
};

static eslSys::CLuaStateSingleton g_luaState;

//--------------------------------------------------------------------------------------------------

static const luaL_Reg eqlua_lib_load[] = {
	{ "",			luaopen_base },
	{ LUA_LOADLIBNAME,	luaopen_package },
	{ LUA_TABLIBNAME,	luaopen_table },
	{ LUA_COLIBNAME,	luaopen_coroutine },
	//{ LUA_IOLIBNAME,	luaopen_io },
	//{ LUA_OSLIBNAME,	luaopen_os },
	{ LUA_STRLIBNAME,	luaopen_string },
	{ LUA_MATHLIBNAME,	luaopen_math },
	{ LUA_DBLIBNAME,	luaopen_debug },
#ifdef LUA_JITLIBNAME
	{ LUA_JITLIBNAME,	luaopen_jit },
#endif // LUA_JITLIBNAME
	{ nullptr,		nullptr }
};

static const luaL_Reg eqlua_lib_preload[] = {
#ifdef LUA_FFILIBNAME
	{ LUA_FFILIBNAME,	luaopen_ffi },
#endif
	{ nullptr,		nullptr }
};

static void OpenLibs(lua_State *L)
{
	const luaL_Reg *lib;
	for (lib = eqlua_lib_load; lib->func; lib++)
	{
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}

	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");

	for (lib = eqlua_lib_preload; lib->func; lib++)
	{
		lua_pushcfunction(L, lib->func);
		lua_setfield(L, -2, lib->name);
	}

	lua_pop(L, 1);
}

esl::LuaTable GetOrCreateGlobalTable(lua_State* L, const char* name)
{
	esl::ScriptState state(L);
	auto tableRes = state.GetGlobal<esl::LuaTable>(name);
	if (!tableRes || !tableRes.value)
	{
		esl::LuaTable table = state.CreateTable();
		state.SetGlobal(name, table);
		return table;
	}
	return *tableRes;
}

// see Lua BaseLib load_aux
static int LuaLoadAux(lua_State* L, int status, int envidx)
{
	if (status == LUA_OK)
	{
		if (envidx != 0) 
		{
			lua_pushvalue(L, envidx);
			if (!lua_setupvalue(L, -2, 1))
				lua_pop(L, 1);
		}
		return 1;
	}

	luaL_pushfail(L);
	lua_insert(L, -2);
	return 2;
}

static int LuaLoadFile(lua_State* L)
{
	const char* fname = luaL_optstring(L, 1, NULL);
	const char* mode = luaL_optstring(L, 2, NULL);
	const int envIdx = (!lua_isnone(L, 3) ? 3 : 0);

	const int loadStatus = esl::ScriptState(L).LoadFileBuffer(g_fileSystem->Open(fname, FS_OPEN_READ), fname);
	return LuaLoadAux(L, loadStatus, envIdx);
}

//--------------------------------------------------
// Error handling
//--------------------------------------------------

static bool s_luaErrorState = false;

void ClearLuaErrorState()
{
	s_luaErrorState = false;
}
	
bool GetLuaErrorState()
{
	return s_luaErrorState;
}

void SetLuaErrorState()
{
	s_luaErrorState = true;
}
	
const char* GetLastLuaError()
{
	const char* lastError = esl::runtime::GetLastError(g_luaState.GetInstancePtr());
	if(lastError && lastError[0] != '\0')
	{
		s_luaErrorState = true;

		// reset state
		esl::runtime::ResetErrorValue(g_luaState.GetInstancePtr());
	}
	
	return lastError;
}

static bool IsLuaDebuggerPresent()
{
	const char* vsCodeDebuggerOn = getenv("LOCAL_LUA_DEBUGGER_VSCODE");
	return vsCodeDebuggerOn && !CString::Compare(vsCodeDebuggerOn, "1");
}

static int DbgRuntimeError(lua_State* L)
{
	if (IsLuaDebuggerPresent())
	{
		esl::ScriptState state(L);
		EqStringRef errorMessage = *state.GetValue<EqStringRef>(1);
		auto assertFunc = *state.GetGlobal<esl::LuaFunctionRef>("assert");
		
		using AssertFunc = esl::runtime::FunctionCall<void, bool, EqStringRef>;
		AssertFunc::Invoke(assertFunc, false, errorMessage);
	}
	return 0;
}

static int DbgAssertHandler(PPSourceLine sl, const char* expression, const char* message, bool skipOnly)
{
	if(!Platform_IsDebuggerPresent() && IsLuaDebuggerPresent())
	{
		// try assert in Lua
		esl::ScriptState state(g_luaState.GetInstancePtr());
		auto assertFunc = *state.GetGlobal<esl::LuaFunctionRef>("assert");
		using AssertFunc = esl::runtime::FunctionCall<void, bool, EqStringRef>;

		EqStringRef errorMessage = EqString::Format("C++ assert: %s %s", expression, message);
		AssertFunc::Invoke(assertFunc, false, errorMessage);
	}
	return DefaultAssertHandler(sl, expression, message, skipOnly);
}

static void OpenDebugger(lua_State* L)
{
#ifndef _RETAIL
	esl::ScriptState state(L);

	// init lua debugger right here
	if (IsLuaDebuggerPresent())
	{
		// we need OS env
		{
			luaL_requiref(state, LUA_IOLIBNAME, luaopen_io, 1);
			lua_pop(state, 1);
			luaL_requiref(state, LUA_OSLIBNAME, luaopen_os, 1);
			lua_pop(state, 1);
		}

		const char* debuggerFilePath = getenv("LOCAL_LUA_DEBUGGER_FILEPATH");

		// Lua debugger supported:
		// https://marketplace.visualstudio.com/items?itemName=ismoh-games.second-local-lua-debugger-vscode
		state.RunChunk(EqString::Format("package.loaded['lldebugger'] = assert(loadfile(%s))()", debuggerFilePath));
		state.RunChunk("require('lldebugger').start()");
		MsgWarning("--- Lua Local Debugger path: %s ---\n", debuggerFilePath);

		esl::runtime::SetErrorHandler(DbgRuntimeError);
		SetAssertHandler(DbgAssertHandler);
	}

#endif // !_RETAIL
}

static void OnUnhandledExceptionCallback(lua_State* L)
{
	lua_Debug ar;
	int depth = 0;

	bool anythingPrinted = false;

	Msg("\nLua stack trace:\n");
	while (lua_getstack(L, depth, &ar))
	{
		int status = lua_getinfo(L, "Sln", &ar);
		ASSERT(status);

		Msg("\t %s:", ar.short_src);
		if (ar.currentline > 0)
			Msg("%d:", ar.currentline);
		if (*ar.namewhat != '\0')  /* is there a name? */
			Msg(" in function '%s'", ar.name);
		else
		{
			if (*ar.what == 'm')  /* main? */
				Msg(" in main chunk");
			else if (*ar.what == 'C' || *ar.what == 't')
				Msg(" ?");  /* C function or tail call */
			else
				Msg(" in function <%s:%d>",
					ar.short_src, ar.linedefined);
		}
		Msg("\n");
		depth++;

		anythingPrinted = true;
	}
	Msg("\n");

#ifndef _RETAIL
	if (anythingPrinted && IsLuaDebuggerPresent())
	{
		esl::ScriptState state(L);
		state.RunChunk("assert(false, 'Unhandled exception in native code')");
		return;
	}
#endif
}

//--------------------------------------------------
// Allocator setup
//--------------------------------------------------

static void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	if (nsize == 0) 
	{
		PPFree(ptr);
		return nullptr;
	}

	PPSourceLine sl = PPSourceLine::Make("Lua", 0);
	if (ptr == nullptr) // NOTE: ReAlloc on existing ptr will not overwrite SL
	{
		switch (osize)
		{
		case LUA_TSTRING:
			sl = PPSourceLine::Make("LuaString", 0);
			break;
		case LUA_TTABLE:
			sl = PPSourceLine::Make("LuaTable", 0);
			break;
		case LUA_TFUNCTION:
			sl = PPSourceLine::Make("LuaFunction", 0);
			break;
		case LUA_TUSERDATA:
			sl = PPSourceLine::Make("LuaUserdata", 0);
			break;
		case LUA_TTHREAD:
			sl = PPSourceLine::Make("LuaThread", 0);
			break;
		}
	}
	return PPDReAlloc(ptr, nsize, sl);
}

static int LuaPanic(lua_State* L)
{
	const char* msg = lua_tostring(L, -1);
	if (!msg)
		msg = "error object is not a string";

	esl::runtime::HandleRuntimeError(L);
	ASSERT_FAIL("Lua unprotected error in call to Lua API: %s\n", msg);

	return 0;  /* return to Lua to abort */
}

static lua_State* LuaNewState(void)
{
	lua_State* L = lua_newstate(LuaAlloc, nullptr);
	if (L) 
	{
		lua_atpanic(L, &LuaPanic);
		//lua_setwarnf(L, warnfoff, L);  /* default is warnings off */
	}
	return L;
}

static lua_State* AllocLuaState()
{
	lua_State* state = LuaNewState();
	OpenLibs(state);
	OpenDebugger(state);

	esl::runtime::SetGlobal(state, "loadfile", &LuaLoadFile);
	return state;
}

static void FreeLuaState(lua_State* state)
{
	lua_gc(state, LUA_GCCOLLECT, 0);
	lua_close(state);
}

static void ExceptionCbPrintLuaStackTrace()
{
	if (!g_luaState.CheckCurrentThreadIsValid())
	{
		MsgInfo("Note: Exception happened not in Lua main thread\n");
		return;
	}

	OnUnhandledExceptionCallback(g_luaState.GetInstancePtr());
}

esl::ScriptState GetScriptState()
{
	return g_luaState.GetInstancePtr();
}

void DestroyScriptState()
{
	g_luaState.Destroy();
}

// initialization function. Can be overrided
void CLuaStateSingleton::Initialize()
{
	if (!Instance)
	{
		Instance = AllocLuaState();
		m_threadId = Threading::GetCurrentThreadID();

		esl::ScriptState state(Instance);
		esl::LuaEvent::Bind(state);

		g_eqCore->AddExceptionCallback(ExceptionCbPrintLuaStackTrace);
	}
}

// deletion function. Can be overrided
void CLuaStateSingleton::Destroy()
{
	if (Instance)
	{
		g_eqCore->RemoveExceptionCallback(ExceptionCbPrintLuaStackTrace);
		FreeLuaState(Instance);
	}

	Instance = nullptr;
}

bool CLuaStateSingleton::CheckCurrentThreadIsValid() const
{
	return m_threadId == Threading::GetCurrentThreadID();
}

lua_State* CLuaStateSingleton::GetInstancePtr()
{
	Initialize();

	ASSERT_MSG(CheckCurrentThreadIsValid(), "Lua state must be accessed from the single thread");
	return Instance;
}

};

#define MAIN_SCRIPT_FILE "scripts/lua/engine.lua"

static bool eslSysInitMainScript(lua_State* L)
{
	esl::runtime::StackGuard g(L);
	esl::ScriptState state(L);
	IFileStreamPtr mainScriptFile = g_fileSystem->Open(MAIN_SCRIPT_FILE, FS_OPEN_READ);
	if (!mainScriptFile)
	{
		esl::runtime::ResetErrorValue(L);
		lua_pushfstring(L, "Main script file '%s' not found", MAIN_SCRIPT_FILE);
		esl::runtime::SetLuaErrorFromTopOfStack(L);
		return false;
	}

	return state.RunFileBuffer(mainScriptFile, mainScriptFile->GetName());
}

bool eslSysInit(const esl::ScriptState& state)
{
	ESL_SYS_INIT(eslSysCoreInit);
	ESL_SYS_INIT(eslSysDebugInit);
	ESL_SYS_INIT(eslSysFileSystemInit);
	ESL_SYS_INIT(eslSysConsoleInit);
	//ESL_SYS_INIT_EXT(eslSysNetworkingInit);

	ESL_SYS_INIT(eslSysMathInit);
	ESL_SYS_INIT(eslSysKeyValuesInit);
	ESL_SYS_INIT(eslSysDebugDrawingInit);
	ESL_SYS_INIT(eslSysInputInit);
	ESL_SYS_INIT(eslSysSoundEmitterSystemInit);
	ESL_SYS_INIT(eslSysStateManagerInit);
	ESL_SYS_INIT(eslSysMaterialSystemInit);
	ESL_SYS_INIT(eslSysHostInit);
	ESL_SYS_INIT(eslSysEquiInit);
	ESL_SYS_INIT(eslSysMoviePlayerInit);
	ESL_SYS_INIT(eslSysAnimatingInit);

	ESL_SYS_INIT(eslSysInitMainScript);

	return true;
}

void eslSysTerm()
{
	eslSysConsoleTerm();
	eslSysInputTerm();
}