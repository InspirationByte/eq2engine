//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Application State handlers
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/event.h"

constexpr int APP_STATE_NONE = 0;

class CAppStateBase;
using StatePreUpdateEvent = Event<void(const float deltaTime)>;
using StatePostUpdateEvent = Event<void(const float deltaTime)>;
using StateEnterEvent = Event<void(CAppStateBase* oldState, CAppStateBase* newState)>;
using StateLeaveEvent = Event<void(CAppStateBase* oldState, CAppStateBase* newState)>;

//--------------------------------------------------------------------------------
// game state handler
//--------------------------------------------------------------------------------

class CAppStateBase
{
public:
	virtual ~CAppStateBase() = default;

	virtual int		GetType() const = 0;

	// when changed to this state
	// @from - used to transfer data
	virtual void	OnEnter( CAppStateBase* from ) {}

	// when the state changes to something
	// @to - used to transfer data
	virtual void	OnLeave( CAppStateBase* to ) {}

	// when 'false' returned the next state goes on
	virtual bool	Update( float fDt ) = 0;

	virtual void	HandleKeyPress( int key, bool down ) {}
	virtual void	HandleMouseClick( int x, int y, int buttons, bool down ) {}
	virtual void	HandleMouseMove( int x, int y, float deltaX, float deltaY ) {}
	virtual void	HandleMouseWheel( int x,int y,int scroll ) {}

	virtual void	HandleJoyAxis( short axis, short value ) {}

	virtual void	GetMouseCursorProperties(bool& visible, bool& centered) {visible = false; centered = false;}
	virtual float	GetTimescale() const { return 1.0f; }

	void			SetNextState(CAppStateBase* state, bool force = false)	{m_nextState = state;m_forceNextState = force;}
	CAppStateBase*	GetNextState(bool* force = nullptr) const					{if(force)*force = m_forceNextState; return m_nextState;}

private:
	CAppStateBase*	m_nextState{ nullptr };
	bool			m_forceNextState{ false };
};

//--------------------------------------------------------------------------------

namespace eqAppStateMng
{
	extern StatePreUpdateEvent	g_onPreUpdateState;
	extern StatePostUpdateEvent	g_onPostUpdateState;
	extern StateEnterEvent		g_onEnterState;
	extern StateLeaveEvent		g_onLeaveState;

	const char*			GetAppNameTitle();

	bool				InitAppStates();
	void				ShutdownAppStates();

	CAppStateBase*		GetAppStateByType(int stateType);

	// forces the current state
	void				SetCurrentState(CAppStateBase* state, bool force = false);

	void				ChangeState(CAppStateBase* state);

	// returns the current state
	CAppStateBase*		GetCurrentState();

	// returns the current state type
	int					GetCurrentStateType();
	void				SetCurrentStateType(int stateType);

	void				ChangeStateType(int stateType);

	void				ScheduleNextState(CAppStateBase* state);
	void				ScheduleNextStateType(int stateType);

	// updates and manages the states
	void				PreUpdateState(float fDt);
	void				PostUpdateState(float fDt);

	bool				UpdateStates(float fDt);
	void				GetStateMouseCursorProperties(bool& visible, bool& centered);

	bool				IsPauseAllowed();

	void				SignalPause();
};

//---------------------------------------------------------------------------------
