//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI traffic driver
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_Traffic.h"
#include "AICarManager.h"

#include "world.h"

ConVar g_disableTrafficLights("g_disableTrafficLights", "0", NULL, CV_CHEAT);

ConVar ai_traffic_debug_steering("ai_traffic_debug_steering", "0", NULL, CV_CHEAT);
ConVar ai_traffic_debug_junctions("ai_traffic_debug_junctions", "0", NULL, CV_CHEAT);
ConVar ai_traffic_debug_straigth("ai_traffic_debug_straigths", "0", NULL, CV_CHEAT);

ConVar ai_traffic_debug_route("ai_traffic_debug_route", "0", NULL, CV_CHEAT);

ConVar ai_changelane("ai_changelane", "-1", NULL, CV_CHEAT);

#define AI_TRACE_CONTENTS (OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE)

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


#define ROADNEIGHBOUR_OFFS_PX(x)		{x+1, x, x-1, x}		// non-diagonal, perpendicular
#define ROADNEIGHBOUR_OFFS_PY(y)		{y, y-1, y, y+1}

const float AI_STOPLINE_DIST = 4.7f;
const float AI_OBSTACLE_DIST = 4.8f;

const float AI_SIDECHECK_DIST_FR = 5.0f;
const float AI_SIDECHECK_DIST_RR = 20.0f;

const float AI_ROAD_STOP_DIST = 16.0f;

const float AI_CAR_TRACE_DIST_MIN = 6.0f;
const float AI_CAR_TRACE_DIST_MAX = 25.0f;

const float AI_ROAD_TRACE_DIST = 30.0f;
const float AI_MIN_LANE_CHANGE_DIST = 30.0f;

const float AI_TARGET_DIST_NOLANE = 4.0f;
const float AI_TARGET_DIST = 10.0f;
const float AI_TARGET_EXTENDED_DIST = 30.0f;

const float AI_LANE_SWITCH_DELAY = 15.0f;
const float AI_LANE_SWITCH_SHORTDELAY = 1.0f;

const float AI_EMERGENCY_ESCAPE_TIME = 0.5f;


#define STRAIGHT_CURRENT	0
#define STRAIGHT_PREV		1

//-------------------------------------------------------------------------------

const float AI_LONG_CAR_CONST = 3.25f;

