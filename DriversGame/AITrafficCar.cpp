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
/*
carla brake ineffectiveness per mass: 0.2 (14000 x 2800kg), effectiveness = 5
bus brake ineffectiveness per mass: 0.5 (15500 x 9000kg), effectiveness = 1.72

so brake distance scale is:
	brakeDistScale = bodyMass / brakeTorque

	???
*/

ConVar g_traffic_maxspeed("g_trafficMaxspeed", "36");
ConVar g_disableTrafficLights("g_disableTrafficLights", "0", NULL, CV_CHEAT);

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

const float AICAR_THINK_TIME = 0.15f;

const float AI_SIDECHECK_DIST = 4.0f;

const float AI_ROAD_STOP_DIST = 18.0f;

const float AI_CAR_TRACE_DIST_MIN = 6.0f;
const float AI_CAR_TRACE_DIST_MAX = 25.0f;

const float AI_ROAD_TRACE_DIST = 30.0f;
const float AI_MIN_LANE_CHANGE_DIST = 40.0f;

const float AI_TARGET_DIST_NOLANE = 5.0f;
const float AI_TARGET_DIST = 10.0f;
const float AI_TARGET_EXTENDED_DIST = 30.0f;

const float AI_LANE_SWITCH_DELAY = 12.0f;

const float AI_EMERGENCY_ESCAPE_TIME = 1.3f;

const float KILL_COLLISION_POWER = 18.0f;

#define STRAIGHT_CURRENT	0
#define STRAIGHT_PREV		1

//-------------------------------------------------------------------------------

bool IsStraightsOnSameLane( const straight_t& a, const straight_t& b )
{
	int dira = a.direction;
	int dirb = b.direction;

	if(dira > 2)
		dira -= 2;

	if(dirb > 2)
		dirb -= 2;

	return a.start[dira] == b.start[dirb];
}

IVector2D GetDirectionVecBy(const IVector2D& posA, const IVector2D& posB)
{
	return clamp(posB-posA, IVector2D(-1,-1),IVector2D(1,1));
}

IVector2D GetDirectionVec(int dirIdx)
{
	if(dirIdx > 3)
		dirIdx -= 4;

	int rX[4] = ROADNEIGHBOUR_OFFS_X(0);
	int rY[4] = ROADNEIGHBOUR_OFFS_Y(0);

	return IVector2D(rX[dirIdx], rY[dirIdx]);
}

bool IsPointOnStraight(const IVector2D& pos, const straight_t& straight)
{
	// make them floats
	float posOnLine = lineProjection((Vector2D)straight.start, (Vector2D)straight.end, (Vector2D)pos);

	IVector2D perpDirVec = GetDirectionVec( straight.direction+1 );

	IVector2D posComp = pos * perpDirVec;
	IVector2D strComp = straight.start * perpDirVec;

	if( posComp == strComp )
		return (posOnLine >= 0.0f && posOnLine <= 1.0f);

	return false;
}

int GetCellsBeforeStraight(const IVector2D& pos, const straight_t& straight)
{
	if(straight.direction == -1)
		return 1000;

	int dirV = 1 - (straight.direction % 2);

	int cmpA = min(pos[dirV], straight.start[dirV]);
	int cmpB = max(pos[dirV], straight.start[dirV]);

	return cmpB-cmpA;
}

int GetDirectionIndex(const IVector2D& vec)
{
	int rX[4] = ROADNEIGHBOUR_OFFS_X(0);
	int rY[4] = ROADNEIGHBOUR_OFFS_Y(0);

	IVector2D v = clamp(vec, IVector2D(-1,-1),IVector2D(1,1));

	for(int i = 0; i < 4; i++)
	{
		IVector2D r(rX[i], rY[i]);

		if(r == v)
			return i;
	}

	return 0;
}

bool CompareDirection(int dirA, int dirB)
{
	int dA = dirA;
	if(dA < 0)
		dA += 4;

	if(dA > 3)
		dA -= 4;

	int dB = dirB;
	if(dB < 0)
		dB += 4;

	if(dB > 3)
		dB -= 4;

	return dA == dB;
}

