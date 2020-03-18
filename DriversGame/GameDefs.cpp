//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game definitions
//////////////////////////////////////////////////////////////////////////////////

#include "car.h"
#include "GameDefs.h"

ConVar		g_autoHandbrake("g_autoHandbrake", "1", "Auto handbrake for steering help", CV_ARCHIVE);
ConVar		g_invincibility("g_invincibility", "0", "No damage for player car", CV_CHEAT);
ConVar		g_infiniteMass("g_infiniteMass", "0", "Infinite mass for player car", CV_CHEAT);
ConVar		g_difficulty("g_difficulty", "0", "Difficulty of the game", CV_ARCHIVE);

RoadBlockInfo_t::~RoadBlockInfo_t()
{
	// clear roadblock cars
	for (int i = 0; i < activeCars.numElem(); i++)
		activeCars[i]->m_assignedRoadblock = nullptr;
}

static const slipAngleCurveParams_t s_standardSlipAngleParams = {
	18.0f,	// Initial Gradient
	1.0f,	// End Gradient
	3.5f,	// End Offset
	0.05f,	// Segment End A
	0.27f,	// Segment End B
};

static const slipAngleCurveParams_t s_AISlipAngleParams = {
	20.0f,	// Initial Gradient
	3.0f,	// End Gradient
	4.5f,	// End Offset
	0.05f,	// Segment End A
	0.35f,	// Segment End B
};

const slipAngleCurveParams_t& GetDefaultSlipCurveParams()
{
	return s_standardSlipAngleParams;
}

const slipAngleCurveParams_t& GetAISlipCurveParams()
{
	// FIXME: should be different by g_difficulty
	return s_AISlipAngleParams;
}

//-------------------------------------------------------------------------------------------

CEventSubject::CEventSubject()
{

}

CEventSubject::~CEventSubject()
{
	for (int i = 0; i < m_subscriptions.numElem(); i++)
	{
		delete m_subscriptions[i];
	}

	m_subscriptions.clear();
}

eventSubscription_t* CEventSubject::Subscribe(eventFunc func)
{
	eventSubscription_t* sub = new eventSubscription_t();
	sub->func = func;
	sub->unsubscribe = false;

	m_subscriptions.append(sub);

	return sub;
}

void CEventSubject::Raise(CGameObject* owner, const Vector3D& origin, void* data)
{
	eventArgs_t args;
	args.owner = owner;
	args.origin = origin;
	args.data = data;

	Raise(args);
}

void CEventSubject::Raise(const eventArgs_t& args)
{
	for (int i = 0; i < m_subscriptions.numElem(); i++)
	{
		eventSubscription_t* sub = m_subscriptions[i];

		if (sub->unsubscribe)
		{
			delete sub;
			m_subscriptions.fastRemoveIndex(i);
			i--;
			continue;
		}

		sub->func(args);
	}
}

// declare world events
CEventSubject g_worldEvents[EVT_COUNT];