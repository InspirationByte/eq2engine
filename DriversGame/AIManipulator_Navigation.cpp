//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_Navigation.h"
#include "car.h"

const float AI_STRAIGHT_PATH_MAX_DISTANCE = 40.0f;

const float AI_PATH_TIME_TO_UPDATE = 5.0f;			// every 5 seconds
const float AI_PATH_TIME_TO_UPDATE_FORCE = 0.1f;

const float AI_PATH_COMPLETION_TO_FASTER_SEARCH = 0.35f;	// percent
const float AI_PATH_COMPLETION_TO_FULL_RENEW = 0.5f;		// percent

const float AI_SEGMENT_VALID_RADIUS = 2.0f; // distance to segment

const int AI_PATH_SEARCH_RANGE = 4096;

ConVar ai_debug_navigator("ai_debug_navigator", "0");
ConVar ai_debug_path("ai_debug_path", "0");
ConVar ai_debug_search("ai_debug_search", "0");

static float s_weatherBrakeDistanceModifier[WEATHER_COUNT] =
{
	0.50f,
	0.57f,
	0.59f
};

static float s_weatherBrakeCurve[WEATHER_COUNT] =
{
	1.00f,
	0.84f,
	0.79f
};

void DebugDrawPath(pathFindResult_t& path, float time = -1.0f)
{
	if(ai_debug_path.GetBool() && path.points.numElem())
	{
		Vector3D lastLinePos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[0]);

		for (int i = 1; i < path.points.numElem(); i++)
		{
			Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[i]);

			float dispalyTime = time > 0 ? time : float(i) * 0.1f;

			debugoverlay->Box3D(pointPos - 0.15f, pointPos + 0.15f, ColorRGBA(1, 1, 0, 1.0f), dispalyTime);
			debugoverlay->Line3D(lastLinePos, pointPos, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), dispalyTime);

			lastLinePos = pointPos;
		}
	}
}

//------------------------------------------------------------------------

const float SIMPLIFY_TRACE_WIDTH = 5.0f;

void SimplifyPath(pathFindResult_t& path)
{
	if(path.points.numElem() <= 2)
		return;
	
	IVector2D lastPointPos = path.points[0];
	IVector2D lastDir2D = path.points[1]-lastPointPos;

	for (int i = 2; i < path.points.numElem()-1; i++)
	{
		IVector2D pointPos = path.points[i];

		IVector2D dir2D(pointPos-lastPointPos);

		// process conditions
		bool isStraight = lastDir2D == dir2D;
		//bool hasCollision = g_pGameWorld->m_level.Nav_TestLine2D(lastPointPos, pointPos) < 1.0f;

		// store last data
		lastPointPos = pointPos;
		lastDir2D = dir2D;

		if(!isStraight)//if(!(isStraight || hasCollision))
		{
			continue;
		}

		i--;
		path.points.removeIndex(i);
	}

}

CAINavigationManipulator::CAINavigationManipulator()
{
	m_driveTarget = vec3_zero;
	m_pathPointIdx = -1;
	m_timeToUpdatePath = 0.0f;
	m_lastSuccessfulSearchPos = vec3_zero;
	m_searchFails = 0;
	m_init = false;
}

void CAINavigationManipulator::SetPath(pathFindResult_t& newPath, const Vector3D& searchPos)
{
	m_path = newPath;
	SimplifyPath( m_path );

	m_pathPointIdx = 0;
	m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE;

	m_lastSuccessfulSearchPos = searchPos;
	m_searchFails = 0;

	DebugDrawPath(m_path);

	m_init = true;
}

Vector3D CAINavigationManipulator::GetAdvancedPointByDist(int& startSeg, float distFromSegment)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	if(m_path.points.numElem() == 0)
		return -1;

	while (startSeg < m_path.points.numElem()-2)
	{
		currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[startSeg]);
		nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[startSeg + 1]);

		float segLen = length(currPointPos - nextPointPos);

		if(distFromSegment > segLen)
		{
			distFromSegment -= segLen;
			startSeg++;
		}
		else
			break;
	}

	currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[startSeg]);
	nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[startSeg + 1]);

	Vector3D dir = fastNormalize(nextPointPos - currPointPos);

	float lastSegLen = length(currPointPos - nextPointPos);
	//distFromSegment = clamp(distFromSegment, 0.0f, lastSegLen);

	return currPointPos + dir*distFromSegment;
}

