//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: OOLua binding
//////////////////////////////////////////////////////////////////////////////////


#include "LuaBinding.h"
#include "IFileSystem.h"

#include "DebugInterface.h"

extern "C"
{
//#include "lundump.h"
//#include "lapi.h"
}

namespace EqLua
{

const int LEVELS1 = 10;
const int LEVELS2 = 20;

lua_State *getthread(lua_State *vm, int *arg)
{
	if(lua_isthread(vm, 1))
	{
		*arg = 1;
		return lua_tothread(vm, 1);
	}
	else
	{
		*arg = 0;
		return vm;
	}
}

int eqlua_stack_trace(lua_State *vm)
{
	int level;
	int firstpart = 1;  /* still before eventual `...' */
	int arg;
	lua_State *L1 = getthread(vm, &arg);
	lua_Debug ar;
	if (lua_isnumber(vm, arg+2)) {
		level = (int)lua_tointeger(vm, arg+2);//NOLINT
		lua_pop(vm, 1);
	}
	else
		level = (vm == L1) ? 1 : 0;  /* level 0 may be this own function */
	if (lua_gettop(vm) == arg)
		lua_pushliteral(vm, "");
	else if (!lua_isstring(vm, arg+1)) return 1;  /* message is not a string */
	else lua_pushliteral(vm, "\n");
	lua_pushliteral(vm, "stack traceback:");
	while (lua_getstack(L1, level++, &ar))
	{
		if (level > LEVELS1 && firstpart)
		{
			/* no more than `LEVELS2' more levels? */
			if (!lua_getstack(L1, level+LEVELS2, &ar))
				level--;  /* keep going */
			else
			{
				lua_pushliteral(vm, "\n\t...");  /* too many levels */
				while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
					level++;
			}
			firstpart = 0;
			continue;
		}
		lua_pushliteral(vm, "\n\t");
		lua_getinfo(L1, "Snl", &ar);
		lua_pushfstring(vm, "%s:", ar.short_src);
		if(ar.currentline > 0)
			lua_pushfstring(vm, "%d:", ar.currentline);
		if(*ar.namewhat != '\0')  /* is there a name? */
			lua_pushfstring(vm, " in function " LUA_QS, ar.name);
		else
		{
			if(*ar.what == 'm')  /* main? */
				lua_pushfstring(vm, " in main chunk");
			else if (*ar.what == 'C' || *ar.what == 't')
				lua_pushliteral(vm, " ?");  /* C function or tail call */
			else
				lua_pushfstring(vm, " in function <%s:%d>",
								ar.short_src, ar.linedefined);
		}
		lua_concat(vm, lua_gettop(vm) - arg);
	}
	lua_concat(vm, lua_gettop(vm) - arg);
	return 1;
}

//--------------------------------------------------------------------------------------------------

LuaStackGuard::LuaStackGuard(lua_State* state)
{
	m_state = state;
	m_pos = lua_gettop(m_state);
}

LuaStackGuard::~LuaStackGuard()
{
	Release();
}

void LuaStackGuard::Release()
{
	lua_settop(m_state, m_pos);
	m_pos = lua_gettop( m_state );
}

//--------------------------------------------------------------------------------------------------

LuaTableFuncRef::LuaTableFuncRef()
{
	m_isError = true;
	m_isTablePushed = false;
}

// gets the reference to function
bool LuaTableFuncRef::Get(const OOLUA::Table& table, const char* funcName, bool quiet)
{
	m_table = table;
	if(!m_table.valid())
	{
		MsgError("LuaTableFuncRef::Get error: table is not valid\n");
		m_isError = true;
		return false;
	}

	if(!m_table.safe_at(funcName, m_ref))
	{
		if(!quiet)
			MsgError("LuaTableFuncRef::Get error: can't get function '%s'\n", funcName);

		m_isError = true;
		return false;
	}

	m_isError = false;

	return true;
}

// pushes function and self table
bool LuaTableFuncRef::Push()
{
	if(m_isError)
		return false;

	lua_State* vm = m_table.state();

	// регистрируем обработчик
	lua_pushcfunction(vm, eqlua_stack_trace);
	m_err_idx = lua_gettop(vm);

	// call table function, passing parameters
	if(!m_ref.push(vm))
		return false;

	// place a parent of this function
	if(!OOLUA::push(vm, m_table))
		return false;

	m_isTablePushed = true;

	return true;
}

// call function, on error return false and use OOLUA error handler
bool LuaTableFuncRef::Call(int nArgs, int nRet, int nErrIndex/* = 0*/)
{
	if(m_isError)
		return false;

	lua_State* vm = m_table.state();

	ASSERT(m_isTablePushed);

	int res = lua_pcall(vm, nArgs+1, nRet, nErrIndex == 0 ? m_err_idx : nErrIndex);

	m_isTablePushed = false;

	if(res != 0)
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error( vm );

	return (res == 0);
}

//--------------------------------------------------------------------------------------------------

static void eqlua_open_libs(lua_State *L)
{
	LuaStackGuard s(L);

	//luaopen_io(L);
	//luaopen_base(L);
	luaopen_table(L);
	luaopen_string(L);
	luaopen_math(L);
	//luaopen_loadlib(L);
}

bool LuaBinding_LoadAndDoFile(lua_State* state, const char* filename, const char* pszFuncName)
{
	DevMsg(2, "running '%s'...\n", filename);

	LuaStackGuard s(state);

	int fnameindex = lua_gettop(state) + 1;

	std::string caller_restore;
	lua_getglobal(state, "CALLER_FILENAME");

	// it must not fault
	bool is_caller_ok = OOLUA::pull(state, caller_restore);

	// установить глобальную переменную file
	lua_pushstring(state, filename);
	lua_setglobal(state, "CALLER_FILENAME");

	long fileSize = 0;
	const char* filebuf = GetFileSystem()->GetFileBuffer(filename, &fileSize);

	if(!filebuf)
	{
		s.Release();

		OOLUA::reset_error_value(state);

		lua_pushfstring(state, "Couldn't open '%s'", filename);
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(state);
		//lua_remove(state, fnameindex);

		return false;
	}

	bool result = LuaBinding_DoBuffer(state, filebuf, fileSize, filename);

	PPFree((void*)filebuf);

	// установить глобальную переменную file
	if( is_caller_ok )
	{
		lua_pushstring(state, caller_restore.c_str());
		lua_setglobal(state, "CALLER_FILENAME");
	}

	return result;
	//return OOLUA::run_file(state, filename);
}


bool LuaBinding_DoBuffer(lua_State* state, const char* data, int size, const char* filename)
{
	LuaStackGuard s(state);

	// регистрируем обработчик
	lua_pushcfunction(state, eqlua_stack_trace);
	int err_idx = lua_gettop(state);

	// установить глобальную переменную file
	lua_pushstring(state, filename);
	lua_setglobal(state, "CALLER_FILENAME");

	int res = luaL_loadbuffer(state, data, size, filename);

	if(res != 0)
	{
		if (res != LUA_ERRMEM)
			OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(state);

		return false;
	}

	// вызов скрипта
	res = lua_pcall(state, 0, 1, err_idx);

	if (res == 0)
		return true;

	if (res != LUA_ERRMEM)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(state);
		return false;
	}

