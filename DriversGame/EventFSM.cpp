//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: finite state machine and events
//////////////////////////////////////////////////////////////////////////////////

#include "EventFSM.h"

CFSM_Base::CFSM_Base() : 
	m_curState(nullptr), m_nextState(nullptr), m_nextDelay(0.0f)
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

