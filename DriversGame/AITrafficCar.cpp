//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: traffic car controller AI
//////////////////////////////////////////////////////////////////////////////////

#include "AITrafficCar.h"
#include "session_stuff.h"
#include "heightfield.h"
#include "IDebugOverlay.h"
#include "world.h"
#include "math/Volume.h"

#include "Shiny.h"

#pragma todo("Peer2peer network model for AI cars")
#pragma todo("Teach how to brake to the line strictly - enter 'required speed' parameter")

ConVar g_traffic_maxspeed("g_trafficMaxspeed", "50.0", NULL, CV_CHEAT); // FIXME: DO NOT USE!
ConVar g_disableTrafficLights("g_disableTrafficLights", "0", NULL, CV_CHEAT);

ConVar ai_traffic_debug_steering("ai_traffic_debug_steering", "0", NULL, CV_CHEAT);
ConVar ai_traffic_debug_junctions("ai_traffic_debug_junctions", "0", NULL, CV_CHEAT);
ConVar ai_traffic_debug_straigth("ai_traffic_debug_straigths", "0", NULL, CV_CHEAT);

ConVar ai_traffic_debug_route("ai_traffic_debug_route", "0", NULL, CV_CHEAT);

ConVar ai_changelane("ai_changelane", "-1", NULL, CV_CHEAT);

ConVar ai_debug_freeze("ai_debug_freeze", "0", NULL, 0);

// here's some signal sequences i've recorded

struct signalVal_t
{
	float start;
	float end;
};

signalVal_t g_sigSeq1[] = {
	{0, 0.133333},
	{0.183333, 0.266667},
	{0.35, 0.433333},
	{0.516667, 1.28333},
};

signalVal_t g_sigSeq2[] = {
	{0, 0.0833333},
	{0.166667, 0.283333},
	{0.466667, 1.36667},
};

signalVal_t g_sigSeq3[] = {
	{0, 0.216667},
	{0.283333, 1.05},
};

signalVal_t g_sigSeq4[] = {
	{0, 0.683333},
	{0.766667, 1.55},
	{1.65, 2.61666},
};

signalVal_t g_sigSeq5[] = {
	{0, 0.316667},
	{0.416667, 0.733333},
	{0.883333, 1},
	{1.1, 1.2},
	{1.3, 1.66667},
	{1.8, 1.9},
	{2, 2.1},
	{2.18333, 2.3},
	{2.4, 2.76666},
	{2.86666, 3},
	{3.1, 3.73333},
};

struct signalSeq_t
{
	signalVal_t*	seq;
	int				numVal;
	float			length;
};

signalSeq_t g_signalSequences[] = {
	//{g_sigSeq5, elementsOf(g_sigSeq5), g_sigSeq5[elementsOf(g_sigSeq5)-1].end },
	{g_sigSeq1, elementsOf(g_sigSeq1), g_sigSeq1[elementsOf(g_sigSeq1)-1].end },
	{g_sigSeq2, elementsOf(g_sigSeq2), g_sigSeq2[elementsOf(g_sigSeq2)-1].end },
	{g_sigSeq3, elementsOf(g_sigSeq3), g_sigSeq3[elementsOf(g_sigSeq3)-1].end },
	{g_sigSeq4, elementsOf(g_sigSeq4), g_sigSeq4[elementsOf(g_sigSeq4)-1].end },
};

const int SIGNAL_SEQUENCE_COUNT = elementsOf(g_signalSequences);

// wheel friction modifier on diferrent weathers
static float trafficSpeedModifier[WEATHER_COUNT] =
{
	1.0f,
	0.9f,
	0.85f
};

static float trafficBrakeDistModifier[WEATHER_COUNT] =
{
	1.0f,
	1.2f,
	1.45f
};

extern ConVar g_traffic_maxdist;

#define ROADNEIGHBOUR_OFFS_PX(x)		{x+1, x, x-1, x}		// non-diagonal, perpendicular
#define ROADNEIGHBOUR_OFFS_PY(y)		{y, y-1, y, y+1}

const float AI_STOPLINE_DIST = 4.0f;
const float AI_OBSTACLE_DIST = 4.5f;

const float AICAR_THINK_TIME = 0.15f;

const float AI_SIDECHECK_DIST_FR = 5.0f;
const float AI_SIDECHECK_DIST_RR = 20.0f;

const float AI_ROAD_STOP_DIST = 14.0f;

const float AI_CAR_TRACE_DIST_MIN = 6.0f;
const float AI_CAR_TRACE_DIST_MAX = 25.0f;

const float AI_ROAD_TRACE_DIST = 30.0f;
const float AI_MIN_LANE_CHANGE_DIST = 30.0f;

const float AI_TARGET_DIST_NOLANE = 5.0f;
const float AI_TARGET_DIST = 10.0f;
const float AI_TARGET_EXTENDED_DIST = 30.0f;

const float AI_LANE_SWITCH_DELAY = 12.0f;

const float AI_EMERGENCY_ESCAPE_TIME = 0.5f;

const float AI_TRAFFIC_MAX_DAMAGE = 2.0f;

#define STRAIGHT_CURRENT	0
#define STRAIGHT_PREV		1

//-------------------------------------------------------------------------------

const float AI_LONG_CAR_CONST = 3.25f;