	MsgError("LuaBinding_DoBuffer: lua_pcall error\n");

	return false;

}

// чисто модулезагрузчик есть жи
int LuaBinding_ModuleFunc(lua_State* state)
{
	EqString filename = lua_tostring(state, -1);
	
	lua_getglobal( state, "CALLER_FILENAME" );
	const char* curr_file = lua_tostring(state, -1);

	if(curr_file)
	{
		EqString fileBase(_Es(curr_file).Path_Strip_Name());

		EqString checkFilename = fileBase + filename;

		if( GetFileSystem()->FileExist(checkFilename.c_str()) )
			filename = checkFilename;
	}
	
	if( !LuaBinding_LoadAndDoFile(state, filename.c_str(), "module()") )
	{
		EqString err = OOLUA::get_last_error(state).c_str();
		OOLUA::reset_error_value(state);
		luaL_error(state, "module() error:\n%s\n", err.c_str());

		return 0;
	}
	
	LuaStackGuard s(state);

	if(curr_file)
	{
		// установить глобальную переменную file назад
		lua_pushstring(state, curr_file);
		lua_setglobal(state, "__FILE__");
	}

	return 0;
}

int LuaErrorHandler(lua_State* L)
{
	//stack: err
	lua_getglobal(L, "debug"); // stack: err debug
	lua_getfield(L, -1, "traceback"); // stack: err debug debug.traceback

	// debug.traceback() возвращает 1 значение
	if(lua_pcall(L, 0, 1, 0))
	{
		const char* err = lua_tostring(L, -1);

		MsgError("Error in debug.traceback() call: %s\n", err);
	}
	else
	{
		// stack: err debug stackTrace
		lua_insert(L, -2); // stack: err stackTrace debug
		lua_pop(L, 1); // stack: err stackTrace
		lua_pushstring(L, "Error:"); // stack: err stackTrace "Error:"
		lua_insert(L, -3); // stack: "Error:" err stackTrace  
		lua_pushstring(L, "\n"); // stack: "Error:" err stackTrace "\n"
		lua_insert(L, -2); // stack: "Error:" err "\n" stackTrace
		lua_concat(L, 4); // stack: "Error:"+err+"\n"+stackTrace
	}

	return 1;
}

void LuaBinding_Initialize(lua_State* state)
{
	eqlua_open_libs(state);

	OOLUA::set_global(state, "module", LuaBinding_ModuleFunc);
}

};