// remember: first lane is rightmost!
bool SolveLaneTrafficLaws(int curDirection, int destDirection, int curRoadWidthLanes, int curLane, int destRoadWidth, int& destLane)
{
	// P.S. Только водители из Астаны меняют направление движения на противоположное :-)
	if( IsOppositeDirectionTo(curDirection, destDirection) )
		return false;

	// if we have to turn other direction, check

	if( curLane == curRoadWidthLanes )	// last left lane
	{
		// don't allow turn right from this lane
		if(CompareDirection(curDirection, destDirection-1))
			return false;

		// only allowed to turn left, or go forward
		if(	(curRoadWidthLanes == 2 && curDirection==destDirection) || CompareDirection(curDirection, destDirection+1) )
		{
			destLane = destRoadWidth; // move to last left lanes
			return true;
		}
	}
	else if( curLane == 1 ) // first (right) lane
	{
		// only allowed to turn right
		if(curRoadWidthLanes > 1 && CompareDirection(curDirection, destDirection-1))
		{
			if(destRoadWidth >= 2)
				destLane = 2;
			else
				destLane = 1;

			return true;
		}
		else
			return false;
	}
	else
	{
		// go forward pls
		if(!CompareDirection(curDirection, destDirection))
			return false;
	}

	destLane = curLane;

	if(destLane > destRoadWidth)
		destLane = destRoadWidth;

	return true;
}

IVector2D GetPerpendicularDirVec(const IVector2D& vec)
{
	return IVector2D(vec.y,vec.x);
}

//----------------------------------------------------------------------------------------------------------------

CAITrafficCar::CAITrafficCar( carConfigEntry_t* carConfig ) : CCar(carConfig), CFSM_Base()
{
	m_frameSkip = false;
	m_switchedLane = false;
	m_autohandbrake = false;

	m_thinkTime = 0.0f;
	m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;
	m_refreshTime = AICAR_THINK_TIME;
	m_laneSwitchTimeout = 0.0f;
	m_emergencyEscape = false;
	m_emergencyEscapeTime = 0.0f;
	m_emergencyEscapeSteer = 1.0f;
	m_prevFract = 1.0f;

	m_signalSeq = NULL;
	m_signalSeqFrame = 0;
}

CAITrafficCar::~CAITrafficCar()
{
	
}

void CAITrafficCar::InitAI( bool isParked )
{
	AI_SetState( &CAITrafficCar::SearchForRoad );
	m_speedModifier = g_replayRandom.Get(0,10);
}

void CAITrafficCar::Spawn()
{
	BaseClass::Spawn();

	// try this, after first collision you must change it
	GetPhysicsBody()->SetCollideMask( COLLIDEMASK_VEHICLE );//OBJECTCONTENTS_DEBRIS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE );

	// perform lazy collision checks
	GetPhysicsBody()->SetMinFrameTime(1.0f / 30.0f);

	if( m_conf->m_colors.numElem() > 0 )
	{
		int col_idx = g_replayRandom.Get(0, m_conf->m_colors.numElem() - 1);
		SetColorScheme(col_idx);
	}
}

int CAITrafficCar::DeadState( float fDt, EStateTransition transition )
{
	SignalNoSequence(0,0);
	return 0;
}

ConVar g_disableTrafficCarThink("g_disableTrafficCarThink", "0", "Disables traffic car thinking", CV_CHEAT);