// remember: first lane is rightmost!
bool SolveIntersection(int curDirection,
	int destDirection,
	int curRoadWidthLanes, int curLane,
	int destRoadWidth, int& destLane,
	bool isLongCar,
	bool availDirs[4])
{
	int availDirsCount = 0;

	bool forwardAvail = false;
	bool leftTurnAvail = false;
	bool rightTurnAvail = false;

	// there only can be 3 directions
	for (int i = 0; i < 4; i++)
	{
		if (availDirs[i])
		{
			if (!leftTurnAvail)
				leftTurnAvail = CompareDirection(curDirection, i + 1);

			if (!rightTurnAvail)
				rightTurnAvail = CompareDirection(curDirection, i - 1);

			if (!forwardAvail)
				forwardAvail = CompareDirection(curDirection, i);

			availDirsCount++;
		}
	}

	// only one direction is available and this is one of them?
	if (availDirsCount == 1 && availDirs[destDirection])
	{
		// go to the corresponding lane
		destLane = min(curLane, destRoadWidth);
		return true;
	}

	if (curLane == curRoadWidthLanes)	// last (left) lane
	{
		if (curDirection == destDirection)	// check forward
		{
			destLane = min(curLane, destRoadWidth);
			return true;
		}

		// only allowed to turn left, or go forward
		if (CompareDirection(curDirection, destDirection + 1))	// go left
		{
			// left turns to the left only
			destLane = destRoadWidth;
			return true;
		}

		return false;
	}
	else if (curLane == 1) // first (right) lane
	{
		if (curDirection == destDirection)	// check forward
		{
			destLane = min(curLane, destRoadWidth);
			return true;
		}

		if (CompareDirection(curDirection, destDirection - 1))	// go right
		{
			// allow long cars to go into the second lane, otherwise only to the first lane
			if (destRoadWidth >= 2 && isLongCar)
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
		if (!availDirs[curDirection])
		{
			// split by half
			float rightToLeftMod = (float)curLane / (float)curRoadWidthLanes;

			if (rightToLeftMod < 0.5f && !CompareDirection(curDirection, destDirection - 1))	// right
			{
				return false;
			}

			if (rightToLeftMod > 0.5f && !CompareDirection(curDirection, destDirection + 1))	// left
			{
				return false;
			}
		}
		else if (!CompareDirection(curDirection, destDirection))		// go forward pls
		{
			return false;
		}
	}

	destLane = min(curLane, destRoadWidth);

	return true;
}

CAITrafficManipulator::CAITrafficManipulator()
{
	m_changingLane = false;
	m_changingDirection = false;

	m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;
	m_emergencyEscape = false;
	m_emergencyEscapeTime = 0.0f;
	m_emergencyEscapeSteer = 1.0f;
	m_prevFract = 1.0f;

	m_triggerHorn = -1.0f;
	m_triggerHornDuration = -1.0f;

	m_maxSpeed = 60.0f;
}

void CAITrafficManipulator::DebugDraw(float fDt)
{
	// previous straight
	Vector3D	prevStartPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_PREV].start);
	Vector3D	prevEndPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_PREV].end);

	// current straight
	Vector3D	startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_CURRENT].start);
	Vector3D	endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_CURRENT].end);

	// shows the all route
	if (ai_traffic_debug_route.GetBool())
	{
		// show prev straight
		debugoverlay->Box3D(prevStartPos - 0.5f, prevStartPos + 0.5f, ColorRGBA(1, 0, 0, 1), fDt);
		debugoverlay->Box3D(prevEndPos - 0.5f, prevEndPos + 0.5f, ColorRGBA(1, 0, 0, 1), fDt);

		debugoverlay->Line3D(prevStartPos + Vector3D(0, 0.25f, 0), prevEndPos + Vector3D(0, 0.25, 0), ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1), fDt);

		// show cur straight
		debugoverlay->Box3D(startPos - 0.5f, startPos + 0.5f, ColorRGBA(1, 1, 0, 1), fDt);
		debugoverlay->Box3D(endPos - 0.5f, endPos + 0.5f, ColorRGBA(1, 1, 0, 1), fDt);

		debugoverlay->Line3D(startPos + Vector3D(0, 0.25f, 0), endPos + Vector3D(0, 0.25, 0), ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), fDt);

		// show next straight
		if (m_junction.selectedExit != -1)
		{
			straight_t& nextRoad = m_junction.exits[m_junction.selectedExit];

			Vector3D nextStart = g_pGameWorld->m_level.GlobalTilePointToPosition(nextRoad.start);
			Vector3D nextEnd = g_pGameWorld->m_level.GlobalTilePointToPosition(nextRoad.end);

			debugoverlay->Box3D(nextStart - 0.5f, nextStart + 0.5f, ColorRGBA(0, 1, 0, 1), fDt);
			debugoverlay->Box3D(nextEnd - 0.5f, nextEnd + 0.5f, ColorRGBA(0, 1, 0, 1), fDt);

			debugoverlay->Line3D(nextStart + Vector3D(0, 0.25f, 0), nextEnd + Vector3D(0, 0.25, 0), ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1), fDt);

			// show the transition line
			debugoverlay->Line3D(endPos + Vector3D(0, 0.25f, 0), nextStart + Vector3D(0, 0.25, 0), ColorRGBA(1, 1, 0, 1), ColorRGBA(0, 1, 0, 1), fDt);
		}
	}

	if (ai_traffic_debug_junctions.GetBool())
	{
		// draw
		for (int i = 0; i < m_junction.exits.numElem(); i++)
		{
			straight_t& road = m_junction.exits[i];

			Vector3D start = g_pGameWorld->m_level.GlobalTilePointToPosition(road.start);
			Vector3D end = g_pGameWorld->m_level.GlobalTilePointToPosition(road.end);

			debugoverlay->Box3D(start - 0.85f, start + 0.85f, ColorRGBA(1, 1, 0, 1), fDt);
			debugoverlay->Box3D(end - 0.5f, end + 0.5f, ColorRGBA(0.5, 1, 0, 1), fDt);

			debugoverlay->Text3D(start, 50.0f, ColorRGBA(1, 1, 1, 1), fDt, "lane %d", road.lane);
		}
	}
}

bool CAITrafficManipulator::HasRoad()
{
	return m_straights[STRAIGHT_CURRENT].direction != -1;
}

void CAITrafficManipulator::ChangeRoad(const straight_t& road)
{
	// cycle roads
	m_straights[STRAIGHT_PREV] = m_straights[STRAIGHT_CURRENT];
	m_straights[STRAIGHT_CURRENT] = road;

	m_provisionCompleted = false;

	// change conditions
	if (m_straights[STRAIGHT_PREV].direction != m_straights[STRAIGHT_CURRENT].direction)
	{
		m_changingDirection = true;
	}

	m_currEnd = road.end;

	// do it every time now
	m_junction.selectedExit = -1;
	m_junction.exits.clear();

	//SearchJunctionAndStraight();
}

