//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Application State handlers
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "sys_state.h"

#include "input/InputCommandBinder.h"
#include "render/IDebugOverlay.h"

namespace eqAppStateMng
{
StatePreUpdateEvent g_onPreUpdateState(PP_SL);
StatePostUpdateEvent g_onPostUpdateState(PP_SL);
StateEnterEvent g_onEnterState(PP_SL);
StateLeaveEvent g_onLeaveState(PP_SL);

static CAppStateBase* s_currentState = nullptr;
static CAppStateBase* s_nextState = nullptr;
static bool s_stateChanging = false;

// forces the current state
void SetCurrentState( CAppStateBase* state, bool force )
{
	if(!force && s_currentState == state)
		return;

	// to be changed while update
	s_nextState = state;
	s_stateChanging = true;
}

void ChangeState(CAppStateBase* state)
{
	// perform state transition
	g_onLeaveState(s_currentState, state);
	if( s_currentState )
		s_currentState->OnLeave( state );

	// call the set of callbacks provided by states
	if( state )
		state->OnEnter( s_currentState );
	g_onEnterState(s_currentState, state);

	s_currentState = state;
}

// returns the current state
CAppStateBase* GetCurrentState()
{
	return s_nextState ? s_nextState : s_currentState;
}

// returns the current state type
int	GetCurrentStateType()
{
	if(s_nextState)
		return s_nextState->GetType();

	if(s_currentState)
		return s_currentState->GetType();

	return APP_STATE_NONE;
}

void SetCurrentStateType( int stateType )
{
	SetCurrentState(GetAppStateByType(stateType));
}

void ChangeStateType(int stateType)
{
	ChangeState(GetAppStateByType(stateType));
}

void ScheduleNextState( CAppStateBase* state )
{
	if(GetCurrentState())
		GetCurrentState()->SetNextState(state);
	else
		SetCurrentState(state);
}

void ScheduleNextStateType( int stateType )
{
	ScheduleNextState(GetAppStateByType(stateType));
}

// updates and manages the states
bool UpdateStates( float fDt )
{
	PROF_EVENT("Host Update States");

	// perform state transition
	if (s_stateChanging)
	{
		ChangeState(s_nextState);
		s_nextState = nullptr;
		s_stateChanging = false;
	}

	if(!s_currentState)
	{
		Msg("--------------------------------------------\nNo current state set, exiting loop...\n--------------------------------------------\n");
		return false;
	}

	PreUpdateState(fDt);
	g_onPreUpdateState(fDt);

	if( !s_currentState->Update(fDt) )
	{
		bool forced;
		CAppStateBase* nextState = s_currentState->GetNextState( &forced );
		s_currentState->SetNextState(nullptr);

		SetCurrentState( nextState, forced );
	}

	g_onPostUpdateState(fDt);

	return true;
}

void GetStateMouseCursorProperties(bool& visible, bool& centered)
{
	if(!s_currentState)
	{
		visible = true;
		centered = false;
		return;
	}

	return s_currentState->GetMouseCursorProperties(visible, centered);
}

}

//------------------------------------------------------------------------
