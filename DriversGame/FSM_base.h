//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: finite state machine
//////////////////////////////////////////////////////////////////////////////////

#ifndef FSM_BASE_H
#define FSM_BASE_H

#include "utils/DkList.h"

class CFSM_Base;
typedef int (CFSM_Base::*fnFunctionState)(float fDt);

class CFSM_Base
{
public:
	CFSM_Base()
	{
		m_curState = NULL;
	}

	virtual ~CFSM_Base() {}

	int DefaultNoState()
	{
		return 0;
	}

	int RunState(float fDt)
	{
		if(m_curState == NULL)
			return -1;

		return (this->*m_curState)(fDt);
	}

	void FSMSetState(fnFunctionState stateFn)
	{
		m_curState = stateFn;
	}

protected:
	fnFunctionState m_curState;
};

#define FSM_SetNextState( fn ) FSMSetState(static_cast<fnFunctionState>(fn))

#endif // FSM_BASE_H