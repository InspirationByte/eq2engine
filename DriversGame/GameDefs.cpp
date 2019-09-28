//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#include "car.h"
#include "GameDefs.h"

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
	// FIXME: should be different by difficulty
	return s_AISlipAngleParams;
}