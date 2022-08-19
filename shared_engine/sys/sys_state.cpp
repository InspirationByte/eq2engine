//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: State handlers
//////////////////////////////////////////////////////////////////////////////////

//
// TODO:
//		- General code refactoring from C-style to better C++ style
//		- Remove junk and unused code
//		- Better names instead of GAME_STATE_* names
//		- Lua state
//

#include "core/core_common.h"
#include "sys_state.h"

#include "input/InputCommandBinder.h"
#include "render/IDebugOverlay.h"

namespace EqStateMgr
{

CBaseStateHandler* g_currentState = nullptr;
CBaseStateHandler* g_nextState = nullptr;
bool g_stateChanging = false;

// forces the current state
void SetCurrentState( CBaseStateHandler* state, bool force )
{
	if(!force && g_currentState == state)
		return;

	// to be changed while update
	g_nextState = state;
	g_stateChanging = true;
}

void ChangeState(CBaseStateHandler* state)
{
	// perform state transition
	if( g_currentState )
		g_currentState->OnLeave( state );

	// call the set of callbacks provided by states
	if( state )
		state->OnEnter( g_currentState );

	g_currentState = state;
}

// returns the current state
CBaseStateHandler* GetCurrentState()
{
	return g_nextState ? g_nextState : g_currentState;
}

// returns the current state type
int	GetCurrentStateType()
{
	if(g_nextState)
		return g_nextState->GetType();

	if(g_currentState)
		return g_currentState->GetType();

	return GAME_STATE_NONE;
}

void SetCurrentStateType( int stateType )
{
	SetCurrentState( g_states[stateType] );
}

void ChangeStateType(int stateType)
{
	ChangeState( g_states[stateType] );
}

void ScheduleNextState( CBaseStateHandler* state )
{
	if(GetCurrentState())
		GetCurrentState()->SetNextState(state);
	else
		SetCurrentState(state);
}

void ScheduleNextStateType( int stateType )
{
	ScheduleNextState( g_states[stateType] );
}

// updates and manages the states
bool UpdateStates( float fDt )
{
	PROF_EVENT("Host Update States");

	// perform state transition
	if(g_nextState)
	{
		ChangeState(g_nextState);
		g_nextState = nullptr;
	}
	else if(g_stateChanging)
		g_currentState = nullptr;

	g_stateChanging = false;

	if(!g_currentState)
	{
		Msg("--------------------------------------------\nNo current state set, exiting loop...\n--------------------------------------------\n");

		return false;
	}

	PreUpdateState(fDt);

	if( !g_currentState->Update(fDt) )
	{
		bool forced;
		CBaseStateHandler* nextState = g_currentState->GetNextState( &forced );
		g_currentState->SetNextState(nullptr);

		SetCurrentState( nextState, forced );
	}

	return true;
}

void GetStateMouseCursorProperties(bool& visible, bool& centered)
{
	if(!g_currentState)
	{
		visible = true;
		centered = false;
		return;
	}

	return g_currentState->GetMouseCursorProperties(visible, centered);
}

}

//------------------------------------------------------------------------