int CAINavigationManipulator::FindSegmentByPosition(const Vector3D& pos, float distToSegment)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	int segment = 0;

	if(m_path.points.numElem() == 0)
		return -1;

	while (segment < m_path.points.numElem()-1)
	{
		currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segment]);
		nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segment + 1]);

		float segProj = lineProjection(currPointPos, nextPointPos, pos);

		Vector3D posOnSegment = lerp(currPointPos, nextPointPos, segProj);

		float distToSegPos = length(posOnSegment-pos);

		if(segProj >= 0.0f && segProj <= 1.0f && distToSegPos < distToSegment)
			return segment;

		segment++;
	}

	return -1;
}

float CAINavigationManipulator::GetPathPercentage(const Vector3D& pos)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	int segment = 0;

	if(m_path.points.numElem() == 0)
		return 1.0f;

	float lengthOnPath = 0.0f;
	float totalPathLength = 0.0f;

	const float PATH_MAX_DISTANCE = 10.0f;

	while (segment < m_path.points.numElem()-1)
	{
		currPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segment]);
		nextPointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segment + 1]);

		float segmentLength = length(currPointPos-nextPointPos);
		float segProj = lineProjection(currPointPos, nextPointPos, pos);

		segProj = clamp(segProj, 0.0f, 1.0f);

		Vector3D posOnSegment = lerp(currPointPos, nextPointPos, segProj);
		float distToSegPos = length(posOnSegment-pos);

		if(distToSegPos < PATH_MAX_DISTANCE)
			lengthOnPath += segmentLength*segProj;

		totalPathLength += segmentLength;

		segment++;
	}

	return lengthOnPath / totalPathLength;
}

void CAINavigationManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	Vector3D carPos = car->GetOrigin();
	Vector3D carBodySize = car->m_conf->physics.body_size;

	const Vector3D& carVelocity = car->GetVelocity();
	
	Matrix4x4 bodyMat;
	car->GetPhysicsBody()->ConstructRenderMatrix(bodyMat);

	Vector3D carForward = car->GetForwardVector();

	Plane frontBackCheckPlane(carForward, -dot(carForward, carPos));

	float distToTarget = length(m_driveTarget - carPos);

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;

	float brakeDistancePerSec = car->m_conf->GetBrakeEffectPerSecond()*0.5f;
	float brakeToStopTime = speedMPS / brakeDistancePerSec*2.0f;				// time to stop from the current speed
	float brakeDistAtCurSpeed = brakeDistancePerSec*brakeToStopTime;			// the brake distance in one second

	bool doesHaveStraightPath = true;

	// test for the straight path and visibility
	if(distToTarget < AI_STRAIGHT_PATH_MAX_DISTANCE)
	{
		float tracePercentage = g_pGameWorld->m_level.Nav_TestLine(carPos, m_driveTarget, false);

		if(tracePercentage < 1.0f)
			doesHaveStraightPath = false;
	}
	else
	{
		doesHaveStraightPath = false;
	}

	if(ai_debug_navigator.GetBool())
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "straight path: %d", doesHaveStraightPath);

	// search on timeout active here
	//if(!doesHaveStraightPath)
	{
		//
		// Get the last point on the path
		//
		int currSeg = FindSegmentByPosition(carPos, AI_SEGMENT_VALID_RADIUS);
		Vector3D pathTarget = (currSeg != -1) ? g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[currSeg]) : m_driveTarget;

		if(currSeg == -1)
		{
			if(m_timeToUpdatePath > AI_PATH_TIME_TO_UPDATE_FORCE )
				m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE_FORCE;

			currSeg = 0;
		}

		float pathCompletionPercentage = GetPathPercentage(carPos);//(float)currSeg / (float)m_path.points.numElem();

		// if we have old path, try continue moving
		if(	pathCompletionPercentage < AI_PATH_COMPLETION_TO_FASTER_SEARCH && m_path.points.numElem() > 1)
		{
			m_timeToUpdatePath -= fDt;
		}
		else
		{
			if(m_timeToUpdatePath > AI_PATH_TIME_TO_UPDATE_FORCE )
				m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE_FORCE;

			m_timeToUpdatePath -= fDt;
		}

		// draw path
		DebugDrawPath(m_path, fDt);

		if(ai_debug_navigator.GetBool())
		{
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "pathCompletionPercentage: %d (length=%d)", (int)(pathCompletionPercentage*100.0f), m_path.points.numElem());
			//debugoverlay->TextFadeOut(0, color4_white, 10.0f, "nextPathUpdateTime: %g", m_timeToUpdatePath);
		}

		ubyte tagetNavPoint = g_pGameWorld->m_level.Nav_GetTileAtGlobalPoint( g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(m_driveTarget) );

		if(tagetNavPoint > 0)
			m_targetSuccessfulSearchPos = m_driveTarget;

		if( m_timeToUpdatePath <= 0.0f)
		{
			Vector3D searchStart(carPos);

			int trials = m_searchFails;

			// try search from last successfull search point to not get stuck
			if(m_searchFails > 0)
				searchStart = m_lastSuccessfulSearchPos;

			pathFindResult_t newPath;
			if( g_pGameWorld->m_level.Nav_FindPath(m_targetSuccessfulSearchPos, searchStart, newPath, AI_PATH_SEARCH_RANGE, false))
			{
				SetPath(newPath, carPos);
			}
			else
			{
				m_searchFails++;

				if(m_searchFails > 1)
					m_searchFails = 0;

				m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE_FORCE;
			}

			if(ai_debug_search.GetBool())
				debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path search result: %d points, tries: %d", newPath.points.numElem(), trials);
		}
	}

	if(!m_init && !doesHaveStraightPath)
	{
		memset(&handling, 0, sizeof(handling));
		return;
	}

	//
	// Introduce handling basics
	//

	float weatherBrakeDistModifier = s_weatherBrakeDistanceModifier[g_pGameWorld->m_envConfig.weatherType];
	float weatherBrakePow = s_weatherBrakeCurve[g_pGameWorld->m_envConfig.weatherType];

	const float AI_LOWSPEED_CURVE		= 15.0f;
	const float AI_LOWSPEED_END			= 8.0f;	// 8 meters per second

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_CURVE);

	// forcing auto handbrake on low speeds
	//if(lowSpeedFactor < 0.1f)
		handling.autoHandbrake = true;

	//handling.autoHandbrake = true;

	const float AI_OBSTACLE_SPHERE_RADIUS = car->m_conf->physics.body_size.x*2.0f;
	const float AI_OBSTACLE_SPHERE_SPEED_SCALE = 0.01f;

	const float AI_OBSTACLE_CORRECTION_CONST_DISTANCE = 2.0f;
	const float AI_OBSTACLE_CORRECTION_SPEED_SCALE = 0.2f;
	const float AI_OBSTACLE_CORRECTION_CURVE = 0.5f;

	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject( car->GetPhysicsBody() );
	collFilter.AddObject( m_excludeColl );

	int traceContents = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE;

	float traceShapeRadius = AI_OBSTACLE_SPHERE_RADIUS + speedMPS*AI_OBSTACLE_SPHERE_SPEED_SCALE;

	CollisionData_t steeringTargetColl;
	CollisionData_t pathColl;

	btSphereShape sphereTraceShape(traceShapeRadius);

	if(doesHaveStraightPath)
	{
		// add half car length to the car position
		carPos += carForward*carBodySize.z;

		m_timeToUpdatePath = 0.0f;

		const float AI_CHASE_TARGET_VELOCITY_SCALE = 0.6f;

		const float AI_STEERING_START_AGRESSIVE_DISTANCE = 15.0f;

		// brake curve
		const float AI_CHASE_BRAKE_CURVE = 0.35f;

		const float AI_CHASE_BRAKE_MIN = 0.05f;

		const float AI_BRAKE_TARGET_DISTANCE_SCALING = 3.5f;
		const float AI_BRAKE_TARGET_DISTANCE_CURVE = 1.5f;

		const float AI_STEERING_TARGET_DISTANCE_SCALING = 0.8f;
		const float AI_STEERING_TARGET_DISTANCE_CURVE = 2.5f;

		// add velocity offset
		Vector3D steeringTargetPos = m_driveTarget + m_driveTargetVelocity * AI_CHASE_TARGET_VELOCITY_SCALE;

		// preliminary steering dir
		Vector3D steeringDir = fastNormalize(steeringTargetPos - carPos);

		float distanceToTarget = length(steeringTargetPos - carPos);

		float distanceScaling = 1.0f - RemapValClamp(distanceToTarget, 0.0f, AI_STEERING_START_AGRESSIVE_DISTANCE, 0.0f, 1.0f);

		float steeringScaling = 1.0f + pow(distanceScaling, AI_STEERING_TARGET_DISTANCE_CURVE) * AI_STEERING_TARGET_DISTANCE_SCALING;
		
		// calculate brake first
		float brakeScaling = 1.0f + pow(distanceScaling, AI_BRAKE_TARGET_DISTANCE_CURVE) * AI_BRAKE_TARGET_DISTANCE_SCALING;
		float brakeFac = (1.0f-fabs(dot(steeringDir, carForward)))*brakeScaling;

		if(brakeFac < AI_CHASE_BRAKE_MIN)
			brakeFac = 0.0f;
		
		float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 4.0f, 15.0f);

		// trace from position A to position B first
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, steeringTargetPos, pathColl,
			traceContents,
			&collFilter);

		if(pathColl.fract < 1.0f)
		{
			float AI_OBSTACLE_PATH_CORRECTION_AMOUNT = 1.1f;
			float AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT = 1.5f;

			Vector3D offsetAmount = pathColl.normal * (traceShapeRadius * AI_OBSTACLE_PATH_CORRECTION_AMOUNT + lowSpeedFactor*AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT);

			steeringTargetPos = pathColl.position + offsetAmount;

			if(ai_debug_navigator.GetBool())
			{
				debugoverlay->Line3D(pathColl.position, steeringTargetPos, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
				debugoverlay->Sphere3D(steeringTargetPos, traceShapeRadius, ColorRGBA(1, 1, 0, 1.0f), fDt);
			}
		}

		// trace forward from car using speed
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, carPos+carForward*forwardTraceDistanceBySpeed, steeringTargetColl,
			traceContents,
			&collFilter);

		if(steeringTargetColl.fract < 1.0f)
		{
			float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 3.5f;

			float offsetAmount = 1.0f - steeringTargetColl.fract;

			steeringTargetPos = steeringTargetColl.position + steeringTargetColl.normal * offsetAmount * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;

			if(ai_debug_navigator.GetBool())
			{
				debugoverlay->Line3D(steeringTargetColl.position, steeringTargetPos, ColorRGBA(0, 1, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
				debugoverlay->Sphere3D(steeringTargetPos, traceShapeRadius, ColorRGBA(0, 1, 0, 1.0f), fDt);
			}
		}

		// final steering dir after collision tests
		steeringDir = fastNormalize(steeringTargetPos - carPos);

		// calculate steering
		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z) * steeringScaling;
		handling.steering = clamp(handling.steering, -1.0f, 1.0f);

		handling.braking = pow(brakeFac, AI_CHASE_BRAKE_CURVE*weatherBrakePow) * lowSpeedFactor;
		handling.braking = min(handling.braking, 1.0f);
		handling.acceleration = 1.0f - handling.braking;
	}
	else if(m_path.points.numElem() > 0 && m_pathPointIdx != -1 && m_pathPointIdx < m_path.points.numElem()-2)
	{
		int segmentByCarPosition = FindSegmentByPosition(carPos, AI_SEGMENT_VALID_RADIUS);
		
		// segment must be corrected
		if(segmentByCarPosition > m_pathPointIdx)
			m_pathPointIdx = segmentByCarPosition;

		if(segmentByCarPosition == -1)
			segmentByCarPosition = m_pathPointIdx;

		// the path segment by m_pathPointIdx
		Vector3D pathSegmentStart = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[m_pathPointIdx]);
		Vector3D pathSegmentEnd = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[m_pathPointIdx + 1]);

		float pathSegmentLength = length(pathSegmentStart - pathSegmentEnd);
		float pathSegmentProj = lineProjection(pathSegmentStart, pathSegmentEnd, carPos);

		// the path segment by segmentByCarPosition
		Vector3D carPosSegmentStart = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segmentByCarPosition]);
		Vector3D carPosSegmentEnd = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[segmentByCarPosition + 1]);

		float carPosSegmentLength = length(carPosSegmentStart - carPosSegmentEnd);
		float carPosSegmentProj = lineProjection(carPosSegmentStart, carPosSegmentEnd, carPos);

		// calculate distance between car position and point on the segment by car position
		// to 
		Vector3D carPosOnSegmentByCarPos = lerp(carPosSegmentStart, carPosSegmentEnd, carPosSegmentProj);
		float distFromSegmentByCarPos = length(carPosOnSegmentByCarPos - carPos);

		debugoverlay->Box3D(carPosOnSegmentByCarPos - 0.5f, carPosOnSegmentByCarPos + 0.5f, ColorRGBA(1, 0, 1, 1.0f), fDt);

		// distance from path segment start
		float distFromPathSegStart = pathSegmentProj*pathSegmentLength;

		const float AI_STEERING_TARGET_CONST_DISTANCE = 1.0f;
		const float AI_STEERING_TARGET_DISTANCE_A = 1.0f;
		const float AI_STEERING_TARGET_DISTANCE_B = 5.0f;

		const float AI_BRAKE_TARGET_CONST_DISTANCE = -1.0f;

		const float AI_BRAKE_TARGET_DISTANCE_A = 2.0f;
		const float AI_BRAKE_TARGET_DISTANCE_B = 4.5f;

		const float	AI_BRAKE_SPEED_DISTANCE_FACTOR_A = 0.6f;
		const float	AI_BRAKE_SPEED_DISTANCE_FACTOR_B = 1.0f;
		
		const float AI_STEERING_PATH_CORRECTION_DISTANCE = 8.0f;
		const float AI_SEGMENT_STEERING_CORRECTION_RADIUS = 4.0f; // distance to segment
		const float AI_BRAKE_PATH_CORRECTION_CURVE = 0.5f;

		// brake curve and limits
		const float AI_BRAKE_CURVE = 0.65f;
		const float AI_BRAKE_LIMIT = 1.0f;

		float steeringBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 40.0f, 0.1f, 5.0f);
		float brakeBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 40.0f, 0.5f, 2.8f);

		const float AI_STEERING_BY_SPEED_CURVE = 0.9f;
		steeringBySpeedModifier = pow(steeringBySpeedModifier, AI_STEERING_BY_SPEED_CURVE);

		//float correctionBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.0f, 0.5f);

		int pathIdx = m_pathPointIdx;

		// steering target position is based on the middle point of the A and B
		Vector3D positionA = GetAdvancedPointByDist(m_pathPointIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_A*steeringBySpeedModifier);
		Vector3D positionB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_B*steeringBySpeedModifier);

		float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 4.0f, 15.0f);

		// trace forward from car using speed
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, carPos+carForward*forwardTraceDistanceBySpeed, steeringTargetColl,
			traceContents,
			&collFilter);

		if(steeringTargetColl.fract < 1.0f)
		{
			float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 3.5f;

			float offsetAmount = 1.0f - steeringTargetColl.fract;

			positionB += steeringTargetColl.normal * offsetAmount * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;

			if(ai_debug_navigator.GetBool())
			{
				debugoverlay->Line3D(carPos, steeringTargetColl.position, ColorRGBA(0, 1, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
				debugoverlay->Sphere3D(steeringTargetColl.position, traceShapeRadius, ColorRGBA(0, 1, 0, 1.0f), fDt);
			}
		}

		// trace from position A to position B
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			positionA, positionB, pathColl,
			traceContents,
			&collFilter);

		if(pathColl.fract < 1.0f)
		{
			float AI_OBSTACLE_PATH_CORRECTION_AMOUNT = 1.1f;
			float AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT = 1.5f;

			Vector3D offsetAmount = pathColl.normal * (traceShapeRadius * AI_OBSTACLE_PATH_CORRECTION_AMOUNT + lowSpeedFactor*AI_OBSTACLE_PATH_LOWSPEED_CORRECTION_AMOUNT);

			positionB += offsetAmount;

			if(ai_debug_navigator.GetBool())
			{
				debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 1, 1.0f), ColorRGBA(1, 0, 1, 1.0f), fDt);
			}
		}

		// calculate steering angle for applying brakes
		pathIdx = m_pathPointIdx;
		Vector3D brakePointA = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_A * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR_A);

		pathIdx = m_pathPointIdx;
		Vector3D brakePointB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_B * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR_B);

		pathIdx = m_pathPointIdx;
		Vector3D pathCorrectionPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_PATH_CORRECTION_DISTANCE);

		// calculate steering. correctionSteeringDir used when car is too far from segment point
		Vector3D correctionSteeringDir = fastNormalize(pathCorrectionPoint - carPos);
		Vector3D pathSteeringDir = fastNormalize(positionB - positionA);

		float pathCorrectionFactor = RemapValClamp(distFromSegmentByCarPos, 0.0f, AI_SEGMENT_STEERING_CORRECTION_RADIUS, 0.0f, 1.0f);

		// steering
		Vector3D steeringDir = lerp(pathSteeringDir, correctionSteeringDir, pathCorrectionFactor);

		// brake
		Vector3D brakeDir = lerp(fastNormalize(brakePointB - brakePointA), correctionSteeringDir, pow(pathCorrectionFactor, AI_BRAKE_PATH_CORRECTION_CURVE));

		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		Vector3D carMoveDir = fastNormalize(carVelocity);

		// make a test for complex steering
		float complexSteeringFactor = 0.0f;
		
		{
			const float AI_COMPLEX_STEERING_ANGLE_POW = 0.7f;
			const float AI_COMPLEX_STEERING_DISTANCE_MIN = 2.0f;
			const float AI_COMPLEX_STEERING_DISTANCE_SCALE = 0.15f;

			const float AI_COMPLEX_STEERING_MIN = 0.25f;

			const float AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE = 35.0f;	// meters away on path from car position

			float complexSteeringDistanceModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.1f, 1.0f);

			pathIdx = m_pathPointIdx;
			Vector3D complexSteeringFinalPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + complexSteeringDistanceModifier*AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE);

			debugoverlay->Box3D(complexSteeringFinalPoint - 0.25f, complexSteeringFinalPoint + 0.25f, ColorRGBA(0, 0, 1, 1.0f), fDt);

			Vector3D prevSegmentDir = carMoveDir;
			
			for(int i = segmentByCarPosition+1; i < pathIdx; i++)
			{
				// the path segment by m_pathPointIdx
				Vector3D segmentStart = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[i-1]);
				Vector3D segmentEnd = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[i]);

				Vector3D segmentDir = fastNormalize(segmentEnd-segmentStart);

				float distanceBetweenSeg = length(segmentEnd-segmentStart)-AI_COMPLEX_STEERING_DISTANCE_MIN;
				distanceBetweenSeg = max(distanceBetweenSeg, 0.0f);

				float cornerAngle = 1.0f - fabs(dot(segmentDir, prevSegmentDir));

				float cornerFactor = pow(cornerAngle, AI_COMPLEX_STEERING_ANGLE_POW) * distanceBetweenSeg*AI_COMPLEX_STEERING_DISTANCE_SCALE;

				complexSteeringFactor += cornerFactor;

				prevSegmentDir = segmentDir;
			}

			if(ai_debug_navigator.GetBool())
				debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path complexness: %g", complexSteeringFactor);

			// if not so complex, handle as usual
			if(complexSteeringFactor < AI_COMPLEX_STEERING_MIN)
				complexSteeringFactor = 0.0f;
		}
		
		bool isBrakePointBehindMe = frontBackCheckPlane.ClassifyPointEpsilon(brakePointA, -car->m_conf->physics.body_size.z) == CP_BACK;
		bool isTargetBehindMe = frontBackCheckPlane.ClassifyPointEpsilon(positionA, -car->m_conf->physics.body_size.z) == CP_BACK;
		
		if(isTargetBehindMe && !isBrakePointBehindMe)
		{
			steeringDir = brakeDir;
			relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);
		}

		const float AI_BRAKE_ACCEL_PRESSING_CURVE = 0.75f;
		const float AI_MIN_BRAKE_LEVEL = 0.6f;

		float brakeFactor = powf(1.0f - fabs(dot(brakeDir, carForward)), AI_BRAKE_CURVE*weatherBrakePow);
		brakeFactor += complexSteeringFactor;

		brakeFactor = min(brakeFactor, AI_BRAKE_LIMIT) * lowSpeedFactor;

		if(brakeFactor > AI_MIN_BRAKE_LEVEL)
			handling.braking = brakeFactor;
		else
			handling.braking = 0.0f;

		handling.acceleration = 1.0f - pow(brakeFactor, AI_BRAKE_ACCEL_PRESSING_CURVE);

		handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z);// * (isTargetBehindMe && isBrakePointBehindMe ? -1.0f : 1.0f);

		handling.steering = clamp(handling.steering, -1.0f, 1.0f);

		if(ai_debug_navigator.GetBool())
		{
			debugoverlay->Box3D(pathCorrectionPoint-0.5f, pathCorrectionPoint+Vector3D(0.0f,1.0f,0.0f)+0.5f, ColorRGBA(1, 0, 1, 1.0f), fDt);

			debugoverlay->Box3D(positionA - 0.25f, positionA + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Box3D(positionB - 0.25f, positionB + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);

			debugoverlay->Box3D(brakePointA - 0.35f, brakePointA + 0.35f, ColorRGBA(1, 0, 1, 1.0f), fDt);
			debugoverlay->Line3D(brakePointA, brakePointB, ColorRGBA(1, 0, 1, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Box3D(brakePointB - 0.35f, brakePointB + 0.35f, ColorRGBA(1, 0, 1, 1.0f), fDt);
		}
	}
}