void CAITrafficManipulator::SearchJunctionAndStraight()
{
	memset(m_junction.availDirs, false, sizeof(m_junction.availDirs));

	m_junction.exits.clear(false);
	m_junction.selectedExit = -1;

	if (!HasRoad())
		return;

	roadJunction_t junc = g_pGameWorld->m_level.Road_GetJunctionAtPoint(m_straights[STRAIGHT_CURRENT].end, 16);

	if (junc.breakIter <= 0)
		return;

	m_junction.size = junc.breakIter;

	bool gotSameDirection = false;
	bool sideJuncFound = false;

	// search for roads connected to junctions
	for (int i = junc.startIter; i < junc.breakIter + 2; i++)
	{
		IVector2D dir = GetDirectionVecBy(junc.start, junc.end);

		IVector2D checkPos = junc.start + dir * i;

		if (!gotSameDirection)
		{
			straight_t straightroad = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkPos, 8);

			if (straightroad.direction != -1 &&
				straightroad.breakIter > 1 &&
				!IsOppositeDirectionTo(m_straights[STRAIGHT_CURRENT].direction, straightroad.direction) &&
				(straightroad.end != m_straights[STRAIGHT_CURRENT].end) &&
				(straightroad.start != m_straights[STRAIGHT_CURRENT].start))
			{
				straightroad.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(straightroad.start);

				m_junction.exits.append(straightroad);
				gotSameDirection = true;
			}
		}


		IVector2D dirCheckVec = GetPerpendicularDirVec(dir);

		// left and right
		for (int j = 0; j < 32; j++)
		{
			int checkDir = j;

			if (j >= 7)
				checkDir = -(checkDir - 8);

			IVector2D checkStraightPos = checkPos + dirCheckVec * checkDir;

			levroadcell_t* rcell = g_pGameWorld->m_level.Road_GetGlobalTileAt(checkStraightPos);

			if (rcell && rcell->type == ROADTYPE_NOROAD)
			{
				if (j >= 7)
					break;
				else
					j = 7;

				continue;
			}

			int dirIdx = GetDirectionIndex(dirCheckVec*sign(checkDir));

			// calc steering dir
			straight_t sideroad = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkStraightPos, 8);

			if (sideroad.direction != -1 &&
				sideroad.direction == dirIdx &&
				(sideroad.end != m_straights[STRAIGHT_CURRENT].end) &&
				(sideroad.start != m_straights[STRAIGHT_CURRENT].start))
			{
				sideroad.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(sideroad.end);

				sideJuncFound = true;
				m_junction.exits.append(sideroad);
				break;
			}
		}

		// correction: move the end of road further
		if (!sideJuncFound)
			m_currEnd = checkPos;
	}

	// get the available directions
	for (int i = 0; i < m_junction.exits.numElem(); i++)
	{
		straight_t& road = m_junction.exits[i];

		m_junction.availDirs[road.direction] = true;
	}
}

