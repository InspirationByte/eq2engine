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

int CFSM_Base::DoStatesAndEvents(float fDt)
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

	DoEvents();

	int status = (this->*m_curState)(fDt, STATE_TRANSITION_NONE);

	return status;
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

void CFSM_Base::RaiseEvent( CEvent* evt )
{
	PushEvent( evt );
}

void CFSM_Base::PushEvent( CEvent* evt )
{
	evt->m_next = m_firstEvent;
	m_firstEvent = evt;
}

void CFSM_Base::DoEvents()
{
	CEvent* evt = m_firstEvent;
	m_firstEvent = nullptr;

	while(evt)
	{
		int evtType = evt->GetType();

		if(m_handlers.count( evtType ) > 0)
		{
			fnEventHandler fnHandler = m_handlers[evtType];
			(this->*fnHandler)( evt );
		}

		CEvent* delEvent = evt;

		evt = evt->m_next;

		delete delEvent;
	}
}

void CFSM_Base::SetEventHandler( int type, fnEventHandler fnHandler )
{
	if(fnHandler == NULL)
	{
		m_handlers.erase(type);
		return;
	}

	m_handlers[type] = fnHandler;
}