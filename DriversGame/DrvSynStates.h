//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: States of Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRVSYNSTATES_H
#define DRVSYNSTATES_H

#include "sys_state.h"

/*
#ifndef NO_LUA
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

EqLua::LuaTableFuncRef	m_onEnter;
EqLua::LuaTableFuncRef	m_onLeave;
};

#endif // NO_LUA
*/

enum EGameStateType
{
	GAME_STATE_TITLESCREEN = GAME_STATE_NONE+1,
	GAME_STATE_MAINMENU,
	GAME_STATE_MPLOBBY,
	GAME_STATE_GAME,

	GAME_STATE_COUNT,

	GAME_STATE_LUA_BEGUN,
};

#endif // DRVSYNSTATES_H