//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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

#include "StateManager.h"
#include "session_stuff.h"

#include "state_title.h"
#include "state_mainmenu.h"

#include "physics.h"
#include "world.h"

#include "KeyBinding/Keys.h"

#include "IDebugOverlay.h"

CBaseStateHandler*	g_states[GAME_STATE_COUNT];

CBaseStateHandler* g_currentState = NULL;

// forces the current state
void SetCurrentState( CBaseStateHandler* state, bool force )
{
	if(!force && g_currentState == state)
		return;

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
	return g_currentState;
}

// returns the current state type
int	GetCurrentStateType()
{
	if(g_currentState)
		return g_currentState->GetType();

	return GAME_STATE_NONE;
}

void SetCurrentStateType( int stateType )
{
	SetCurrentState( g_states[stateType] );
}

void SheduleNextState( CBaseStateHandler* state )
{
	if(GetCurrentState())
		GetCurrentState()->SetNextState(state);
	else
		SetCurrentState(state);
}

void SheduleNextStateType( int stateType )
{
	SheduleNextState( g_states[stateType] );
}

// updates and manages the states
bool UpdateStates( float fDt )
{
	if(!g_currentState)
	{
		Msg("--------------------------------------------\nNo current state set, exiting loop...\n--------------------------------------------\n");

		/*
		materials->Setup2D(g_pHost->m_nWidth, g_pHost->m_nHeight);
		g_pShaderAPI->Clear( true,true, false );

		IEqFont* font = g_fontCache->GetFont("default", 0, 0);

		font->DrawTextEx("UpdateStates: No state set", 5,5,30,30, TEXT_ORIENT_RIGHT, 0, true);
		*/
		return false;
	}

	if( !g_currentState->Update(fDt) )
	{
		bool forced;
		CBaseStateHandler* nextState = g_currentState->GetNextState( &forced );
		g_currentState->SetNextState(NULL);
		SetCurrentState( nextState, forced);
	}

	return true;
}

void InitRegisterStates()
{
	for(int i = 0; i < GAME_STATE_COUNT; i++)
		g_states[i] = NULL;

	g_states[GAME_STATE_GAME] = g_State_Game;
	g_states[GAME_STATE_TITLESCREEN] = g_State_Title;
	g_states[GAME_STATE_MAINMENU] = g_State_MainMenu;
	
	// TODO: other states

	// init the current state
	//SetCurrentState( g_states[GAME_STATE_TITLESCREEN] );
	//SetCurrentState( g_states[GAME_STATE_GAME] );
}

//------------------------------------------------------------------------