// remember: first lane is rightmost!
bool SolveIntersection(	int curDirection,
						int destDirection,
						int curRoadWidthLanes, int curLane,
						int destRoadWidth, int& destLane,
						bool isLongCar,
						bool availDirs[4])
{
	int availDirsCount = 0;

	bool straightAvail = false;
	bool leftTurnAvail = false;
	bool rightTurnAvail = false;

	// there only can be 3 directions
	for(int i = 0; i < 4; i++)
	{
		if(availDirs[i])
		{
			if(!leftTurnAvail)
				leftTurnAvail = CompareDirection(curDirection, i+1);

			if(!rightTurnAvail)
				rightTurnAvail = CompareDirection(curDirection, i-1);

			if(!straightAvail)
				straightAvail = CompareDirection(curDirection, i);

			availDirsCount++;
		}
	}

	// only one direction is available and this is one of them?
	if(availDirsCount == 1 && availDirs[destDirection])
	{
		// go to the corresponding lane
		destLane = min(curLane, destRoadWidth);
		return true;
	}

	if( curLane == curRoadWidthLanes )	// last (left) lane
	{
		if(curDirection == destDirection)	// check forward
		{
			destLane = min(curLane, destRoadWidth);
			return true;
		}
		
		// only allowed to turn left, or go forward
		if( CompareDirection(curDirection, destDirection+1) )	// go left
		{
			// left turns to the left only
			destLane = destRoadWidth;
			return true;
		}

		return false;
	}
	else if( curLane == 1 ) // first (right) lane
	{
		if(curDirection == destDirection)	// check forward
		{
			destLane = min(curLane, destRoadWidth);
			return true;
		}

		if(CompareDirection(curDirection, destDirection-1))	// go right
		{
			// allow long cars to go into the second lane, otherwise only to the first lane
			if(destRoadWidth >= 2 && isLongCar)
				destLane = 2;
			else
				destLane = 1;

			return true;
		}

		return false;
	}
	else // other lanes
	{
		// if forward is not an variant
		if(!availDirs[curDirection])
		{
			// split by half
			float rightToLeftMod = (float)curLane / (float)curRoadWidthLanes;

			if(rightToLeftMod < 0.5f && !CompareDirection(curDirection, destDirection-1))	// right
			{
				return false;
			}

			if(rightToLeftMod > 0.5f && !CompareDirection(curDirection, destDirection+1))	// left
			{
				return false;
			}
		}
		else if(!CompareDirection(curDirection, destDirection))		// go forward pls
		{
			return false;
		}
	}

	destLane = min(curLane, destRoadWidth);

	return true;
}

//----------------------------------------------------------------------------------------------------------------

CAITrafficCar::CAITrafficCar( vehicleConfig_t* carConfig ) : CCar(carConfig), CFSM_Base()
{
	m_frameSkip = false;

	m_changingLane = false;
	m_changingDirection = false;

	m_autohandbrake = false;
	m_autogearswitch = false;

	m_thinkTime = 0.0f;
	m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;
	m_refreshTime = AICAR_THINK_TIME;
	m_emergencyEscape = false;
	m_emergencyEscapeTime = 0.0f;
	m_emergencyEscapeSteer = 1.0f;
	m_prevFract = 1.0f;

	SetMaxDamage(AI_TRAFFIC_MAX_DAMAGE);

	m_signalSeq = NULL;
	m_signalSeqFrame = 0;

	m_condition = 0;

	m_gearboxShiftThreshold = 0.6f;
}

CAITrafficCar::~CAITrafficCar()
{

}

void CAITrafficCar::InitAI( bool isParked )
{
	m_speedModifier = g_replayRandom.Get(0,10);

	// helps kickstart the AI from script environment
	// before the level is loaded
	AI_SetState( &CAITrafficCar::InitTrafficState );
}

int CAITrafficCar::InitTrafficState( float fDt, EStateTransition transition )
{
	if(transition == STATE_TRANSITION_NONE)
	{
		UpdateTransform();

		Vector3D carPos = GetOrigin();

		// calc steering dir
		straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPos(carPos, 32);

		if(road.direction != -1)
		{
			road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.start);

			ChangeRoad( road );

			if(IsEnabled() && GetPhysicsBody())
				SetVelocity(GetForwardVector() * g_traffic_maxspeed.GetFloat() * KPH_TO_MPS * 0.5f);

			AI_SetState( &CAITrafficCar::TrafficDrive );
		}
		else
			AI_SetState( &CAITrafficCar::SearchForRoad );
	}

	return 0;
}

void CAITrafficCar::Spawn()
{
	BaseClass::Spawn();

	// try this, after first collision you must change it
	//GetPhysicsBody()->SetCollideMask( COLLIDEMASK_VEHICLE );

	// perform lazy collision checks
	GetPhysicsBody()->SetMinFrameTime(1.0f / 30.0f);

	// also randomize lane switching
	//m_nextSwitchLaneTime = g_replayRandom.Get(0, AI_LANE_SWITCH_DELAY);
}

int CAITrafficCar::DeadState( float fDt, EStateTransition transition )
{
	SignalNoSequence(0,0);
	int buttons = GetControlButtons();
	buttons &= ~(IN_HORN | IN_ACCELERATE | IN_BRAKE);

	SetControlButtons(buttons);
	return 0;
}

ConVar g_disableTrafficCarThink("g_disableTrafficCarThink", "0", "Disables traffic car thinking", CV_CHEAT);