void CAITrafficManipulator::ChangeLanes(CCar* car)
{
	if (!HasRoad())
		return;

	if (m_nextSwitchLaneTime > 0.0f)
		return;

	if (m_changingDirection || m_changingLane)
	{
		m_nextSwitchLaneTime = AI_LANE_SWITCH_SHORTDELAY;
		return;
	}

	IVector2D carPosOnCell = g_pGameWorld->m_level.PositionToGlobalTilePoint(car->GetOrigin());

	// if we have to turn, don't switch lanes when it's too late
	if (m_junction.selectedExit != -1)
	{
		int cellsBeforeEnd = GetCellsBeforeStraightEnd(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

		if (cellsBeforeEnd < 6)
			return;

		straight_t& nextRoad = m_junction.exits[m_junction.selectedExit];

		// don't change lanes if we're going in the same direction
		if (nextRoad.direction == m_straights[STRAIGHT_CURRENT].direction)
			return;
	}

	// calculate road width again
	int numLanes = g_pGameWorld->m_level.Road_GetNumLanesAtPoint(carPosOnCell);

	if (numLanes <= 1)
		return;

	IVector2D forwardDir = GetDirectionVec(m_straights[STRAIGHT_CURRENT].direction);
	IVector2D rightPerpDir = GetDirectionVec(m_straights[STRAIGHT_CURRENT].direction + 1);

	int randomLane = (ai_changelane.GetInt() != -1) ? ai_changelane.GetInt() : g_replayRandom.Get(0, 1);

	if (ai_changelane.GetInt() != -1) ai_changelane.SetInt(-1);
	if (randomLane == 0) randomLane = -1;

	int newLane = clamp(m_straights[STRAIGHT_CURRENT].lane + randomLane, 0, numLanes-1);

	// don't change to the same late
	if (m_straights[STRAIGHT_CURRENT].lane == newLane)
		return;

	int laneOfs = m_straights[STRAIGHT_CURRENT].lane - newLane;

	IVector2D laneTile = carPosOnCell + (forwardDir * 4) + rightPerpDir * laneOfs;
	Vector3D lanePos = g_pGameWorld->m_level.GlobalTilePointToPosition(laneTile) + Vector3D(0, 0.5f, 0);

	if (ai_traffic_debug_steering.GetBool())
		debugoverlay->Box3D(lanePos - 0.5f, lanePos + 0.5f, ColorRGBA(0, 0, 1, 1), 5.0f);

	straight_t newStraight = g_pGameWorld->m_level.Road_GetStraightAtPoint(laneTile, 8);

	if (newStraight.direction != -1 &&
		newStraight.direction == m_straights[STRAIGHT_CURRENT].direction)
	{
		IVector2D dir = GetDirectionVec(newStraight.direction);

		Vector3D laneDir(dir.x, 0, dir.y);

		Vector3D traceStart = lanePos - laneDir * AI_SIDECHECK_DIST_RR;
		Vector3D traceEnd = lanePos + laneDir * AI_SIDECHECK_DIST_FR;

		// Trace convex car
		CollisionData_t coll;

		eqPhysCollisionFilter collFilter;
		collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
		collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
		collFilter.AddObject(car->GetPhysicsBody());

		btConvexShape* shape = (btConvexShape*)car->GetPhysicsBody()->GetBulletShape();

		bool canSwitchLane = g_pPhysics->TestConvexSweep(shape, car->GetOrientation(), traceStart, traceEnd, coll, AI_TRACE_CONTENTS, &collFilter) == false;

		car->SetLight(CAR_LIGHT_EMERGENCY, false);

		if (canSwitchLane)
		{
			car->SetLight(laneOfs < 0 ? CAR_LIGHT_DIM_LEFT : CAR_LIGHT_DIM_RIGHT, true);

			ChangeRoad(newStraight);
			SearchJunctionAndStraight();

			m_changingLane = true;
			m_nextSwitchLaneTime = AI_LANE_SWITCH_DELAY;
		}

		if (ai_traffic_debug_steering.GetBool())
		{
			ColorRGBA col(0, 1, 1, 1);

			//col[m_switchedLane ? 1 : 0] = 1.0f;

			debugoverlay->Box3D(traceStart - 0.5f, traceStart + 0.5f, col, 5.0f);
			debugoverlay->Box3D(traceEnd - 0.5f, traceEnd + 0.5f, col, 5.0f);
			debugoverlay->Line3D(traceStart, traceEnd, col, col, 5.0f);
		}
	}
	else
	{
		if (ai_traffic_debug_steering.GetBool())
			debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "SwitchLane(): no parallel lane found\n");
	}
}

void CAITrafficManipulator::RoadProvision()
{
	if (!HasRoad() || m_provisionCompleted)
		return;

	IVector2D currentRoadEnd = m_straights[STRAIGHT_CURRENT].end;

	straight_t straightInfo = g_pGameWorld->m_level.Road_GetStraightAtPoint(currentRoadEnd, 8);

	// if it's still the same end, there is no need to continue
	if (straightInfo.breakIter <= 1 || straightInfo.direction != m_straights[STRAIGHT_CURRENT].direction)
	{
		m_provisionCompleted = true;
		return;
	}

	// set the end
	m_straights[STRAIGHT_CURRENT].end = straightInfo.end;
}

