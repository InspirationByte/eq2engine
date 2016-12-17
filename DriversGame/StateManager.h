//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: State handlers
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include "Font.h"
#include "utils/KeyValues.h"
#include "materialsystem/IMaterialSystem.h"

enum EGameStateType 
{
	GAME_STATE_NONE = 0,

	GAME_STATE_TITLESCREEN,
	GAME_STATE_MAINMENU,
	GAME_STATE_MPLOBBY,
	GAME_STATE_GAME,

	GAME_STATE_COUNT,

	GAME_STATE_LUA_BEGUN,
};

//--------------------------------------------------------------------------------
// game state handler
//--------------------------------------------------------------------------------

class CBaseStateHandler
{
public:
						CBaseStateHandler() : m_nextState(NULL), m_forceNextState(false) {}
				virtual ~CBaseStateHandler(){}

	virtual int			GetType() const = 0;

	// when changed to this state
	// @from - used to transfer data
	virtual void		OnEnter( CBaseStateHandler* from ) {}

	// when the state changes to something
	// @to - used to transfer data
	virtual void		OnLeave( CBaseStateHandler* to ) {}

	// when 'false' returned the next state goes on
	virtual bool		Update( float fDt ) = 0;

	virtual void		HandleKeyPress( int key, bool down ) {}
	virtual void		HandleMouseClick( int x, int y, int buttons, bool down ) {}
	virtual void		HandleMouseMove( int x, int y, float deltaX, float deltaY ) {}
	virtual void		HandleMouseWheel( int x,int y,int scroll ) {}

	virtual void		HandleJoyAxis( short axis, short value ) {}

	virtual bool		IsMouseCursorVisible() {return false;}

	void				SetNextState(CBaseStateHandler* state, bool force = false)	{m_nextState = state;m_forceNextState = force;}
	CBaseStateHandler*	GetNextState(bool* force = NULL) const						{if(force)*force = m_forceNextState; return m_nextState;}

private:
	CBaseStateHandler*	m_nextState;
	bool				m_forceNextState;
};

//--------------------------------------------------------------------------------

// forces the current state
void					SetCurrentState( CBaseStateHandler* state, bool force = false );

void					ChangeState(CBaseStateHandler* state);

// returns the current state
CBaseStateHandler*		GetCurrentState();

// returns the current state type
int						GetCurrentStateType();
void					SetCurrentStateType( int stateType );

void					SheduleNextState( CBaseStateHandler* state );
void					SheduleNextStateType( int stateType );

// updates and manages the states
bool					UpdateStates( float fDt );
bool					GetStateMouseCursorVisibility();

void					InitRegisterStates();

extern CBaseStateHandler*	g_states[GAME_STATE_COUNT];

//---------------------------------------------------------------------------------
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
#endif // STATEMANAGER_H