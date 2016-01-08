//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: OOLua binding
//////////////////////////////////////////////////////////////////////////////////

#ifndef LUABINDING_H
#define LUABINDING_H

#include <iostream>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "oolua.h"

#include "DebugInterface.h"

class IVirtualStream;

extern OOLUA::Script& GetLuaState();

namespace EqLua
{

//-----------------------------------------

class LuaStackGuard
{
public:
	LuaStackGuard(lua_State* state);
	~LuaStackGuard();

	// this resets stack to it's begin state
	void		Release();

private:
	lua_State*	m_state;
	int			m_pos;
};

//-----------------------------------------

// TODO: make it for userdata because possible

// Table function helper class
class LuaTableFuncRef
{
public:
			LuaTableFuncRef();

	// gets the reference to function
	bool					Get(const OOLUA::Table& table, const char* funcName, bool quiet = false);

	// pushes function and this
	bool					Push();

	// call function, on error return false and use OOLUA error handler
	bool					Call(int nArgs, int nRet, int nErrIndex = 0);

	OOLUA::Lua_func_ref		m_ref;
	OOLUA::Table			m_table;

	int						m_err_idx;

	bool					m_isError;
	bool					m_isTablePushed;
};

//-----------------------------------------

void	LuaBinding_Initialize(lua_State* state);

bool	LuaBinding_LoadAndDoFile(lua_State* state, const char* filename, const char* pszFuncName);
bool	LuaBinding_DoBuffer(lua_State* state, const char* data, int size, const char* filename);

//------------------------------------------

// this is a slow, but effective callbacks on userdata
template<class T>
inline bool LuaCallUserdataCallback(const T& object, const char* functionName, OOLUA::Table& argTable)
{
	// search part
	OOLUA::Script& state = GetLuaState();

	EqLua::LuaStackGuard g(state);

	OOLUA::push(state, object);

	if(lua_isnil(state, -1))
	{
		MsgError("LuaTableFuncRef::Get error: table is NIL\n");
		return false;
	}

	lua_getfield( state, -1, functionName);

	if( !lua_isfunction(state, -1) )
		return false;

	OOLUA::Lua_func_ref	funcref;
	funcref.lua_get(state, -1);
	//funcref.set_ref( state, luaL_ref(state, LUA_REGISTRYINDEX));

	g.Release();

	//-----------------------------------------------
	// call part


	// call table function, passing parameters
	if(!funcref.push(state))
		return false;

	OOLUA::push(state, object);

	int nArgs = 1;

	if(argTable.valid())
	{
		if(!argTable.push_on_stack(state))
			MsgError("LuaCallUserdataCallback can't push table on stack\n");
		nArgs++;
	}
	else
		::MsgError("LuaCallUserdataCallback can't push table on stack - invalid\n");

	int res = lua_pcall(state, nArgs, 0, 0);

	if(res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error( state );
		MsgError(":%s error:\n %s\n", functionName, OOLUA::get_last_error(state).c_str());

		return false;
	}

	return true;
}

};

#endif // LUABINDING_H
