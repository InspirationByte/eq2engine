//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Triggers
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "Effects.h"

class CLogicRelay : public BaseEntity
{
public:
	DEFINE_CLASS_BASE(BaseEntity);

	CLogicRelay()
	{
		m_fDelay = 0.0f;
	}

	void InputCall(inputdata_t &data)
	{
		variable_t val;
		m_OutputOnCall.FireOutput(val, data.pActivator, this, m_fDelay);
	}

protected:
	DECLARE_DATAMAP();

	CBaseEntityOutput	m_OutputOnCall;

	float m_fDelay;
};

// declare save data
BEGIN_DATAMAP(CLogicRelay)

	DEFINE_KEYFIELD( m_fDelay,	"delay", VTYPE_FLOAT),

	DEFINE_OUTPUT( "OnCall", m_OutputOnCall),
	DEFINE_INPUTFUNC( "Call", InputCall),

END_DATAMAP()

DECLARE_ENTITYFACTORY(logic_relay, CLogicRelay)
