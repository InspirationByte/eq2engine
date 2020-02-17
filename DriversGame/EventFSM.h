//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: finite state machine and events
//////////////////////////////////////////////////////////////////////////////////

#ifndef EVENTFSM_H
#define EVENTFSM_H

#include <map>

// intrenal state event
enum EStateTransition
{
	STATE_TRANSITION_NONE = 0,		// cycle

	STATE_TRANSITION_PROLOG,	// when state starts work
	STATE_TRANSITION_EPILOG,	// when state ends work
};

class CEvent;
class CFSM_Base;
class CGameObject;
typedef int (CFSM_Base::*fnStateHandler)( float fDt, EStateTransition transition );
typedef void (CFSM_Base::*fnEventHandler)( CEvent* );

//
// Events... used for... events!
//
class CEvent
{
	friend class CFSM_Base;
public:
	CEvent(CGameObject* sender) : m_sender(sender)
	{}
	virtual				~CEvent() {}
	virtual int			GetType() = 0;

protected:
	CGameObject*		m_sender;
	CEvent*				m_next;
};

//
// The basis of FSM
//
class CFSM_Base
{
public:
						CFSM_Base();
	virtual				~CFSM_Base() {}

	int					DefaultNoState() { return 0; }

	int					DoStatesAndEvents( float fDt );

	void				FSMSetState(fnStateHandler stateFn);
	void				FSMSetNextState(fnStateHandler stateFn, float delay);

	void				DoEvents();

	fnStateHandler		FSMGetCurrentState() const {return m_curState;}

	void				RaiseEvent( CEvent* evt );
	void				SetEventHandler( int type, fnEventHandler fnHandler );

private:
	void							PushEvent( CEvent* evt );

	fnStateHandler					m_curState;

	fnStateHandler					m_nextState;
	float							m_nextDelay;

	std::map<int, fnEventHandler>	m_handlers;

	CEvent*							m_firstEvent;
};

#define AI_State(fn) static_cast<fnStateHandler>(fn)
#define AI_SetState( fn ) FSMSetState(AI_State(fn))
#define AI_SetNextState( fn, delay ) FSMSetNextState(AI_State(fn), delay)

#endif // AIEVENTFSM_H