void CAITrafficManipulator::HandleJunctionExit(CCar* car)
{
	straight_t& current = m_straights[STRAIGHT_CURRENT];

	IVector2D carPosOnCell = g_pGameWorld->m_level.PositionToGlobalTilePoint(car->GetOrigin());
	int cellsBeforeEnd = GetCellsBeforeStraightEnd(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

	int curRoadWidth = g_pGameWorld->m_level.Road_GetNumLanesAtPoint(m_straights[STRAIGHT_CURRENT].start);

	if (m_junction.selectedExit != -1)
	{
		if (cellsBeforeEnd < 8 && !m_changingLane && !m_changingDirection)
		{
			straight_t& selRoad = m_junction.exits[m_junction.selectedExit];

			bool hasLeftTurn = CompareDirection(m_straights[STRAIGHT_CURRENT].direction - 1, selRoad.direction);

			if (hasLeftTurn)
				car->SetLight(CAR_LIGHT_DIM_LEFT, true);
			else if (CompareDirection(m_straights[STRAIGHT_CURRENT].direction + 1, selRoad.direction))
				car->SetLight(CAR_LIGHT_DIM_RIGHT, true);
		}

		straight_t& selectedExit = m_junction.exits[m_junction.selectedExit];

		// HACK: move change a bit further
		if (m_straights[STRAIGHT_CURRENT].direction != selectedExit.direction)	// assuming that is the right turn
		{
			if (m_straights[STRAIGHT_CURRENT].lane == 1)
			{
				carPosOnCell -= GetDirectionVec(m_straights[STRAIGHT_CURRENT].direction) * 2;
				cellsBeforeEnd = GetCellsBeforeStraightEnd(carPosOnCell, m_straights[STRAIGHT_CURRENT]);
			}
		}

		if (cellsBeforeEnd < 1)
		{
			ChangeRoad(m_junction.exits[m_junction.selectedExit]);
			return;
		}
	}
	else if(cellsBeforeEnd < 8)
	{
		SearchJunctionAndStraight();
		car->SetLight(CAR_LIGHT_EMERGENCY, false);
	}

	if (m_junction.selectedExit == -1 && m_junction.exits.numElem() > 0)
	{
		if (ai_traffic_debug_junctions.GetBool())
			debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "--- Has no selected straight ---");

		bool isLongCar = (car->m_conf->physics.body_size.z > AI_LONG_CAR_CONST);

		DkList<int> allowedStraightIds;

		for (int i = 0; i < m_junction.exits.numElem(); i++)
		{
			straight_t& road = m_junction.exits[i];

			int destRoadWidth = g_pGameWorld->m_level.Road_GetNumLanesAtPoint(road.start);

			int destLane = -1;
			bool result = SolveIntersection(m_straights[STRAIGHT_CURRENT].direction, road.direction,
				curRoadWidth, m_straights[STRAIGHT_CURRENT].lane,
				destRoadWidth, destLane,
				isLongCar,
				m_junction.availDirs);

			// check simple laws
			if (result && (destLane == -1 || destLane == road.lane))
				allowedStraightIds.append(i);
		}

		if (allowedStraightIds.numElem())
		{
			int rndIdx = g_replayRandom.Get(0, allowedStraightIds.numElem() - 1);
			m_junction.selectedExit = allowedStraightIds[rndIdx];

			if (ai_traffic_debug_junctions.GetBool())
				debugoverlay->TextFadeOut(0, ColorRGBA(1), 5.0f, "--- Selected allowed straight earlier ---");
		}
		else
		{
			// try select forward
			for (int i = 0; i < m_junction.exits.numElem(); i++)
			{
				straight_t& road = m_junction.exits[i];

				// check simple laws
				if (CompareDirection(m_straights[STRAIGHT_CURRENT].direction, road.direction))
					allowedStraightIds.append(i);
			}

			if (allowedStraightIds.numElem())
			{
				int rndIdx = g_replayRandom.Get(0, allowedStraightIds.numElem() - 1);
				m_junction.selectedExit = allowedStraightIds[rndIdx];
			}
			else
			{
				m_junction.selectedExit = g_replayRandom.Get(0, m_junction.exits.numElem() - 1);
			}
		}
	}
}

void CAITrafficManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	handling.acceleration = 0.0f;
	handling.braking = 0.0f;
	handling.steering = 0.0f;
	handling.confidence = 1.0f;
	handling.autoHandbrake = false;

	HandleJunctionExit(car);

	DebugDraw(fDt);

	// 1. Build a line (A-B) from cell grid to 3D positions
	// 2. Make middle (or near) point (C), lerp(a,b,0.25f)
	//		A---C------------B
	// 3. Drive car to the C point

	m_nextSwitchLaneTime -= fDt;

	Vector3D	carForward = car->GetForwardVector();
	
	Vector3D	carRight = car->GetRightVector();
	Vector3D	carPos = car->GetOrigin() + carForward * car->m_conf->physics.body_size.z*0.5f;

	const Vector3D&	carTracePos = car->GetOrigin();
	const Vector3D&	carVelocity = car->GetVelocity();
	const Quaternion& carOrient = car->GetOrientation();

	Vector3D	carBodySide = car->GetRightVector()*car->m_conf->physics.body_size.x*0.85f;

	IVector2D	carPosOnCell = g_pGameWorld->m_level.PositionToGlobalTilePoint(carPos);

	// previous straight
	Vector3D	prevStartPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_PREV].start);
	Vector3D	prevEndPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_PREV].end);

	// current straight
	Vector3D	startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_CURRENT].start);
	Vector3D	endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_straights[STRAIGHT_CURRENT].end);

	Vector3D	realEndPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_currEnd);

	Vector3D	roadDir = fastNormalize(startPos - endPos);

	// road end
	Plane		roadEndPlane(roadDir, -dot(roadDir, endPos));
	float		distToStop = roadEndPlane.Distance(carPos);

	// do some things if were not too close to stop line
	if (distToStop > AI_ROAD_STOP_DIST)
	{
		RoadProvision();
		ChangeLanes(car);
	}

	Plane		roadRealEndPlane(roadDir, -dot(roadDir, realEndPos));
	float		distToChange = roadRealEndPlane.Distance(carPos);

	float invStraightDist = 1.0f / length(startPos - endPos);
	float projWay = lineProjection(startPos, endPos, carPos);

	float targetDistance = (projWay < 0.0f) ? (m_changingLane ? AI_TARGET_EXTENDED_DIST : AI_TARGET_DIST_NOLANE) : AI_TARGET_DIST;

	float targetOfsOnLine = projWay + targetDistance * invStraightDist;

	bool isOnPrevRoad = IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_PREV]);
	bool isOnCurrRoad = IsPointOnStraight(carPosOnCell, m_straights[STRAIGHT_CURRENT]);

	// steering target
	Vector3D steeringTargetPos = lerp(startPos, endPos, targetOfsOnLine);

	float maxSpeed = m_maxSpeed * trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType] + m_speedModifier;

	float carForwardSpeed = dot(carForward, carVelocity);

	float carSpeed = fabs(carForwardSpeed)*MPS_TO_KPH;

	float brakeDistancePerSec = car->m_conf->GetBrakeEffectPerSecond()*0.5f;

	float brakeToStopTime = carForwardSpeed / brakeDistancePerSec * 2.0f;
	float brakeDistAtCurSpeed = brakeDistancePerSec * brakeToStopTime;

	handling.acceleration = ((maxSpeed - carSpeed) / maxSpeed); // go forward
	handling.acceleration = 1.0f - pow(1.0f - handling.acceleration, 4.0f);

	Vector3D steerAbsDir = fastNormalize(steeringTargetPos - carPos);

	Vector3D steerRelatedDir = fastNormalize((!car->m_worldMatrix.getRotationComponent()) * steerAbsDir);

	float fSteeringAngle = clamp(atan2f(steerRelatedDir.x, steerRelatedDir.z), -1.0f, 1.0f);
	fSteeringAngle = sign(fSteeringAngle) * powf(fabs(fSteeringAngle), 1.25f);

	// modifier by steering
	handling.acceleration *= trafficSpeedModifier[g_pGameWorld->m_envConfig.weatherType];

	if (ai_traffic_debug_straigth.GetBool())
	{
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "isOnCurrRoad: %d\n", isOnCurrRoad);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "isOnPrevRoad: %d\n", isOnPrevRoad);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "changing lane: %d\n", m_changingLane);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "change lane timeout: %.2f (dist: %.2f)\n", m_nextSwitchLaneTime, distToStop);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "changing direction: %d\n", m_changingDirection);
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "num exit straights: %d\n", m_junction.exits.numElem());
		debugoverlay->TextFadeOut(0, 1.0f, 5.0f, "selected next straight: %d\n", m_junction.selectedExit);
		
		
	}

	// disable lights turning
	if (m_changingDirection && isOnCurrRoad)
	{
		SearchJunctionAndStraight();

		car->SetLight(CAR_LIGHT_EMERGENCY, false);
		m_changingDirection = false;
	}

	if (m_changingLane && !isOnPrevRoad && isOnCurrRoad)
	{
		SearchJunctionAndStraight();

		car->SetLight(CAR_LIGHT_EMERGENCY, false);
		m_changingLane = false;
	}

	//
	// Breakage on red light
	//
	if (distToStop < AI_ROAD_STOP_DIST && distToStop > 0.0f)
	{
		// brake on global traffic light value
		int trafficLightDir = g_pGameWorld->m_globalTrafficLightDirection;

		int curDir = m_straights[STRAIGHT_CURRENT].direction;

		levroadcell_t* roadTile = g_pGameWorld->m_level.Road_GetGlobalTileAt(carPosOnCell);

		// check current tile for traffic light indication
		bool hasTrafficLight = roadTile ? (roadTile->flags & ROAD_FLAG_TRAFFICLIGHT) : false;

		bool isAllowedToMove = hasTrafficLight ? (trafficLightDir % 2 == curDir % 2) : true;

		if (!g_disableTrafficLights.GetBool() && (!isAllowedToMove || (g_pGameWorld->m_globalTrafficLightTime < 2.0f)))
		{
			float brakeSpeedDiff = brakeDistAtCurSpeed + AI_STOPLINE_DIST - distToStop;
			brakeSpeedDiff = max(brakeSpeedDiff, 0.0f);

			float brake = brakeSpeedDiff / AI_ROAD_STOP_DIST;
			brake = min(brake, 0.9f);


			if (distToStop > AI_STOPLINE_DIST)
			{
				handling.acceleration -= brake * 2.0f;
				handling.braking = brake;
			}
			else
			{
				handling.acceleration = 0.0f;
				handling.braking = brake;
			}
		}
		else if (m_junction.selectedExit != -1)
		{
			//
			// Breakage before junction when changing direction
			//

			straight_t& selRoad = m_junction.exits[m_junction.selectedExit];

			if (m_straights[STRAIGHT_CURRENT].direction != selRoad.direction)
			{
				float fDecelRate = RemapValClamp(distToStop, 0.0f, AI_ROAD_STOP_DIST, 1.0f, 0.25f);
				handling.acceleration -= fDecelRate;
			}
		}
	}

	// if we're on intersection
	if (m_changingDirection && !isOnCurrRoad)
	{
		DkList<CCar*> forwardCars;
		g_pAIManager->QueryTrafficCars(forwardCars, m_junction.size*4.0f, carPos, carForward, 0.8f);

		CCar* nearestOppositeMovingCar = nullptr;
		float maxDist = DrvSynUnits::MaxCoordInUnits;

		for (int i = 0; i < forwardCars.numElem(); i++)
		{
			if (forwardCars[i] == car || forwardCars[i]->GetSpeed() < 5.0f)
				continue;

			const Vector3D& velocity = forwardCars[i]->GetVelocity();

			if (dot(fastNormalize(velocity), -carForward) > 0.4f)
			{
				float distBetweenCars = distance(carTracePos, forwardCars[i]->GetOrigin());

				if (distBetweenCars < maxDist)
				{
					nearestOppositeMovingCar = forwardCars[i];
					maxDist = distBetweenCars;
				}
			}
		}

		if (nearestOppositeMovingCar)
		{
			debugoverlay->Line3D(carTracePos, startPos, ColorRGBA(1,0,0,1), ColorRGBA(1, 0, 0, 1), fDt);
		
			Vector3D oppLeft = -nearestOppositeMovingCar->GetRightVector();
			Vector3D oppPos = nearestOppositeMovingCar->GetOrigin() + oppLeft;

			debugoverlay->Line3D(oppPos + oppLeft, oppPos + oppLeft*2.0f, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1), fDt);
			debugoverlay->Sphere3D(oppPos, 2.0f, ColorRGBA(1, 0, 0, 1), fDt);

			Plane oppPl(oppLeft, -dot(oppLeft, oppPos));

			Vector3D out;
			float fractOut;

			// trace the line from my center to the road start
			if (oppPl.ClassifyPoint(carTracePos) == CP_FRONT && oppPl.GetIntersectionLineFraction(carTracePos, startPos, out, fractOut))
			{
				debugoverlay->Line3D(carTracePos, out, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), fDt);
				debugoverlay->Sphere3D(out, 0.25f, ColorRGBA(1, 1, 0, 1), fDt);

				handling.braking = (1.0f - fractOut);
			}
		}

		handling.acceleration -= fabs(fSteeringAngle)*0.65f;
	}

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(car->GetPhysicsBody());

	//
	// Breakage when has object in front
	//
	{
		float fTraceDist = AI_CAR_TRACE_DIST_MAX * trafficBrakeDistModifier[g_pGameWorld->m_envConfig.weatherType];

		CollisionData_t frontObjColl;

		// FIXME: detect steering wheels?
		//Matrix3x3 wrotation = car->GetWheel(0).m_wheelOrient*transpose(car->m_worldMatrix).getRotationComponent();

		Vector3D traceDir = lerp(carForward, steerAbsDir, pow(fabs(fSteeringAngle), 0.5f));

		Vector3D carTraceEndPos = carTracePos + traceDir * fTraceDist;
		//Vector3D navTraceEndPos = carTracePos+traceDir*(carSpeed*0.1f+4.0f);

		//debugoverlay->Line3D(carTracePos, carTraceEndPos, ColorRGBA(1,0,0,1), ColorRGBA(0,0,1,1), 0.05f);

		btConvexShape* carShape = (btConvexShape*)car->GetPhysicsBody()->GetBulletShape();
		btBoxShape carBoxShape(btVector3(car->m_conf->physics.body_size.x, car->m_conf->physics.body_size.y * 2, 0.25f));

		//float navTestLine = g_pGameWorld->m_level.Nav_TestLine(carTracePos, navTraceEndPos, true);

		if (g_pPhysics->TestConvexSweep(&carBoxShape, carOrient, carTracePos, carTraceEndPos, frontObjColl, AI_TRACE_CONTENTS, &collFilter))
		{
			float frontFract = frontObjColl.fract;//min(navTestLine, frontObjColl.fract);

			float lineDist = frontFract * fTraceDist;
			float diffForwardSpeed = carForwardSpeed;

			CEqCollisionObject* hitObj = frontObjColl.hitobject;

			//FReal frontCarBrake = 0.0f;

			if (hitObj)
			{
				if ((hitObj->m_flags & BODY_ISCAR))
				{
					CCar* pCar = (CCar*)hitObj->GetUserData();

					// add the velocity difference
					float frontCarSpeed = dot(pCar->GetVelocity(), pCar->GetForwardVector());

					if (frontCarSpeed > 8.0f)
						diffForwardSpeed -= frontCarSpeed * 0.5f;

					if (m_prevFract == 0.0f)
						m_prevFract = 0.05f;

					float fromPrevPercentage = frontFract / m_prevFract;

					if (lineDist < AI_CAR_TRACE_DIST_MIN || (1.0f - fromPrevPercentage) >= 0.45f)
					{
						// if vehicle direction and speed differs
						float velDiff = dot((carVelocity + carForward) - pCar->GetVelocity(), carForward);

						if ((velDiff > 15.0f || (1.0f - fromPrevPercentage) >= 0.45f) && carForwardSpeed > 0.25f)
						{
							m_triggerHorn = 0.3f;
							m_triggerHornDuration = 0.0f;
						}
						
					}
				}
			}

			if (lineDist < AI_ROAD_STOP_DIST)
			{
				float dbrakeToStopTime = diffForwardSpeed / brakeDistancePerSec * 2.0f;
				float dbrakeDistAtCurSpeed = brakeDistancePerSec * dbrakeToStopTime;

				float brakeSpeedDiff = dbrakeDistAtCurSpeed + AI_OBSTACLE_DIST - lineDist;
				brakeSpeedDiff = max(brakeSpeedDiff, 0.0f);

				float brake = brakeSpeedDiff / 20.0f;

				if (lineDist > AI_OBSTACLE_DIST)
				{
					handling.acceleration -= brake * 2.0f;
					handling.braking = brake;
				}
				else
				{
					handling.braking = brake;
					handling.acceleration = 0.0f;
				}
			}
			else
			{
				handling.acceleration *= (frontFract - 0.5f) * 2.0f;
			}

			m_prevFract = frontFract;
		}

		//
		// EMERGENCY BRAKE AND STEERING
		//
		if (carForwardSpeed > 5.0f)
		{
			fTraceDist = RemapValClamp(carForwardSpeed, 0.0f, 40.0f, AI_CAR_TRACE_DIST_MIN, AI_CAR_TRACE_DIST_MAX)*3.5f;

			handling.autoHandbrake = false;

			carTraceEndPos = carTracePos + carForward*fTraceDist;

			if (g_pPhysics->TestConvexSweep(carShape, carOrient, carTracePos, carTraceEndPos, frontObjColl, AI_TRACE_CONTENTS, &collFilter))
			{
				float emergencyFract = frontObjColl.fract;

				//
				// Emergency situation
				//
				if (emergencyFract < 1.0f)
				{
					CEqCollisionObject* hitObj = frontObjColl.hitobject;//(coll_lineL.fract < 1.0f) ? coll_lineL.hitobject : ((coll_lineR.fract < 1.0f) ? coll_lineR.hitobject : NULL);

					if (hitObj->m_flags & BODY_ISCAR)
					{
						CCar* pCar = (CCar*)hitObj->GetUserData();

						// if vehicle direction and speed differs
						float velDiff = dot((carVelocity + carForward) - pCar->GetVelocity(), carForward);

						if (velDiff > 30.0f)
						{
							m_triggerHorn = 0.3f;;
							m_triggerHornDuration = 1.5f;

							if (!m_emergencyEscape && emergencyFract < 0.5f)
							{
								m_emergencyEscape = true;

								m_emergencyEscapeTime = AI_EMERGENCY_ESCAPE_TIME;

								handling.autoHandbrake = true;
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
	if (m_emergencyEscape && m_emergencyEscapeTime > 0.0f)
	{
		m_emergencyEscapeTime -= fDt;
		fSteeringAngle = m_emergencyEscapeSteer;
	}
	else
		m_emergencyEscape = false;

	if (handling.acceleration > 0.05f)
		car->GetPhysicsBody()->Wake();

	float speedClampFac = 20.0f - carSpeed;
	handling.acceleration *= RemapValClamp(speedClampFac, 0.0f, 20.0f, 0.6f, 1.0f);
	handling.steering = clamp(fSteeringAngle, -0.95f, 0.95f);

	//SetControlVars(accelerator, brake, clamp(fSteeringAngle, -0.95f, 0.95f));
}