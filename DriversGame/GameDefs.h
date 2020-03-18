//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include "dktypes.h"

enum EEventType
{
	EVT_INVALID = -1,

	// GO_CAR events
	EVT_CAR_HORN,
	EVT_CAR_SIREN,
	EVT_CAR_DAMAGE_INFLICT,		// has data as CollisionPairData_t
	EVT_CAR_DAMAGE_RECIEVE,		// has data as CollisionPairData_t
	EVT_CAR_DEATH,

	EVT_PEDESTRIAN_SCARED,

	EVT_COUNT,
};

// There are some traffic infraction types that cop can recognize
enum EInfractionType
{
	INFRACTION_NONE = 0,

	INFRACTION_HAS_FELONY,		// this car has a felony

	INFRACTION_MINOR,			// minor infraction ratio

	INFRACTION_SPEEDING,			// speeding
	INFRACTION_RED_LIGHT,			// he's went on red light
	INFRACTION_WRONG_LANE,			// he drives opposite lane

	INFRACTION_HIT_MINOR,			// hit the debris
	INFRACTION_HIT,					// hit the wall / building
	INFRACTION_HIT_VEHICLE,			// hit another vehicle
	INFRACTION_HIT_SQUAD_VEHICLE,	// hit a squad car

	INFRACTION_COUNT,
};

const float INFRACTION_SKIP_REGISTER_TIME = 2.0f;

// cop data
struct PursuerData_t
{
	DECLARE_CLASS_NOBASE(PursuerData_t);
	DECLARE_NETWORK_TABLE();
	DECLARE_EMBEDDED_NETWORKVAR();

	float	lastInfractionTime[INFRACTION_COUNT];
	bool	hasInfraction[INFRACTION_COUNT];
	float	felonyRating;

	float	lastSeenTimer;
	float	hitCheckTimer;
	float	loudhailerTimer;
	float	speechTimer;

	int		pursuedByCount;
	float	lastDirectionTimer;
	int		lastDirection;		// swne

	bool	announced;
	bool	copsHasAttention;
};

struct RoadBlockInfo_t
{
	RoadBlockInfo_t() : targetEnteredRoadblock(nullptr), runARoadblock(false)
	{}

	~RoadBlockInfo_t();

	Vector3D roadblockPosA;
	Vector3D roadblockPosB;

	int totalSpawns;
	DkList<CCar*> activeCars;

	CCar*	targetEnteredRoadblock;
	float	targetEnteredSign;
	bool	runARoadblock;
};

// curve params
struct slipAngleCurveParams_t
{
	float fInitialGradient;
	float fEndGradient;
	float fEndOffset;

	float fSegmentEndA;
	float fSegmentEndB;
};

const slipAngleCurveParams_t& GetDefaultSlipCurveParams();
const slipAngleCurveParams_t& GetAISlipCurveParams();

//--------------------------------------------------------------------

struct eventArgs_t
{
	CGameObject* owner;

	Vector3D origin;
	void* data;
};

typedef void(*eventFunc)(const eventArgs_t& args);

struct eventSubscription_t
{
	eventFunc func;
	bool unsubscribe;
};

class CEventSubject
{
public:
	CEventSubject();
	~CEventSubject();

	eventSubscription_t*	Subscribe(eventFunc func);

	void					Raise(const eventArgs_t& args);

	void					Raise(CGameObject* owner, const Vector3D& origin, void* data = nullptr);

private:
	DkList<eventSubscription_t*> m_subscriptions;
};

extern CEventSubject g_worldEvents[EVT_COUNT];

#endif // GAMEDEFS_H