void CAITrafficCar::OnPrePhysicsFrame( float fDt )
{
	if(ai_debug_freeze.GetBool())
	{
		GetPhysicsBody()->m_flags |= BODY_FORCE_PRESERVEFORCES;
		GetPhysicsBody()->Freeze();
	}
	else
		GetPhysicsBody()->Wake();

	BaseClass::OnPrePhysicsFrame(fDt);

	PROFILE_BEGIN(AITrafficCar_Think);

	if( !g_disableTrafficCarThink.GetBool())
	{
		m_thinkTime -= fDt;

		if(m_thinkTime <= 0.0f)
		{
			if( IsAlive() )
				UpdateLightsState();

			int res = DoStatesAndEvents( m_refreshTime+fDt );
			m_thinkTime = m_refreshTime;
		}

		if(m_straights[STRAIGHT_CURRENT].direction == -1)
		{
			if(FSMGetCurrentState() == &CAITrafficCar::TrafficDrive)
			{
				AI_SetState( &CAITrafficCar::SearchForRoad );
			}
		}

		int controls = m_controlButtons;

		// play signal sequence
		if(m_hornTime.IsOn())
		{
			if(m_signalSeq && m_signalSeqFrame < m_signalSeq->numVal)
			{
				float curSeqTime = m_signalSeq->length - m_hornTime.GetRemainingTime();

				if( curSeqTime >= m_signalSeq->seq[m_signalSeqFrame].start &&
					curSeqTime <= m_signalSeq->seq[m_signalSeqFrame].end )
					controls |= IN_HORN;
				else
					m_controlButtons &= ~IN_HORN;

				if(curSeqTime > m_signalSeq->seq[m_signalSeqFrame].end)
					m_signalSeqFrame++;
			}
			else if(!m_signalSeq) // simple signal
				controls |= IN_HORN;
			else
				m_controlButtons &= ~IN_HORN;
		}

		m_controlButtons = controls;

		m_hornTime.Update(fDt);
	}

	// frame skip. For cop think
	if( g_pGameSession->GetLeadCar() &&
		g_pGameSession->GetLeadCar() == g_pGameSession->GetPlayerCar() )
	{
		/*
		if( m_frameSkip )
		{
			float dist = length(g_pGameSession->GetPlayerCar()->GetOrigin() - GetOrigin());
			if(dist > 180.0f)
			{
				// perform lazy collision checks
				//m_refreshTime = AICAR_THINK_TIME*2.0f;
				GetPhysicsBody()->SetMinFrameTime( 1.0f / 30.0f, false );
			}
			else if(dist > 80.0f)
			{
				//m_refreshTime = AICAR_THINK_TIME*1.5f;
				GetPhysicsBody()->SetMinFrameTime( 1.0f / 30.0f );
			}
			else
			{
				GetPhysicsBody()->SetMinFrameTime( 0.0f, false);
			}
		}
		else
			GetPhysicsBody()->SetMinFrameTime( 0.0f, false);
		*/
		if(IsAlive())
			m_refreshTime = AICAR_THINK_TIME;
	}
	else if(!IsAlive())
		m_refreshTime = 0.0f;

	GetPhysicsBody()->UpdateBoundingBoxTransform();

	PROFILE_END();
}

void CAITrafficCar::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(FSMGetCurrentState() == &CAITrafficCar::DeadState)
		return;

	if(pair.impactVelocity > 2.0f && (pair.bodyA->m_flags & BODY_ISCAR))
	{
		// restore collision
		GetPhysicsBody()->SetCollideMask(COLLIDEMASK_VEHICLE);
		GetPhysicsBody()->SetMinFrameTime(0.0f);

		if(IsAlive())
			SignalNoSequence(0.7f, 0.5f);
	}
}

int	CAITrafficCar::SearchForRoad(float fDt, EStateTransition transition)
{
	if(transition != STATE_TRANSITION_NONE)
		return 0;

	Vector3D carPos = GetOrigin();

	// calc steering dir
	straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPos(carPos, 32);

	if(road.direction != -1)
	{
		road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.start);

		ChangeRoad( road );

		AI_SetState( &CAITrafficCar::TrafficDrive );
	}

	SetControlButtons( 0 );
	SetControlVars(1.0f, 1.0f, 1.0f);

	return 0;
}

void CAITrafficCar::ChangeRoad( const straight_t& road )
{
	// cycle roads
	m_straights[STRAIGHT_PREV] = m_straights[STRAIGHT_CURRENT];
	m_straights[STRAIGHT_CURRENT] = road;

	// change conditions
	if (m_straights[STRAIGHT_PREV].direction != m_straights[STRAIGHT_CURRENT].direction)
	{
		m_changingDirection = true;
	}

	m_currEnd = road.end;

	// do it every time now
	m_nextJuncDetails.selectedStraight = -1;
	m_nextJuncDetails.foundStraights.clear();

	//SearchJunctionAndStraight();
}

void CAITrafficCar::SwitchLane()
{
	if(m_straights[STRAIGHT_CURRENT].direction == -1)
		return;

	if (m_changingDirection || m_changingLane)
		return;

	// if we have to turn, don't switch lanes; its illegal :D
	if (m_nextJuncDetails.selectedStraight != -1)
	{
		straight_t& nextRoad = m_nextJuncDetails.foundStraights[m_nextJuncDetails.selectedStraight];

		if(nextRoad.direction != m_straights[STRAIGHT_CURRENT].direction)
			return;
	}

	IVector2D carPosOnCell = g_pGameWorld->m_level.PositionToGlobalTilePoint( GetOrigin() );

	int cellsBeforeEnd = GetCellsBeforeStraightEnd(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

	if (cellsBeforeEnd < 6)
		return;

	int numLanes = g_pGameWorld->m_level.Road_GetNumLanesAtPoint( carPosOnCell );

	if(ai_traffic_debug_steering.GetBool())
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "SwitchLane() lanes on my side: %d\n", numLanes);

	if(numLanes == 1)
		return;

	IVector2D forwardDir = GetDirectionVec( m_straights[STRAIGHT_CURRENT].direction );
	IVector2D rightPerpDir = GetDirectionVec( m_straights[STRAIGHT_CURRENT].direction+1 );

	int randomLane = g_replayRandom.Get(0, 1);

	if(ai_changelane.GetInt() != -1)
	{
		randomLane = ai_changelane.GetInt();
		ai_changelane.SetInt(-1);
	}

	if(randomLane == 0)
		randomLane = -1;

	int newLane = clamp(m_straights[STRAIGHT_CURRENT].lane+randomLane, 0, numLanes-1);

	if( m_straights[STRAIGHT_CURRENT].lane == newLane )
		return;

	int laneOfs = m_straights[STRAIGHT_CURRENT].lane-newLane;

	IVector2D laneTile = carPosOnCell + (forwardDir*4) + rightPerpDir*laneOfs;
	Vector3D lanePos = g_pGameWorld->m_level.GlobalTilePointToPosition(laneTile) + Vector3D(0,0.5f,0);

	if( ai_traffic_debug_steering.GetBool() )
		debugoverlay->Box3D(lanePos - 0.5f, lanePos + 0.5f, ColorRGBA(0,0,1,1), 5.0f);

	straight_t newStraight = g_pGameWorld->m_level.Road_GetStraightAtPoint( laneTile, 16 );

	if( newStraight.direction != -1 &&
		newStraight.direction == m_straights[STRAIGHT_CURRENT].direction)
	{
		IVector2D dir = GetDirectionVec(newStraight.direction);

		Vector3D laneDir(dir.x,0,dir.y);

		Vector3D traceStart = lanePos - laneDir* AI_SIDECHECK_DIST_RR;
		Vector3D traceEnd = lanePos + laneDir* AI_SIDECHECK_DIST_FR;

		// Trace convex car
		CollisionData_t coll;

		eqPhysCollisionFilter collFilter;
		collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
		collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
		collFilter.AddObject( GetPhysicsBody() );

		btConvexShape* shape = (btConvexShape*)GetPhysicsBody()->GetBulletShape();

		bool canSwitchLane = g_pPhysics->TestConvexSweep(shape, GetOrientation(), traceStart, traceEnd, coll, AI_TRACE_CONTENTS, &collFilter) == false;

		SetLight(CAR_LIGHT_EMERGENCY, false);

		if(canSwitchLane)
		{
			SetLight(laneOfs < 0 ? CAR_LIGHT_DIM_LEFT : CAR_LIGHT_DIM_RIGHT, true);

			ChangeRoad( newStraight );
			SearchJunctionAndStraight();

			m_changingLane = true;
		}

		if(ai_traffic_debug_steering.GetBool())
		{
			ColorRGBA col(0,1,1,1);

			//col[m_switchedLane ? 1 : 0] = 1.0f;

			debugoverlay->Box3D(traceStart-0.5f, traceStart+0.5f, col, 5.0f);
			debugoverlay->Box3D(traceEnd-0.5f, traceEnd+0.5f, col, 5.0f);
			debugoverlay->Line3D(traceStart, traceEnd, col, col, 5.0f);
		}
	}
	else
	{
		if(ai_traffic_debug_steering.GetBool())
			debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "SwitchLane(): no parallel lane found\n");
	}
}

