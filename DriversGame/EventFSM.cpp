//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: finite state machine and events
//////////////////////////////////////////////////////////////////////////////////

#include "EventFSM.h"

CFSM_Base::CFSM_Base() : 
	m_curState(nullptr), m_firstEvent(nullptr), m_nextState(nullptr), m_nextDelay(0.0f)
{
}

int CFSM_Base::RunState(float fDt)
{
	if(m_nextState || m_nextDelay > 0.0f)
	{
		m_nextDelay -= fDt;
		if(m_nextDelay <= 0.0f)
		{
			FSMSetState( m_nextState );
			m_nextState = nullptr;
		}
	}

	if(m_curState == nullptr)
		return -1;

	return (this->*m_curState)(fDt, STATE_TRANSITION_NONE);
}

void CFSM_Base::FSMSetState(fnStateHandler stateFn)
{
	if(m_curState == stateFn)
		return;	// nothing to do

	if( m_curState )
		(this->*m_curState)(0.0f, STATE_TRANSITION_EPILOG);

	m_curState = stateFn;

	if( m_curState )
		(this->*m_curState)(0.0f, STATE_TRANSITION_PROLOG);
}

void CFSM_Base::FSMSetNextState(fnStateHandler stateFn, float delay)
{
	m_nextState = stateFn;
	m_nextDelay = delay;
}

void CFSM_Base::PushEvent( Event_t* evt )
{
	evt->next = m_firstEvent;
	m_firstEvent = evt;
}

void CFSM_Base::ClearEvents()
{
	Event_t* evt = m_firstEvent;
	while(evt)
	{
		Event_t* thisEvt = evt;
		evt = evt->next;

		delete evt;
	}

	m_firstEvent = nullptr;
}