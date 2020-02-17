//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: scripted object
//////////////////////////////////////////////////////////////////////////////////

#include "object_scripted.h"

CObject_Scripted::CObject_Scripted( kvkeybase_t* kvdata )
{
	m_scriptName = KV_GetValueString(kvdata->FindKeyBase("bind"));
}

CObject_Scripted::~CObject_Scripted()
{
}

void CObject_Scripted::OnRemove()
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if(m_onRemove.Push())
	{
		if(!m_onRemove.Call(0, 0, 0))
			MsgError("Error in script call OnRemove:\n%s\n", OOLUA::get_last_error(state).c_str());
	}

	BaseClass::OnRemove();
}

void CObject_Scripted::Spawn()
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	BaseClass::Spawn();

	OOLUA::Table objTable;

	if(state.call("CreateScriptedObject", m_scriptName.c_str(), (CGameObject*)this))
	{
		state.pull(objTable);

		if( objTable.valid() )
		{
			m_onSpawn.Get(objTable, "OnSpawn", false);
			m_onRemove.Get(objTable, "OnRemove", false);
			m_simulate.Get(objTable, "Simulate", true);
		}
		else
			MsgError("No such scripted object '%s'!\n", m_scriptName.c_str());
	
		if(m_onSpawn.Push())
		{
			if(!m_onSpawn.Call(0, 0, 0))
				MsgError("Error in script call OnSpawn:\n%s\n", OOLUA::get_last_error(state).c_str());
		}

	}
	else
		MsgError("Error in script call CreateScriptedObject:\n%s\n", OOLUA::get_last_error(state).c_str());
}

void CObject_Scripted::Draw( int nRenderFlags )
{
	// TODO: actulally we can draw something
}

void CObject_Scripted::Simulate(float fDt)
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if(m_simulate.Push() && state.push(fDt))
	{
		if(!m_simulate.Call(1, 0, 0))
			MsgError("Error in script call Simulate:\n%s\n", OOLUA::get_last_error(state).c_str());
	}
}