void CAITrafficCar::SearchJunctionAndStraight()
{
	memset(m_nextJuncDetails.availDirs, false, sizeof(m_nextJuncDetails.availDirs));

	m_nextJuncDetails.foundStraights.clear(false);
	m_nextJuncDetails.allowedMovement = 0;
	m_nextJuncDetails.selectedStraight = -1;

	if (m_straights[STRAIGHT_CURRENT].direction == -1)
		return;

	roadJunction_t junc = g_pGameWorld->m_level.Road_GetJunctionAtPoint( m_straights[STRAIGHT_CURRENT].end, 16 );

	if( junc.breakIter <= 0)
		return;

	m_nextJuncDetails.junc = junc;

	bool gotSameDirection = false;
	bool sideJuncFound = false;

	// search for roads connected to junctions
	for(int i = junc.startIter; i < junc.breakIter+2; i++)
	{
		IVector2D dir = GetDirectionVecBy( junc.start, junc.end );

		IVector2D checkPos = junc.start+dir*i;

		straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkPos, 24);

		if(	road.direction != -1 &&
			road.breakIter > 1 &&
			!IsOppositeDirectionTo(m_straights[STRAIGHT_CURRENT].direction, road.direction) &&
			(road.end != m_straights[STRAIGHT_CURRENT].end) &&
			(road.start != m_straights[STRAIGHT_CURRENT].start) &&
			!gotSameDirection)
		{
			road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.start);

			m_nextJuncDetails.foundStraights.append( road );
			gotSameDirection = true;
		}

		IVector2D dirCheckVec = GetPerpendicularDirVec(dir);

		// left and right
		for(int j = 0; j < 32; j++)
		{
			int checkDir = j;

			if(j >= 7)
				checkDir = -(checkDir-8);

			IVector2D checkStraightPos = checkPos+dirCheckVec*checkDir;

			levroadcell_t* rcell = g_pGameWorld->m_level.Road_GetGlobalTileAt(checkStraightPos);

			if(rcell && rcell->type == ROADTYPE_NOROAD)
			{
				if(j >= 7)
					break;
				else
					j = 7;

				continue;
			}

			int dirIdx = GetDirectionIndex(dirCheckVec*sign(checkDir));

			// calc steering dir
			straight_t road = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkStraightPos, 16);

			if(	road.direction != -1 &&
				road.direction == dirIdx &&
				(road.end != m_straights[STRAIGHT_CURRENT].end) &&
				(road.start != m_straights[STRAIGHT_CURRENT].start))
			{
				road.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(road.end);

				sideJuncFound = true;
				m_nextJuncDetails.foundStraights.append( road );
				break;
			}
		}

		// correction: move the end of road further
		if(!sideJuncFound)
			m_currEnd = checkPos;
	}

	// get the available directions
	for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
	{
		straight_t& road = m_nextJuncDetails.foundStraights[i];

		m_nextJuncDetails.availDirs[road.direction] = true;
	}

}

