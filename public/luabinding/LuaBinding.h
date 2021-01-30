//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OOLua binding
//////////////////////////////////////////////////////////////////////////////////

#ifndef LUABINDING_H
#define LUABINDING_H

#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "oolua.h"
#include "DebugInterface.h"

class IVirtualStream;

extern OOLUA::Script& GetLuaState();

namespace EqLua
{

int eqlua_stack_trace(lua_State* vm);
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
class LuaUserDataFuncRef
{
public:
	LuaUserDataFuncRef();

	void					Release();

	// gets the reference to function
	template <class T>
	bool					Get(OOLUA::Script& state, const T& thisObject, const char* funcName, bool quiet = false);

	// pushes function and this
	template <class T>
	bool					Push(const T& thisObject);

	// call function, on error return false and use OOLUA error handler
	bool					Call(int nArgs, int nRet, int nErrIndex = 0);

	OOLUA::Lua_func_ref		m_ref;
	lua_State*				m_state;

	int						m_err_idx;
	int						m_funcPushLevel;	// 0 - none, 1 - function, 2 - function + table

	bool					m_isError;
};
	
// Table function helper class
class LuaTableFuncRef
{
public:
	LuaTableFuncRef();

	void					Release();
	
	// gets the reference to function
	bool					Get(const OOLUA::Table& table, const char* funcName, bool quiet = false);

	// pushes function and this
	bool					Push(bool pushSelf = true);

	// call function, on error return false and use OOLUA error handler
	bool					Call(int nArgs, int nRet, int nErrIndex = 0);

	OOLUA::Lua_func_ref		m_ref;
	OOLUA::Table			m_table;

	int						m_err_idx;
	int						m_funcPushLevel;	// 0 - none, 1 - function, 2 - function + table

	bool					m_isError;
};

//-----------------------------------------

lua_State*	LuaBinding_AllocState();
void		LuaBinding_Initialize(lua_State* state);

bool		LuaBinding_LoadAndDoFile(lua_State* state, const char* filename, const char* pszFuncName);
bool		LuaBinding_DoBuffer(lua_State* state, const char* data, int size, const char* filename);

	
void		LuaBinding_ClearErrors(lua_State* state);
const char*	LuaBinding_GetLastError(lua_State* state);
bool		LuaBinding_GetErrors(DkList<EqString>* errors);

//-------------------------------------------------------------------------
// implementation for LuaUserDataFuncRef
	
// gets the reference to function
template <class T>
bool LuaUserDataFuncRef::Get(OOLUA::Script& state, const T& thisObject, const char* funcName, bool quiet)
{
	m_state = state;

	EqLua::LuaStackGuard g(state);

	OOLUA::push(state, thisObject);

	if (lua_isnil(state, -1))
	{
		MsgError("LuaUserDataFuncRef::Get error: thisObject is NIL\n");
		m_isError = true;
		return false;
	}

	lua_getfield(state, -1, funcName);

	if (!lua_isfunction(state, -1))
	{
		if (!quiet)
			MsgError("LuaUserDataFuncRef::Get error: can't get function '%s'\n", funcName);

		m_isError = true;
		return false;
	}

	m_ref.lua_get(state, -1);

	g.Release();

	m_isError = false;

	return true;
}

// pushes function and self table
template <class T>
bool LuaUserDataFuncRef::Push(const T& thisObject)
{
	if (m_isError)
		return false;

	if (m_funcPushLevel > 0)
		return false;

	// регистрируем обработчик
	lua_pushcfunction(m_state, eqlua_stack_trace);
	m_err_idx = lua_gettop(m_state);

	// call table function, passing parameters
	if (!m_ref.push(m_state))
		return false;

	m_funcPushLevel++;

	// place a parent of this function
	if (OOLUA::push(m_state, thisObject))
	{
		m_funcPushLevel++;
	}
	else
		return false;

	return true;
}

//-------------------------------------------------------------------------
	
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
		MsgError("LuaCallUserdataCallback error: table is NIL\n");
		return false;
	}

	lua_getfield( state, -1, functionName);

	if( !lua_isfunction(state, -1) )
		return false;

	OOLUA::Lua_func_ref	funcref;
	funcref.lua_get(state, -1);

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