void CAITrafficCar::OnPrePhysicsFrame( float fDt )
{
	BaseClass::OnPrePhysicsFrame(fDt);

	PROFILE_BEGIN(AITrafficCar_Think);

	if( !g_disableTrafficCarThink.GetBool())
	{
		m_thinkTime -= fDt;

		if(m_thinkTime <= 0.0f)
		{
			int res = DoStatesAndEvents( m_refreshTime+fDt );
			m_thinkTime = m_refreshTime;
		}

		if(m_straights[STRAIGHT_CURRENT].direction != -1)
		{
			//Vector3D start = g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].start );
			//Vector3D target = g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].end );

			//debugoverlay->Box3D(start-1.5f, start+1.5f, ColorRGBA(1,0,1,0.25f));
			//debugoverlay->Box3D(target-1.5f, target+1.5f, ColorRGBA(0,1,1,0.25f));
		}
		else if(FSMGetCurrentState() == &CAITrafficCar::TrafficDrive)
		{
			AI_SetState( &CAITrafficCar::SearchForRoad );
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

	if( g_pGameSession->GetLeadCar() && 
		g_pGameSession->GetLeadCar() == g_pGameSession->GetPlayerCar() )
	{
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

		SignalNoSequence(0.7f, 0.5f);

		CEqRigidBody* body = (CEqRigidBody*)pair.bodyA;

		float impulse = pair.appliedImpulse*body->GetInvMass();

		if(impulse > KILL_COLLISION_POWER)
		{
			m_hornTime.Set(0);

			AI_SetState( &CAITrafficCar::DeadState );

			int buttons = GetControlButtons();
			buttons &= ~(IN_HORN | IN_ACCELERATE | IN_BRAKE);

			SetControlButtons(buttons);
		}
	}
}

int	CAITrafficCar::SearchForRoad(float fDt, EStateTransition transition)
{
	if(transition != STATE_TRANSITION_NONE)
		return 0;

	Matrix4x4 carMatrix;
	GetPhysicsBody()->ConstructRenderMatrix(carMatrix);

	Vector3D carPos = GetOrigin();

	// calc steering dir
	straight_t road = g_pGameWorld->m_level.GetStraightAtPos(carPos, 32);

	if( m_straights[STRAIGHT_CURRENT].direction == -1 )
	{
		if(road.direction != -1)
		{
			road.lane = g_pGameWorld->m_level.GetLaneIndexAtPoint(road.start);

			ChangeRoad( road );

			AI_SetState( &CAITrafficCar::TrafficDrive );
		}
	}

	SetControlButtons( 0 );
	SetControlVars(1.0f, 1.0f, 1.0f);

	return 0;
}

void CAITrafficCar::ChangeRoad( const straight_t& road )
{
	m_straights[STRAIGHT_PREV] = m_straights[STRAIGHT_CURRENT];
	m_straights[STRAIGHT_CURRENT] = road;

	m_currEnd = road.end;

	// do it every time now
	SearchJunctionAndStraight();
}

void CAITrafficCar::SwitchLane()
{
	if(m_straights[STRAIGHT_CURRENT].direction == -1)
		return;

	int numLanes = g_pGameWorld->m_level.GetNumLanesAtPoint( m_straights[STRAIGHT_CURRENT].end );

	if(numLanes == 1)
		return;

	IVector2D rightPerpDir = GetDirectionVec( m_straights[STRAIGHT_CURRENT].direction+1 );

	int randomLane = g_replayRandom.Get(0, 1);

	if(randomLane == 0)
		randomLane = -1;

	int newLane = clamp(m_straights[STRAIGHT_CURRENT].lane+randomLane, 0, numLanes-1);

	if( m_straights[STRAIGHT_CURRENT].lane == newLane )
		return;

	int laneOfs = newLane-m_straights[STRAIGHT_CURRENT].lane;

	IVector2D laneTile = m_straights[STRAIGHT_CURRENT].start - rightPerpDir*laneOfs;

	straight_t newStraight = g_pGameWorld->m_level.GetStraightAtPoint( laneTile, 16 );

	if( newStraight.direction != -1 &&
		newStraight.direction == m_straights[STRAIGHT_CURRENT].direction)
	{
		IVector2D dir = GetDirectionVec(newStraight.direction);

		Vector3D laneDir(dir.x,0,dir.y);

		Vector3D lanePos = g_pGameWorld->m_level.GlobalTilePointToPosition(laneTile);

		Vector3D traceStart = lanePos - laneDir*AI_SIDECHECK_DIST;
		Vector3D traceEnd = lanePos + laneDir*AI_SIDECHECK_DIST;

		// Trace convex car
		CollisionData_t coll;

		eqPhysCollisionFilter collFilter;
		collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
		collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
		collFilter.AddObject( GetPhysicsBody() );

		btConvexShape* shape = (btConvexShape*)GetPhysicsBody()->GetBulletShape();

		// if has any collision then deny
		if( g_pPhysics->TestConvexSweep(shape, GetOrientation(), traceStart, traceEnd, coll, AI_TRACE_CONTENTS, &collFilter) )
			return;

		if( g_pPhysics->TestConvexSweep(shape, GetOrientation(), GetOrigin(), traceEnd, coll, AI_TRACE_CONTENTS, &collFilter) )
			return;

		if( randomLane < 0 )
			m_lightsEnabled |= CAR_LIGHT_DIM_LEFT;
		else if( randomLane > 0 )
			m_lightsEnabled |= CAR_LIGHT_DIM_RIGHT;

		m_switchedLane = true;

		ChangeRoad( newStraight );
	}
}

void CAITrafficCar::SearchJunctionAndStraight()
{
	m_nextJuncDetails.foundStraights.clear(false);
	m_nextJuncDetails.allowedMovement = 0;
	m_nextJuncDetails.selectedStraight = -1;

	if( m_straights[STRAIGHT_CURRENT].direction == -1 )
	{
		//Msg("Current straight has no direction, exiting\n");
		return;
	}

	roadJunction_t junc = g_pGameWorld->m_level.GetJunctionAtPoint( m_straights[STRAIGHT_CURRENT].end, 16 );

	if( junc.breakIter <= 0)
	{
		//Msg("Junction was not found\n");
		return;
	}

	m_nextJuncDetails.junc = junc;

	bool gotSameDirection = false;
	bool sideJuncFound = false;

	// search for roads connected to junctions
	for(int i = junc.startIter; i < junc.breakIter+2; i++)
	{
		IVector2D dir = GetDirectionVecBy( junc.start, junc.end );

		IVector2D checkPos = junc.start+dir*i;

		/*
		{
			Vector3D checkPos3D = g_pGameWorld->m_level.GlobalTilePointToPosition(checkPos);
			debugoverlay->Box3D(checkPos3D-0.8f, checkPos3D+0.8f, ColorRGBA(1,1,0,1), 10.25f);
		}
		*/

		straight_t road = g_pGameWorld->m_level.GetStraightAtPoint(checkPos, 16);

		if(	road.direction != -1 &&
			road.breakIter > 1 && 
			!IsOppositeDirectionTo(m_straights[STRAIGHT_CURRENT].direction, road.direction) &&
			(road.end != m_straights[STRAIGHT_CURRENT].end) &&
			(road.start != m_straights[STRAIGHT_CURRENT].start) &&
			!gotSameDirection)
		{
			road.lane = g_pGameWorld->m_level.GetLaneIndexAtPoint(road.start);
	
			m_nextJuncDetails.foundStraights.append( road );
			gotSameDirection = true;
		}

		IVector2D dirCheckVec = GetPerpendicularDirVec(dir);

		// left and right
		for(int j = 0; j < 16; j++)
		{
			int checkDir = j;

			if(j >= 7)
				checkDir = -(checkDir-8);

			IVector2D checkStraightPos = checkPos+dirCheckVec*checkDir;

			int dirIdx = GetDirectionIndex(dirCheckVec*sign(checkDir));

			// calc steering dir
			straight_t road = g_pGameWorld->m_level.GetStraightAtPoint(checkStraightPos, 16);

			

			if(	road.direction != -1 && 
				road.direction == dirIdx &&
				(road.end != m_straights[STRAIGHT_CURRENT].end) && 
				(road.start != m_straights[STRAIGHT_CURRENT].start))
			{
				road.lane = g_pGameWorld->m_level.GetLaneIndexAtPoint(road.end);

				sideJuncFound = true;
				m_nextJuncDetails.foundStraights.append( road );
				break;
			}
		}

		// correction: move the end of road further
		if(!sideJuncFound)
			m_currEnd = checkPos;
	}
}


int CAITrafficCar::TrafficDrive(float fDt, EStateTransition transition)
{
	if(!m_enabled)
		return 0;

	// 1. Build a line (A-B) from cell grid to 3D positions
	// 2. Make middle (or near) point (C), lerp(a,b,0.25f)
	//		A---C------------B
	// 3. Drive car to the C point

	m_nextSwitchLaneTime -= fDt;

	Matrix4x4 bodyMat;
	GetPhysicsBody()->ConstructRenderMatrix( bodyMat );

	Vector3D	carPos			= GetOrigin() + GetForwardVector()*m_conf->m_body_size.z*0.5f;

	Vector3D	carTracePos		= GetOrigin();

	Vector3D	carBodySide		= GetRightVector()*m_conf->m_body_size.x*0.75f;

	IVector2D	carPosOnCell	= g_pGameWorld->m_level.PositionToGlobalTilePoint(carPos);

	// previous straight
	Vector3D	prevStartPos	= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_PREV].start );
	Vector3D	prevEndPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_PREV].end );

	// current straight
	Vector3D	startPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].start );

	IVector2D	straightDir		= GetDirectionVec(m_straights[STRAIGHT_CURRENT].direction);
	IVector2D	straightEnd		= m_straights[STRAIGHT_CURRENT].start + straightDir*m_straights[STRAIGHT_CURRENT].dirChangeIter;

	Vector3D	straightEndPos	= g_pGameWorld->m_level.GlobalTilePointToPosition(straightEnd);
	Vector3D	endPos			= g_pGameWorld->m_level.GlobalTilePointToPosition( m_straights[STRAIGHT_CURRENT].end );
	
	Vector3D	realEndPos		= g_pGameWorld->m_level.GlobalTilePointToPosition( m_currEnd );

	Vector3D	roadDir = fastNormalize(startPos-endPos);

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
	
	if( m_switchedLane )
	{
		bool isOnPrevRoad = IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_PREV]);

		if( m_laneSwitchTimeout > 0 || isOnPrevRoad )
		{
			targetOfsOnLine = projWay + AI_TARGET_EXTENDED_DIST*invStraightDist;

			if( isOnPrevRoad )
				m_laneSwitchTimeout = 5.0f;
			else
				m_laneSwitchTimeout -= fDt;
		}
		else
		{
			m_switchedLane = false;
			m_lightsEnabled &= ~CAR_LIGHT_EMERGENCY;
		}
	}

	IVector2D dir2D = GetDirectionVec( m_straights[STRAIGHT_CURRENT].direction);
	straight_t newStraight = g_pGameWorld->m_level.GetStraightAtPoint( m_straights[STRAIGHT_CURRENT].end + dir2D, 16 );

	if( newStraight.direction != -1 &&
		newStraight.direction == m_straights[STRAIGHT_CURRENT].direction &&
		GetCellsBeforeStraight(carPosOnCell, newStraight) < 16 )
	{
		newStraight.lane = m_straights[STRAIGHT_CURRENT].lane;
		m_straights[STRAIGHT_CURRENT] = newStraight;

		SearchJunctionAndStraight();
		return 0;
	}

	// try change straight
	if(m_straights[STRAIGHT_CURRENT].direction != -1)
	{
		if(	distToChange < 0.0f ) // && projWay > 1.0f)
		{
			if( newStraight.direction == -1 )
			{
				//debugoverlay->Text3D(carPos, ColorRGBA(1,1,1,1), "Reached straight end, next straight ID: %d\n", m_nextJuncDetails.selectedStraight);

				// change state?
				if( m_nextJuncDetails.selectedStraight != -1 )
					ChangeRoad( m_nextJuncDetails.foundStraights[ m_nextJuncDetails.selectedStraight ] );
			}
			else
			{
				//debugoverlay->Text3D(carPos, ColorRGBA(1,1,1,1), "Search for new straight\n");

				// continue way
				if( m_straights[STRAIGHT_CURRENT].direction == newStraight.direction )
				{
					// only accept in this direction and on same line, else select others
					newStraight.lane = g_pGameWorld->m_level.GetLaneIndexAtPoint( newStraight.end );

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
			if(	m_nextSwitchLaneTime < 0)
			{
				m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;

				if(IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_CURRENT]))
				{
					SwitchLane();
					return 0;
				}
			}
		}
	}

	/*
	// draw
	for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
	{
		straight_t& road = m_nextJuncDetails.foundStraights[i];

		Vector3D start = g_pGameWorld->m_level.GlobalTilePointToPosition(road.start);
		Vector3D end = g_pGameWorld->m_level.GlobalTilePointToPosition(road.end);

		debugoverlay->Box3D(start-0.85f, start+0.85f, ColorRGBA(1,1,0,1));
		debugoverlay->Box3D(end-0.5f, end+0.5f, ColorRGBA(0.5,1,0,1));
	}*/

	if( m_nextJuncDetails.selectedStraight == -1 &&
		m_nextJuncDetails.foundStraights.numElem() > 0 )
	{
		DkList<int> allowedStraightIds;
		int curNumLanes = g_pGameWorld->m_level.GetNumLanesAtPoint( m_straights[STRAIGHT_CURRENT].start );

		for(int i = 0; i < m_nextJuncDetails.foundStraights.numElem(); i++)
		{
			straight_t& road = m_nextJuncDetails.foundStraights[i];

			int destRoadWidth = g_pGameWorld->m_level.GetNumLanesAtPoint( road.start );

			int destLane = -1;
			bool result = SolveLaneTrafficLaws(m_straights[STRAIGHT_CURRENT].direction, road.direction, curNumLanes, m_straights[STRAIGHT_CURRENT].lane, destRoadWidth, destLane);

			// check simple laws
			if( result && destLane == road.lane)
				allowedStraightIds.append(i);
		}

		if( allowedStraightIds.numElem() )
		{
			int rndIdx = g_replayRandom.Get(0, allowedStraightIds.numElem() - 1);
			m_nextJuncDetails.selectedStraight = allowedStraightIds[rndIdx];
			//Msg("Selected allowed straight earlier\n");
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
	//debugoverlay->Box3D(steeringTargetPos-1,steeringTargetPos+1, ColorRGBA(1,1,0,1));

	float maxSpeed = g_traffic_maxspeed.GetFloat() * trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType] + m_speedModifier;

	float carForwardSpeed = dot(GetForwardVector(), GetVelocity());

	float carSpeed = fabs(carForwardSpeed)*SPEEDO_SCALE;

	float accelerator = 1.0f - pow(1.0f - ((maxSpeed - carSpeed) / maxSpeed), 5.5f);	// go forward
	float brake = 0.0f;

	Vector3D steerDir = fastNormalize((!bodyMat.getRotationComponent()) * fastNormalize(steeringTargetPos-carPos));
	float fSteeringAngle = clamp(atan2(steerDir.x, steerDir.z), -1.0f, 1.0f);

	// modifier by steering
	accelerator *= clamp(0.45f+(1.0f-fabs(fSteeringAngle))*0.55f, 0.0f, 1.0f) * trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType];

	int controls = IN_ACCELERATE | IN_EXTENDTURN;

	//
	// Breakage on red light
	//
	if( distToStop < AI_ROAD_STOP_DIST*trafficBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType] )
	{
		// brake on global traffic light value
		int trafficLightDir = g_pGameWorld->m_globalTrafficLightDirection;

		int curDir = m_straights[STRAIGHT_CURRENT].direction % 2;

		if( !g_disableTrafficLights.GetBool() && ((trafficLightDir == curDir) || (g_pGameWorld->m_globalTrafficLightTime < 2.0f)) )
		{
			if(carForwardSpeed > 0.5f)
			{
				float fBrakeRate = 1.0f - RemapValClamp(distToStop, 0.0f, AI_ROAD_STOP_DIST, 0.0f, 1.0f);

				brake = clamp(fBrakeRate + (carForwardSpeed-distToStop)*0.25f, 0.0f, 0.9f);

				accelerator -= brake*1.1f;

				controls |= IN_BRAKE;
			}
			else
				controls |= IN_HANDBRAKE;

			controls &= ~IN_ACCELERATE;
		}
		else
		{
			//
			// Breakage before junction when changing direction
			//

			if(m_nextJuncDetails.selectedStraight != -1)
			{
				straight_t& selRoad = m_nextJuncDetails.foundStraights[m_nextJuncDetails.selectedStraight];
				int currentDir = m_straights[STRAIGHT_CURRENT].direction;

				m_lightsEnabled &= ~CAR_LIGHT_EMERGENCY;

				if( CompareDirection(m_straights[STRAIGHT_CURRENT].direction-1, selRoad.direction) )
					m_lightsEnabled |= CAR_LIGHT_DIM_LEFT;
				else if( CompareDirection(m_straights[STRAIGHT_CURRENT].direction+1, selRoad.direction) )
					m_lightsEnabled |= CAR_LIGHT_DIM_RIGHT;
				else
					m_lightsEnabled &= ~CAR_LIGHT_EMERGENCY;

				if( carForwardSpeed > 8.0f*trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType] )
				{
					if( m_straights[STRAIGHT_CURRENT].direction != selRoad.direction )
					{
						float distToNextStraight = distToStop;

						float fBrakeRate = 1.0f - RemapValClamp(distToNextStraight, 0.0f, AI_ROAD_STOP_DIST, 0.0f, 1.0f);

						brake = clamp(fBrakeRate + (carForwardSpeed-distToStop)*0.25f, 0.0f, 0.5f);

						accelerator -= brake*1.1f;

						if(brake >= 0.5f)
							controls |= IN_BRAKE;
					}
				}
				else if(carForwardSpeed > 3.0f*trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType])
				{
					accelerator *= 0.25f;
				}
			}
		}
	}

	controls |= IN_ANALOGSTEER;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject( GetPhysicsBody() );

	//
	// Breakage when has object in front
	//
	{
		float fTraceDist = AI_CAR_TRACE_DIST_MAX * trafficBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType]; //RemapValClamp(carForwardSpeed, 0.0f, 40.0f, AI_CAR_TRACE_DIST_MIN, AI_CAR_TRACE_DIST_MAX);

		CollisionData_t frontObjColl;

		// FIXME: detect steering wheels?
		Matrix3x3 wrotation = m_pWheels[0].wheelOrient*transpose(bodyMat).getRotationComponent();

		Vector3D carTraceEndPos = carTracePos+GetForwardVector()*fTraceDist;

		btConvexShape* carShape = (btConvexShape*)GetPhysicsBody()->GetBulletShape();
		btBoxShape carBoxShape(btVector3(m_conf->m_body_size.x-0.15f, m_conf->m_body_size.y, 0.25f));

		if( g_pPhysics->TestConvexSweep(&carBoxShape, GetOrientation(), carTracePos, carTraceEndPos, frontObjColl, AI_TRACE_CONTENTS, &collFilter) )
		{
			float frontFract = frontObjColl.fract;

			float lineDist = frontFract*fTraceDist;
			float speedDiff = carForwardSpeed - lineDist;
				
			CEqCollisionObject* hitObj = frontObjColl.hitobject;//(coll_lineL.fract < 1.0f) ? coll_lineL.hitobject : ((coll_lineR.fract < 1.0f) ? coll_lineR.hitobject : NULL);

			//FReal frontCarBrake = 0.0f;

			if(hitObj)
			{
				if( (hitObj->m_flags & BODY_ISCAR) )
				{
					CCar* pCar = (CCar*)hitObj->GetUserData();

					if(m_prevFract == 0.0f)
						m_prevFract = 0.05f;

					float fromPrevPercentage = frontFract / m_prevFract;
					

					if( lineDist < AI_CAR_TRACE_DIST_MIN || (1.0f-fromPrevPercentage) >= 0.45f )
					{
						// if vehicle direction and speed differs
						float velDiff = dot((GetVelocity() + GetForwardVector()) - pCar->GetVelocity(), GetForwardVector());

						if( (velDiff > 15.0f || (1.0f-fromPrevPercentage) >= 0.45f) && carForwardSpeed > 0.25f )
							SignalRandomSequence(0.3f); //m_hornTime.SetIfNot(1.0f, 0.3f);

						// add the velocity difference
						float velDiff2 = carForwardSpeed - dot(pCar->GetVelocity(), pCar->GetForwardVector());
						speedDiff += velDiff2;
					}

				}
			}

			accelerator = clamp(frontFract - speedDiff, 0.0f, 0.5f);

			if( (speedDiff > -2.0f) || 
				(lineDist < AI_CAR_TRACE_DIST_MIN))
			{
				if( lineDist > AI_CAR_TRACE_DIST_MIN )
				{
					brake = clamp(speedDiff + (1.0f-frontFract), 0.1f, 0.9f);
				}
				else
				{
					brake = 0.8f;
					accelerator = 0.0f;
				}

				if(carForwardSpeed > 0.25f)
				{
					if(frontFract < 0.7f)
						controls |= IN_BRAKE;
				}
				else
				{
					controls &= ~IN_ACCELERATE;
					controls |= IN_HANDBRAKE;
				}
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

							m_emergencyEscapeSteer = sign(dot(GetRightVector(), frontObjColl.normal));
						}
					}

				} // emergencyFract < 1.0f

			} // TestConvexSweep

		} // if(carForwardSpeed > 5.0f)
	}

	if( m_emergencyEscape && m_emergencyEscapeTime > 0.0f )
	{
		m_emergencyEscapeTime -= fDt;
		
		if(m_emergencyEscapeTime < 0.7f && carForwardSpeed > 0.5f)
		{
			controls |= IN_BRAKE;
			brake = 1.0f;
			accelerator = 1.0f;
		}
		else
			fSteeringAngle = m_emergencyEscapeSteer;
	}
	else
		m_emergencyEscape = false;

	if( accelerator > 0.1f )
		GetPhysicsBody()->Wake();

	SetControlButtons( controls );

	accelerator = clamp(accelerator, 0.0f, 0.6f);

	SetControlVars(accelerator, brake, clamp(fSteeringAngle, -1.0f, 1.0f));

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