int CAITrafficCar::TrafficDrive(float fDt, EStateTransition transition)
{
	if(!m_enabled)
		return 0;

	if(transition != EStateTransition::STATE_TRANSITION_NONE)
		return 0;

	// if this car was flipped, so put it to death state
	// but only if it's not a pursuer type
	if(!IsPursuer())
	{
		if( IsFlippedOver() || !IsAlive() )
		{
			AI_SetState( &CAITrafficCar::DeadState );
			m_hornTime.Set(0);
		}
	}

	//if(g_replayData->m_state == REPL_PLAYING)
	//	return 0;

	// 1. Build a line (A-B) from cell grid to 3D positions
	// 2. Make middle (or near) point (C), lerp(a,b,0.25f)
	//		A---C------------B
	// 3. Drive car to the C point

	m_nextSwitchLaneTime -= fDt;

	Vector3D	carForward		= GetForwardVector();
	Vector3D	carRight		= GetRightVector();
	Vector3D	carPos			= GetOrigin() + carForward*m_conf->physics.body_size.z*0.5f;

	Vector3D	carTracePos		= GetOrigin();

	Vector3D	carBodySide		= GetRightVector()*m_conf->physics.body_size.x*0.75f;

	IVector2D	carPosOnCell	= g_pGameWorld->m_level.PositionToGlobalTilePoint(carPos);

	// previous straight
	Vector3D	prevStartPos	= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_PREV].start );
	Vector3D	prevEndPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_PREV].end );

	// current straight
	Vector3D	startPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].start );
	Vector3D	endPos			= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].end );

	Vector3D	realEndPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_currEnd );

	Vector3D	roadDir			= fastNormalize(startPos-endPos);

	// shows the all route
	if(ai_traffic_debug_route.GetBool())
	{
		// show prev straight
		debugoverlay->Box3D(prevStartPos-0.5f, prevStartPos+0.5f, ColorRGBA(1,0,0,1), AICAR_THINK_TIME);
		debugoverlay->Box3D(prevEndPos-0.5f, prevEndPos+0.5f, ColorRGBA(1,0,0,1), AICAR_THINK_TIME);

		debugoverlay->Line3D(prevStartPos+Vector3D(0,0.25f,0), prevEndPos+Vector3D(0,0.25,0), ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1), AICAR_THINK_TIME);

		// show cur straight
		debugoverlay->Box3D(startPos-0.5f, startPos+0.5f, ColorRGBA(1,1,0,1), AICAR_THINK_TIME);
		debugoverlay->Box3D(endPos-0.5f, endPos+0.5f, ColorRGBA(1,1,0,1), AICAR_THINK_TIME);

		debugoverlay->Line3D(startPos+Vector3D(0,0.25f,0), endPos+Vector3D(0,0.25,0), ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1), AICAR_THINK_TIME);

		// show next straight
		if(m_nextJuncDetails.selectedStraight != -1)
		{
			straight_t& nextRoad = m_nextJuncDetails.foundStraights[m_nextJuncDetails.selectedStraight];

			Vector3D nextStart = g_pGameWorld->m_level.GlobalTilePointToPosition(nextRoad.start);
			Vector3D nextEnd = g_pGameWorld->m_level.GlobalTilePointToPosition(nextRoad.end);

			debugoverlay->Box3D(nextStart-0.5f, nextStart+0.5f, ColorRGBA(0,1,0,1), AICAR_THINK_TIME);
			debugoverlay->Box3D(nextEnd-0.5f, nextEnd+0.5f, ColorRGBA(0,1,0,1), AICAR_THINK_TIME);

			debugoverlay->Line3D(nextStart+Vector3D(0,0.25f,0), nextEnd+Vector3D(0,0.25,0), ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1), AICAR_THINK_TIME);

			// show the transition line
			debugoverlay->Line3D(endPos+Vector3D(0,0.25f,0), nextStart+Vector3D(0,0.25,0), ColorRGBA(1,1,0,1), ColorRGBA(0,1,0,1), AICAR_THINK_TIME);
		}
	}

	if(ai_traffic_debug_junctions.GetBool())
	{
		// draw
		for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
		{
			straight_t& road = m_nextJuncDetails.foundStraights[i];

			Vector3D start = g_pGameWorld->m_level.GlobalTilePointToPosition(road.start);
			Vector3D end = g_pGameWorld->m_level.GlobalTilePointToPosition(road.end);

			debugoverlay->Box3D(start-0.85f, start+0.85f, ColorRGBA(1,1,0,1), AICAR_THINK_TIME);
			debugoverlay->Box3D(end-0.5f, end+0.5f, ColorRGBA(0.5,1,0,1), AICAR_THINK_TIME);

			debugoverlay->Text3D(start, 50.0f, ColorRGBA(1,1,1,1), AICAR_THINK_TIME, "lane %d", road.lane);
		}
	}

	// road end
	Plane		roadEndPlane(roadDir, -dot(roadDir,endPos));
	float		distToStop = roadEndPlane.Distance(carPos);

	Plane		roadRealEndPlane(roadDir, -dot(roadDir,realEndPos));
	float		distToChange = roadRealEndPlane.Distance(carPos);

	float invStraightDist = 1.0f / length(startPos-endPos);
	float projWay = lineProjection(startPos, endPos, carPos);

	float targetOfsOnLine = projWay + AI_TARGET_DIST*invStraightDist;

	if( projWay < 0.0f)
	{
		targetOfsOnLine = projWay + AI_TARGET_DIST_NOLANE*invStraightDist;
	}

	bool isOnPrevRoad = IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_PREV]);
	bool isOnCurrRoad = IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

	bool nextStraightIsSame = m_straights[STRAIGHT_PREV].direction == m_straights[STRAIGHT_CURRENT].direction;

	if( m_changingLane )
		targetOfsOnLine = projWay + AI_TARGET_EXTENDED_DIST*invStraightDist;

	IVector2D dir2D = GetDirectionVec( m_straights[STRAIGHT_CURRENT].direction);
	IVector2D newStraightStartPoint = m_straights[STRAIGHT_CURRENT].end+dir2D;

	// get the new straight
	straight_t newStraight = g_pGameWorld->m_level.Road_GetStraightAtPoint(newStraightStartPoint, 16);
	newStraight.lane = m_straights[STRAIGHT_CURRENT].lane;

	int cellsBeforeEnd = GetCellsBeforeStraightEnd(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

	// continue movement on current straight by extending it
	if (newStraight.direction != -1 &&
		newStraight.direction == m_straights[STRAIGHT_CURRENT].direction && cellsBeforeEnd < 4)
	{
		ChangeRoad(newStraight);
		SearchJunctionAndStraight();
		return 0;
	}

	// try change straight
	if(m_straights[STRAIGHT_CURRENT].direction != -1)
	{
		if(ai_traffic_debug_straigth.GetInt())
		{
			straight_t& road = m_straights[ai_traffic_debug_straigth.GetInt()-1];

			Vector3D start = g_pGameWorld->m_level.GlobalTilePointToPosition(road.start);
			Vector3D end = g_pGameWorld->m_level.GlobalTilePointToPosition(road.end);

			debugoverlay->Box3D(start-0.85f, start+0.85f, ColorRGBA(1,1,0,1), AICAR_THINK_TIME);
			debugoverlay->Box3D(end-0.5f, end+0.5f, ColorRGBA(0.5,1,0,1), AICAR_THINK_TIME);
			debugoverlay->Line3D(start+Vector3D(0,1,0), end+Vector3D(0,1,0), ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1), AICAR_THINK_TIME);
		}

		float preferredChangeDistance = 7.0f;

		if (m_straights[STRAIGHT_CURRENT].lane == 1)
			preferredChangeDistance = 4.0f;

		if(	distToChange < preferredChangeDistance) // && projWay > 1.0f)
		{
			if( newStraight.direction == -1 )
			{
				if(ai_traffic_debug_junctions.GetBool())
				{
					debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "Reached straight end, next straight ID: %d", m_nextJuncDetails.selectedStraight);
				}

				// change state?
				if( m_nextJuncDetails.selectedStraight != -1 )
				{
					ChangeRoad( m_nextJuncDetails.foundStraights[ m_nextJuncDetails.selectedStraight ] );
				}
			}
			else
			{
				if(ai_traffic_debug_junctions.GetBool())
				{
					debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "eached straight end, search for new straight");
				}

				// continue way
				if( m_straights[STRAIGHT_CURRENT].direction == newStraight.direction )
				{
					// only accept in this direction and on same line, else select others
					newStraight.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint( newStraight.end );

					ChangeRoad( newStraight );
					return 0;
				}
				else if( m_nextJuncDetails.selectedStraight != -1)
				{
					ChangeRoad( m_nextJuncDetails.foundStraights[ m_nextJuncDetails.selectedStraight ] );
				}
			}

			return 0;
		}
		else if(distToChange > AI_MIN_LANE_CHANGE_DIST)
		{
			if(	(m_nextSwitchLaneTime < 0 && isOnCurrRoad) || ai_changelane.GetInt() != -1)
			{
				m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;

				SwitchLane();
				return 0;
			}
		}
	}

	if( m_nextJuncDetails.selectedStraight == -1 && m_nextJuncDetails.foundStraights.numElem() > 0 )
	{
		if(ai_traffic_debug_junctions.GetBool())
		{
			debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "--- Has no selected straight ---");
		}
		
		int curRoadWidth = g_pGameWorld->m_level.Road_GetNumLanesAtPoint( m_straights[STRAIGHT_CURRENT].start );

		bool isLongCar = (m_conf->physics.body_size.z > AI_LONG_CAR_CONST);

		DkList<int> allowedStraightIds;

		for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
		{
			straight_t& road = m_nextJuncDetails.foundStraights[i];

			int destRoadWidth = g_pGameWorld->m_level.Road_GetNumLanesAtPoint( road.start );

			int destLane = -1;
			bool result = SolveIntersection(m_straights[STRAIGHT_CURRENT].direction, road.direction, 
											curRoadWidth, m_straights[STRAIGHT_CURRENT].lane, 
											destRoadWidth, destLane,
											isLongCar,
											m_nextJuncDetails.availDirs);

			// check simple laws
			if( result && (destLane == -1 || destLane == road.lane))
				allowedStraightIds.append(i);
		}

		if( allowedStraightIds.numElem() )
		{
			int rndIdx = g_replayRandom.Get(0, allowedStraightIds.numElem() - 1);
			m_nextJuncDetails.selectedStraight = allowedStraightIds[rndIdx];

			if(ai_traffic_debug_junctions.GetBool())
			{
				debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "Selected allowed straight earlier");
			}
		}
		else
		{
			// try select forward
			for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
			{
				straight_t& road = m_nextJuncDetails.foundStraights[i];

				// check simple laws
				if( CompareDirection(m_straights[STRAIGHT_CURRENT].direction, road.direction) )
					allowedStraightIds.append(i);
			}

			if(allowedStraightIds.numElem())
			{
				int rndIdx = g_replayRandom.Get(0, allowedStraightIds.numElem() - 1);
				m_nextJuncDetails.selectedStraight = allowedStraightIds[rndIdx];
			}
			else
			{
				m_nextJuncDetails.selectedStraight = g_replayRandom.Get(0, m_nextJuncDetails.foundStraights.numElem() - 1);
			}
		}
	}

	// steering target
	Vector3D steeringTargetPos = lerp(startPos, endPos, targetOfsOnLine);

	if(ai_traffic_debug_steering.GetBool())
		debugoverlay->Box3D(steeringTargetPos-0.25f,steeringTargetPos+0.25f, ColorRGBA(1,1,0,1), AICAR_THINK_TIME);

	float maxSpeed = g_traffic_maxspeed.GetFloat() * trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType] + m_speedModifier;

	float carForwardSpeed = dot(GetForwardVector(), GetVelocity());

	float carSpeed = fabs(carForwardSpeed)*MPS_TO_KPH;

	float brakeDistancePerSec = m_conf->GetBrakeEffectPerSecond()*0.5f;

	float brakeToStopTime = carForwardSpeed / brakeDistancePerSec*2.0f;
	float brakeDistAtCurSpeed = brakeDistancePerSec*brakeToStopTime;

	float accelerator = ((maxSpeed-carSpeed) / maxSpeed); // go forward
	accelerator = 1.0f - pow(1.0f - accelerator, 4.0f);
	float brake = 0.0f;

	Vector3D steerAbsDir = fastNormalize(steeringTargetPos-carPos);

	Vector3D steerRelatedDir = fastNormalize((!m_worldMatrix.getRotationComponent()) * steerAbsDir);

	float fSteeringAngle = clamp(atan2f(steerRelatedDir.x, steerRelatedDir.z), -1.0f, 1.0f);
	fSteeringAngle = sign(fSteeringAngle) * powf(fabs(fSteeringAngle), 1.25f);

	// modifier by steering
	accelerator *= trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType];
	accelerator -= fabs(fSteeringAngle*0.25f);

	int controls = IN_ACCELERATE | IN_EXTENDTURN;

	if(ai_traffic_debug_straigth.GetBool())
	{
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "isOnCurrRoad: %d\n", isOnCurrRoad);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "isOnPrevRoad: %d\n", isOnPrevRoad);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "changing lane: %d\n", m_changingLane);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "changing direction: %d\n", m_changingDirection);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "nextStraightIsSame: %d\n", nextStraightIsSame);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "num next straights: %d\n", m_nextJuncDetails.foundStraights.numElem());
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "selected next straight: %d\n", m_nextJuncDetails.selectedStraight);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "cellsBeforeEnd: %d\n", cellsBeforeEnd);
	}

	// disable lights turning
	if (m_changingDirection && isOnCurrRoad)
	{
		SearchJunctionAndStraight();

		SetLight(CAR_LIGHT_EMERGENCY, false);
		m_changingDirection = false;
	}

	if (m_changingLane && isOnCurrRoad)
	{
		SearchJunctionAndStraight();

		SetLight(CAR_LIGHT_EMERGENCY, false);
		m_changingLane = false;
	}

	if (m_nextJuncDetails.selectedStraight != -1)
	{
		if (!m_changingLane && !m_changingDirection)
		{
			bool doTurnSignals = cellsBeforeEnd < 8;

			straight_t& selRoad = m_nextJuncDetails.foundStraights[m_nextJuncDetails.selectedStraight];

			bool hasLeftTurn = CompareDirection(m_straights[STRAIGHT_CURRENT].direction - 1, selRoad.direction);

			if (doTurnSignals && hasLeftTurn)
				SetLight(CAR_LIGHT_DIM_LEFT, true);
			else if (doTurnSignals && CompareDirection(m_straights[STRAIGHT_CURRENT].direction + 1, selRoad.direction))
				SetLight(CAR_LIGHT_DIM_RIGHT, true);
		}
	}
	else if (cellsBeforeEnd < 8)
	{
		SearchJunctionAndStraight();
		SetLight(CAR_LIGHT_EMERGENCY, false);
	}

	//
	// Breakage on red light
	//
	if( distToStop < AI_ROAD_STOP_DIST && distToStop > 0.0f )
	{
		// brake on global traffic light value
		int trafficLightDir = g_pGameWorld->m_globalTrafficLightDirection;

		int curDir = m_straights[STRAIGHT_CURRENT].direction;

		levroadcell_t* roadTile = g_pGameWorld->m_level.Road_GetGlobalTileAt(carPosOnCell);

		// check current tile for traffic light indication
		bool hasTrafficLight = roadTile ? (roadTile->flags & ROAD_FLAG_TRAFFICLIGHT) : false;

		bool isAllowedToMove = hasTrafficLight ? (trafficLightDir%2 == curDir%2) : true;

		if( !g_disableTrafficLights.GetBool() && (!isAllowedToMove || (g_pGameWorld->m_globalTrafficLightTime < 2.0f)) )
		{
			float brakeSpeedDiff = brakeDistAtCurSpeed + AI_STOPLINE_DIST - distToStop;
			brakeSpeedDiff = max(brakeSpeedDiff, 0.0f);
			
			brake = brakeSpeedDiff / AI_ROAD_STOP_DIST;
			brake = min(brake, 0.9f);


			if(distToStop > AI_STOPLINE_DIST)
			{
				accelerator -= brake*2.0f;
				controls |= IN_BRAKE;
			}
			else
			{
				controls |= IN_BRAKE;
				controls &= ~IN_ACCELERATE;
			}
		}
		else if (m_nextJuncDetails.selectedStraight != -1)
		{
			//
			// Breakage before junction when changing direction
			//

			straight_t& selRoad = m_nextJuncDetails.foundStraights[m_nextJuncDetails.selectedStraight];

			if( m_straights[STRAIGHT_CURRENT].direction != selRoad.direction)
			{
				if( carForwardSpeed > 8.0f*trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType] )
				{
					// FIXME: calculate acceleration more gentle

					float distToNextStraight = distToStop;

					float fBrakeRate = 1.0f - RemapValClamp(distToNextStraight, 0.0f, AI_ROAD_STOP_DIST, 0.0f, 1.0f);

					brake = clamp(fBrakeRate + (carForwardSpeed-distToStop)*0.25f, 0.0f, 0.5f);

					accelerator -= brake*1.1f;

					if(brake >= 0.5f)
						controls |= IN_BRAKE;
				}
			}
		}
	}

	// if we're on intersection
	if (m_changingDirection && !isOnCurrRoad)
	{
		DkList<CCar*> forwardCars;
		g_pAIManager->QueryTrafficCars(forwardCars, 16.0f, carPos, carForward, 0.8f);

		CCar* firstOppositeMovingCar = nullptr;

		for (int i = 0; i < forwardCars.numElem(); i++)
		{
			if (forwardCars[i] == this || forwardCars[i]->GetSpeed() < 5.0f)
				continue;

			const Vector3D& velocity = forwardCars[i]->GetVelocity();

			if (dot(fastNormalize(velocity), -carForward) > 0.4f)
			{
				firstOppositeMovingCar = forwardCars[i];
				break;
			}
		}

		if (firstOppositeMovingCar)
		{
			brake = 1.0f;
			accelerator -= brake * 2.0f;
			controls |= IN_BRAKE;
		}
	}

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(GetPhysicsBody());

	//
	// Breakage when has object in front
	//
	{
		float fTraceDist = AI_CAR_TRACE_DIST_MAX * trafficBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType];

		CollisionData_t frontObjColl;

		// FIXME: detect steering wheels?
		Matrix3x3 wrotation = m_wheels[0].m_wheelOrient*transpose(m_worldMatrix).getRotationComponent();

		Vector3D traceDir = lerp(GetForwardVector(), steerAbsDir, pow(fabs(fSteeringAngle), 0.5f));

		Vector3D carTraceEndPos = carTracePos+traceDir*fTraceDist;
		//Vector3D navTraceEndPos = carTracePos+traceDir*(carSpeed*0.1f+4.0f);

		//debugoverlay->Line3D(carTracePos, carTraceEndPos, ColorRGBA(1,0,0,1), ColorRGBA(0,0,1,1), 0.05f);

		btConvexShape* carShape = (btConvexShape*)GetPhysicsBody()->GetBulletShape();
		btBoxShape carBoxShape(btVector3(m_conf->physics.body_size.x, m_conf->physics.body_size.y*2, 0.25f));

		//float navTestLine = g_pGameWorld->m_level.Nav_TestLine(carTracePos, navTraceEndPos, true);

		if( g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(), carTracePos, carTraceEndPos, frontObjColl, AI_TRACE_CONTENTS, &collFilter))
		{
			float frontFract = frontObjColl.fract;//min(navTestLine, frontObjColl.fract);

			float lineDist = frontFract*fTraceDist;
			float diffForwardSpeed = carForwardSpeed;

			CEqCollisionObject* hitObj = frontObjColl.hitobject;

			//FReal frontCarBrake = 0.0f;

			if(hitObj)
			{
				if( (hitObj->m_flags & BODY_ISCAR) )
				{
					CCar* pCar = (CCar*)hitObj->GetUserData();

					// add the velocity difference
					float frontCarSpeed = dot(pCar->GetVelocity(), pCar->GetForwardVector());

					if(frontCarSpeed > 8.0f)
						diffForwardSpeed -= frontCarSpeed*0.5f;

					if(m_prevFract == 0.0f)
						m_prevFract = 0.05f;

					float fromPrevPercentage = frontFract / m_prevFract;

					if( lineDist < AI_CAR_TRACE_DIST_MIN || (1.0f-fromPrevPercentage) >= 0.45f )
					{
						// if vehicle direction and speed differs
						float velDiff = dot((GetVelocity() + GetForwardVector()) - pCar->GetVelocity(), GetForwardVector());

						if( (velDiff > 15.0f || (1.0f-fromPrevPercentage) >= 0.45f) && carForwardSpeed > 0.25f )
							SignalRandomSequence(0.3f); //m_hornTime.SetIfNot(1.0f, 0.3f);
					}
				}
			}

			if (lineDist < AI_ROAD_STOP_DIST)
			{
				float dbrakeToStopTime = diffForwardSpeed / brakeDistancePerSec * 2.0f;
				float dbrakeDistAtCurSpeed = brakeDistancePerSec * dbrakeToStopTime;

				float brakeSpeedDiff = dbrakeDistAtCurSpeed + AI_OBSTACLE_DIST - lineDist;
				brakeSpeedDiff = max(brakeSpeedDiff, 0.0f);

				brake = brakeSpeedDiff / 20.0f;

				if (lineDist > AI_OBSTACLE_DIST)
				{
					accelerator -= brake * 2.0f;
					controls |= IN_BRAKE;
				}
				else
				{
					controls |= IN_BRAKE;
					controls &= ~IN_ACCELERATE;
				}
			}
			else
			{
				accelerator *= (frontFract-0.5f) * 2.0f;
			}

			m_prevFract = frontFract;
		}

		//
		// EMERGENCY BRAKE AND STEERING
		//
		if(carForwardSpeed > 5.0f)
		{
			fTraceDist = RemapValClamp(carForwardSpeed, 0.0f, 40.0f, AI_CAR_TRACE_DIST_MIN, AI_CAR_TRACE_DIST_MAX)*3.5f;

			m_autohandbrake = false;

			carTraceEndPos = carTracePos+GetForwardVector()*fTraceDist;

			if( g_pPhysics->TestConvexSweep(carShape, GetOrientation(), carTracePos, carTraceEndPos, frontObjColl, AI_TRACE_CONTENTS, &collFilter) )
			{
				float emergencyFract = frontObjColl.fract;

				//
				// Emergency situation
				//
				if( emergencyFract < 1.0f)
				{
					CEqCollisionObject* hitObj = frontObjColl.hitobject;//(coll_lineL.fract < 1.0f) ? coll_lineL.hitobject : ((coll_lineR.fract < 1.0f) ? coll_lineR.hitobject : NULL);

					if( hitObj->m_flags & BODY_ISCAR )
					{
						CCar* pCar = (CCar*)hitObj->GetUserData();

						// if vehicle direction and speed differs
						float velDiff = dot((GetVelocity() + GetForwardVector()) - pCar->GetVelocity(), GetForwardVector());

						if( velDiff > 30.0f)
						{
							SignalNoSequence(1.0f, 0.2f);

							if(!m_emergencyEscape && emergencyFract < 0.5f)
							{
								m_emergencyEscape = true;

								m_emergencyEscapeTime = AI_EMERGENCY_ESCAPE_TIME;

								m_autohandbrake = true;
							}

							Plane dPlane(carRight, -dot(carPos, carRight));
							float posSteerFactor = dPlane.Distance(pCar->GetOrigin() + pCar->GetVelocity()*0.25f);

							m_emergencyEscapeSteer = sign(posSteerFactor) * -1.0f;
						}
					}

				} // emergencyFract < 1.0f

			} // TestConvexSweep

		} // if(carForwardSpeed > 5.0f)
	}

	// control method for emergency escape
	if( m_emergencyEscape && m_emergencyEscapeTime > 0.0f )
	{
		m_emergencyEscapeTime -= fDt;

		fSteeringAngle = m_emergencyEscapeSteer;
		controls |= IN_TURNRIGHT;
	}
	else
	{
		m_emergencyEscape = false;
		controls |= IN_ANALOGSTEER;
	}

	if( accelerator > 0.05f )
		GetPhysicsBody()->Wake();

	SetControlButtons( controls );

	float speedClampFac = 20.0f-carSpeed;
	accelerator *= RemapValClamp(speedClampFac, 0.0f, 20.0f, 0.6f, 1.0f);

	SetControlVars(accelerator, brake, clamp(fSteeringAngle, -0.95f, 0.95f));

	return 0;
}

void CAITrafficCar::SignalRandomSequence( float delayBeforeStart )
{
	if(m_hornTime.IsOn())
		return;

	int seqId = RandomInt(0,SIGNAL_SEQUENCE_COUNT-1);

	m_signalSeq = &g_signalSequences[seqId];
	m_signalSeqFrame = 0;

	m_hornTime.SetIfNot(m_signalSeq->length, delayBeforeStart);
}

void CAITrafficCar::SignalNoSequence( float time, float delayBeforeStart )
{
	m_signalSeq = NULL;

	m_hornTime.SetIfNot(time, delayBeforeStart);
}

OOLUA_EXPORT_FUNCTIONS(
	CAITrafficCar,
	InitAI
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CAITrafficCar,
	IsPursuer
)
