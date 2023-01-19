//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: States of Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sys/sys_state.h"

/*
#ifndef EDITOR
class CLuaState : public CBaseStateHandler
{
public:
CLuaState();

int					GetType() { return m_stateType; };

void				OnEnter( CBaseStateHandler* from );
void				OnLeave( CBaseStateHandler* to );

bool				Update( float fDt );

void				HandleKeyPress( int key, bool down );
void				HandleMouseMove( int x, int y, float deltaX, float deltaY );
void				HandleMouseWheel(int x,int y,int scroll);

void				HandleJoyAxis( short axis, short value );

protected:

int						m_stateType;
OOLUA::Table			m_object;

EqLua::Lua_func_ref	m_onEnter;
EqLua::Lua_func_ref	m_onLeave;
};

#endif // EDITOR
*/

enum EGameStateType
{
	GAME_STATE_SAMPLE_GAME_DEMO,
	GAME_STATE_COUNT,

};
