//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game definitions
//////////////////////////////////////////////////////////////////////////////////

#include "car.h"
#include "GameDefs.h"
#include "AIManager.h"

ConVar		g_autoHandbrake("g_autoHandbrake", "1", "Auto handbrake for steering help", CV_ARCHIVE);
ConVar		g_invincibility("g_invincibility", "0", "No damage for player car", CV_CHEAT);
ConVar		g_infiniteMass("g_infiniteMass", "0", "Infinite mass for player car", CV_CHEAT);
ConVar		g_difficulty("g_difficulty", "0", 0.0f, 3.0f, "Difficulty of the game", CV_ARCHIVE);

RoadBlockInfo_t::~RoadBlockInfo_t()
{
	// clear roadblock cars
	for (int i = 0; i < activeCars.numElem(); i++)
		activeCars[i]->m_assignedRoadblock = nullptr;
}

static const slipAngleCurveParams_t s_standardSlipAngleParams = {
	20.0f,	// Initial Gradient
	1.0f,	// End Gradient
	3.5f,	// End Offset
	0.05f,	// Segment End A
	0.30f,	// Segment End B
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
	static slipAngleCurveParams_t difficultySlip;
	difficultySlip = s_AISlipAngleParams;

	// FIXME: should be different than using g_difficulty
	difficultySlip.fInitialGradient += 1.0f * g_pAIManager->GetCopHandlingDifficulty();
	difficultySlip.fSegmentEndB += g_pAIManager->GetCopHandlingDifficulty() * 0.1f;

	return s_AISlipAngleParams;
}

float GetAICenterOfMassCorrection()
{
	return -0.15f * g_pAIManager->GetCopHandlingDifficulty();
}

//-------------------------------------------------------------------------------------------

struct eventSubscription_t
{
	PPMEM_MANAGED_OBJECT()

	eventSubscription_t* next;
	eventFunc_t		func;

	void*			handler;
	bool			unsubscribe;
};

void eventSub_t::Unsubscribe()
{
	if (m_sub)
		m_sub->unsubscribe = true;
	m_sub = nullptr;
}

//-------------------------------------------------------------------------------------------

CEventSubject::CEventSubject()
{
	m_subs = nullptr;
}

CEventSubject::~CEventSubject()
{
	eventSubscription_t* sub = m_subs;
	while (sub)
	{
		eventSubscription_t* nextSub = sub->next;
		delete sub;
		sub = nextSub;
	}
	m_subs = nullptr;
}

eventSubscription_t* CEventSubject::Subscribe(eventFunc_t func, void* handler)
{
	eventSubscription_t* sub = new eventSubscription_t();
	sub->func = func;
	sub->handler = handler;
	sub->unsubscribe = false;

	sub->next = m_subs;
	m_subs = sub;

	return sub;
}

void CEventSubject::Raise(void* creator, const Vector3D& origin, void* data)
{
	eventArgs_t args;
	args.creator = creator;
	args.origin = origin;
	args.data = data;

	Raise(args);
}

void CEventSubject::Raise(const eventArgs_t& args)
{
	eventArgs_t handlerArgs = args;

	eventSubscription_t* sub = m_subs;
	eventSubscription_t* prevSub = nullptr;
	while (sub)
	{
		if (sub->unsubscribe)
		{
			if (m_subs == sub)		// quickly migrate to next element
				m_subs = sub->next;

			if (prevSub)			// link previous with next
				prevSub->next = sub->next;

			delete sub;
			sub = nullptr;

			if(prevSub)				// goto (del)sub->next
				sub = prevSub->next;

			continue;
		}

		// set the right handler and go
		handlerArgs.handler = sub->handler;
		sub->func(handlerArgs);

		// goto next
		prevSub = sub;
		sub = sub->next;
	}
}

// declare world events
CEventSubject g_worldEvents[EVT_COUNT];