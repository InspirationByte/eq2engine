//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: finite state machine and events
//////////////////////////////////////////////////////////////////////////////////

#ifndef EVENTFSM_H
#define EVENTFSM_H

//
// Events... used for... events!
//
struct Event_t
{
	int								type;
	Event_t*						next;
};

// intrenal state event
enum EStateTransition
{
	STATE_TRANSITION_NONE = 0,		// cycle

	STATE_TRANSITION_PROLOG,	// when state starts work
	STATE_TRANSITION_EPILOG,	// when state ends work
};

class CFSM_Base;
typedef int (CFSM_Base::*fnStateHandler)( float fDt, EStateTransition transition );

//
// The basis of FSM
//
class CFSM_Base
{
public:
				CFSM_Base();
	virtual		~CFSM_Base() {}

	int			DefaultNoState() { return 0; }

	int			RunState(float fDt);

	void		FSMSetState(fnStateHandler stateFn);
	void		FSMSetNextState(fnStateHandler stateFn, float delay);

	void		PushEvent( Event_t* evt );
	void		ClearEvents();

protected:
	fnStateHandler		m_curState;

	fnStateHandler		m_nextState;
	float				m_nextDelay;

	Event_t*			m_firstEvent;
};

#define AI_SetState( fn ) FSMSetState(static_cast<fnStateHandler>(fn))
#define AI_SetNextState( fn, delay ) FSMSetNextState(static_cast<fnStateHandler>(fn), delay)

#endif // AIEVENTFSM_H