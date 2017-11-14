//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_Navigation.h"
#include "car.h"

const float AI_STRAIGHT_PATH_MAX_DISTANCE = 40.0f;
const float AI_MAX_DISTANCE_TO_PATH = 10.0f;

const float AI_PATH_TIME_TO_UPDATE = 5.0f;			// every 5 seconds
const float AI_PATH_TIME_TO_UPDATE_FORCE = 0.25f;

const float AI_SEGMENT_VALID_RADIUS = 1.0f; // distance to segment
const float AI_SEGMENT_STEERING_CORRECTION_RADIUS = 3.0f; // distance to segment

const int AI_PATH_SEARCH_RANGE = 4096;

ConVar ai_debug_navigator("ai_debug_navigator", "0");
ConVar ai_debug_path("ai_debug_path", "0");
ConVar ai_debug_search("ai_debug_search", "0");

static float s_weatherBrakeDistanceModifier[WEATHER_COUNT] =
{
	0.48f,
	0.56f,
	0.58f
};

static float s_weatherBrakeCurve[WEATHER_COUNT] =
{
	0.92f,
	0.85f,
	0.84f
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

	IVector2D lastTracePointPos = path.points[0];

	for (int i = 2; i < path.points.numElem()-1; i++)
	{
		IVector2D pointPos = path.points[i];

		IVector2D dir2D(pointPos-lastPointPos);

		// process conditions
		bool isStraight = lastDir2D == dir2D;
		bool hasCollision = g_pGameWorld->m_level.Nav_TestLine2D(lastPointPos, pointPos) < 0.8f;

		// store last data
		lastPointPos = pointPos;
		lastDir2D = dir2D;

		if(!(isStraight || hasCollision))
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

	// push drive target to the path to back
	IVector2D targetPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(m_driveTarget);
	m_path.points.append(targetPoint);

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

	m_path.points.removeIndex(m_path.points.numElem() - 1);

	// push drive target to the path to back
	IVector2D targetPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(m_driveTarget);
	m_path.points.append(targetPoint);

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
	distFromSegment = clamp(distFromSegment, 0.0f, lastSegLen);

	return currPointPos + dir*distFromSegment;
}

int CAINavigationManipulator::FindSegmentByPosition(const Vector3D& pos, float distToSegment)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	int segment = 0;

	m_path.points.removeIndex(m_path.points.numElem() - 1);

	// push drive target to the path to back
	IVector2D targetPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(m_driveTarget);
	m_path.points.append(targetPoint);

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

void CAINavigationManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	const Vector3D& carPos = car->GetOrigin();
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
		int lastPoint = m_pathPointIdx;
		Vector3D pathTarget = (lastPoint != -1) ? g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[lastPoint]) : m_driveTarget;

		float pathCompletionPercentage = (float)lastPoint / (float)m_path.points.numElem();

		if(length(pathTarget-carPos) > AI_MAX_DISTANCE_TO_PATH)
			pathCompletionPercentage = 1.0f;

		// if we have old path, try continue moving
		if(pathCompletionPercentage < 0.5f && m_path.points.numElem() > 1)
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
			debugoverlay->TextFadeOut(0, color4_white, 10.0f, "nextPathUpdateTime: %g", m_timeToUpdatePath);
		}

		if( m_timeToUpdatePath <= 0.0f)
		{
			Vector3D searchStart(carPos);

			int trials = m_searchFails;

			// try search from last successfull search point to not get stuck
			if(m_searchFails > 0)
				searchStart = m_lastSuccessfulSearchPos;

			pathFindResult_t newPath;
			if( g_pGameWorld->m_level.Nav_FindPath(m_driveTarget, searchStart, newPath, AI_PATH_SEARCH_RANGE, false))
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

	const float AI_LOWSPEED_BRAKE_CURVE = 15.0f;
	const float AI_LOWSPEED_END			= 8.0f;	// 8 meters per second

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_BRAKE_CURVE);

	// forcing auto handbrake on low speeds
	if(lowSpeedFactor < 0.1f)
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
		m_timeToUpdatePath = 0.0f;

		const float AI_CHASE_TARGET_VELOCITY_SCALE = 0.6f;

		// brake curve
		const float AI_CHASE_BRAKE_POW_FACTOR = 0.35f;

		// add velocity offset
		Vector3D steeringTargetPos = m_driveTarget + m_driveTargetVelocity * AI_CHASE_TARGET_VELOCITY_SCALE;
		/*
		// trace the car body in velocity direction
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, steeringTargetPos, steeringTargetColl,
			traceContents,
			&collFilter);

		// correct the steering to prevent car damage if we moving towards obstacle
		if(steeringTargetColl.fract < 1.0f)
		{
			float distanceScale = (1.0f - pow(steeringTargetColl.fract, AI_OBSTACLE_CORRECTION_CURVE)) * AI_OBSTACLE_CORRECTION_SPEED_SCALE;

			float newPositionDist = distanceScale * AI_OBSTACLE_CORRECTION_CONST_DISTANCE + speedMPS * distanceScale;

			Vector3D newSteeringTargetPos = steeringTargetColl.position + steeringTargetColl.normal * newPositionDist; // 4 meters is enough to be safe

			Vector3D checkSteeringDir = fastNormalize(steeringTargetPos - carPos);

			if(fabs(dot(checkSteeringDir, carForward)) > 0.5f)
			{
				steeringTargetPos = newSteeringTargetPos;

				if(ai_debug_navigator.GetBool())
				{
					debugoverlay->Box3D(steeringTargetColl.position - traceShapeRadius, steeringTargetColl.position + traceShapeRadius, ColorRGBA(1, 0, 0, 1.0f), fDt);
					debugoverlay->Line3D(steeringTargetColl.position, steeringTargetPos, ColorRGBA(1, 1, 0, 1.0f), ColorRGBA(1, 1, 0, 1.0f), fDt);
					debugoverlay->Box3D(steeringTargetPos - traceShapeRadius, steeringTargetPos + traceShapeRadius, ColorRGBA(0, 1, 0, 1.0f), fDt);
				}
			}
		}*/
		
		// calculate steering
		Vector3D steeringDir = fastNormalize(steeringTargetPos - carPos);
		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z);
		handling.steering = clamp(handling.steering, -1.0f, 1.0f);

		// calculate brake
		float brakeFac = (1.0f-fabs(dot(steeringDir, carForward)));

		handling.braking = pow(brakeFac, AI_CHASE_BRAKE_POW_FACTOR*weatherBrakePow) * lowSpeedFactor;
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

		const float AI_STEERING_TARGET_CONST_DISTANCE = 2.0f;
		const float AI_STEERING_TARGET_DISTANCE_A = 2.0f;
		const float AI_STEERING_TARGET_DISTANCE_B = 4.5f;

		const float AI_BRAKE_TARGET_CONST_DISTANCE = -1.0f;

		const float AI_BRAKE_TARGET_DISTANCE_A = 0.0f;
		const float AI_BRAKE_TARGET_DISTANCE_B = 3.0f;

		const float	AI_BRAKE_SPEED_DISTANCE_FACTOR = 0.9f;
		
		const float AI_STEERING_PATH_CORRECTION_DISTANCE = 7.0f;

		// brake curve and limits
		const float AI_BRAKE_POW_FACTOR = 0.75f;
		const float AI_BRAKE_LIMIT = 0.9f;

		float steeringBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 50.0f, 1.0f, 4.0f);
		float brakeBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.5f, 1.0f);

		const float AI_STEERING_BY_SPEED_CURVE = 0.8f;
		steeringBySpeedModifier = pow(steeringBySpeedModifier, AI_STEERING_BY_SPEED_CURVE);

		//float correctionBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.0f, 0.5f);

		int pathIdx = m_pathPointIdx;

		// steering target position is based on the middle point of the A and B
		Vector3D positionA = GetAdvancedPointByDist(m_pathPointIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_A*steeringBySpeedModifier);
		Vector3D positionB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_B*steeringBySpeedModifier);

		// correct the position A and position B
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			positionA, positionB, pathColl,
			traceContents,
			&collFilter);

		// correct the position A and position B
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, positionA, steeringTargetColl,
			traceContents,
			&collFilter);

		if(steeringTargetColl.fract < 1.0f)
		{
			float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 0.5f;

			positionB += steeringTargetColl.normal*steeringTargetColl.fract*AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;
		}

		if(pathColl.fract < 1.0f)
		{
			float AI_OBSTACLE_PATH_CORRECTION_AMOUNT = 1.1f;

			Vector3D offsetAmount = pathColl.normal * traceShapeRadius * AI_OBSTACLE_PATH_CORRECTION_AMOUNT;

			positionB += offsetAmount;

			if(ai_debug_navigator.GetBool())
			{
				debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 1, 1.0f), ColorRGBA(1, 0, 1, 1.0f), fDt);
			}
		}

		// calculate steering angle for applying brakes
		pathIdx = m_pathPointIdx;
		Vector3D brakePointA = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_A * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR);

		pathIdx = m_pathPointIdx;
		Vector3D brakePointB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_B * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR);

		pathIdx = m_pathPointIdx;
		Vector3D pathCorrectionPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_PATH_CORRECTION_DISTANCE);

		// brake
		Vector3D brakeDir = fastNormalize(brakePointB - brakePointA);

		// calculate steering. correctionSteeringDir used when car is too far from segment point
		Vector3D correctionSteeringDir = fastNormalize(pathCorrectionPoint - carPos);
		Vector3D pathSteeringDir = fastNormalize(positionB - positionA);

		Vector3D steeringDir = lerp(pathSteeringDir, correctionSteeringDir, RemapValClamp(distFromSegmentByCarPos, 0.0f, AI_SEGMENT_STEERING_CORRECTION_RADIUS, 0.0f, 1.0f));


		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		Vector3D carMoveDir = fastNormalize(carVelocity);
		
		const float AI_COMPLEX_STEERING_TURN_COUNT = 1;

		const float AI_COMPLEX_STEERING_SCALING = 1.0f / (float)AI_COMPLEX_STEERING_TURN_COUNT;
		const float AI_COMPLEX_STEERING_THRESH	= 0.15f;	// more threshold - more lighter steering to be
		const float AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE = 40.0f;	// 40 meters away
		const float AI_COMPLEX_STEERING_MIN = 0.10f;
		const float AI_COMPLEX_STEERING_CURVE = 0.35f;
		const float AI_COMPLEX_STEERING_DISTANCEFACTOR = 0.1f;

		// make a test for complex steering
		float complexSteeringFactor = 0.0f;
		/*
		{
			float complexSteeringDistanceModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.1f, 1.0f);

			pathIdx = m_pathPointIdx;
			Vector3D complexSteeringFinalPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + complexSteeringDistanceModifier*AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE);

			debugoverlay->Box3D(complexSteeringFinalPoint - 0.58f, complexSteeringFinalPoint + 0.58f, ColorRGBA(0, 0, 1, 1.0f), fDt);

			Vector3D prevSegmentDir = carMoveDir;
			
			for(int i = segmentByCarPosition+1; i < pathIdx; i++)
			{
				// the path segment by m_pathPointIdx
				Vector3D segmentStart = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[i-1]);
				Vector3D segmentEnd = g_pGameWorld->m_level.Nav_GlobalPointToPosition(m_path.points[i]);

				float distanceBetweenSeg = length(segmentEnd-segmentStart);

				Vector3D segmentDir = fastNormalize(segmentEnd-segmentStart);

				float corner = (1.0f - fabs(dot(segmentDir, prevSegmentDir))) * distanceBetweenSeg*AI_COMPLEX_STEERING_DISTANCEFACTOR;

				if(corner > AI_COMPLEX_STEERING_THRESH)
					complexSteeringFactor += corner*AI_COMPLEX_STEERING_SCALING;

				prevSegmentDir = segmentDir;
			}

			if(ai_debug_navigator.GetBool())
				debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path complexness: %g", complexSteeringFactor);

			// if not so complex, handle as usual
			if(complexSteeringFactor < AI_COMPLEX_STEERING_MIN)
				complexSteeringFactor = 0.0f;
		}*/
		
		bool isBrakePointBehindMe = frontBackCheckPlane.ClassifyPointEpsilon(brakePointA, -car->m_conf->physics.body_size.z) == CP_BACK;
		bool isTargetBehindMe = frontBackCheckPlane.ClassifyPointEpsilon(positionA, -car->m_conf->physics.body_size.z) == CP_BACK;
		
		if(isTargetBehindMe && !isBrakePointBehindMe)
		{
			steeringDir = brakeDir;
			relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);
		}

		const float AI_BRAKE_ACCEL_PRESSING_CURVE = 0.85f;
		const float AI_MIN_BRAKE_LEVEL = 0.6f;

		float brakeFactor = powf(1.0f - fabs(dot(brakeDir, carForward)), AI_BRAKE_POW_FACTOR*weatherBrakePow);
		brakeFactor += pow(complexSteeringFactor, AI_COMPLEX_STEERING_CURVE);

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
			debugoverlay->Box3D(positionA - 0.25f, positionA + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Box3D(positionB - 0.25f, positionB + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);

			debugoverlay->Box3D(brakePointA - 0.35f, brakePointA + 0.35f, ColorRGBA(1, 0, 1, 1.0f), fDt);
			debugoverlay->Line3D(brakePointA, brakePointB, ColorRGBA(1, 0, 1, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			debugoverlay->Box3D(brakePointB - 0.35f, brakePointB + 0.35f, ColorRGBA(1, 0, 1, 1.0f), fDt);
		}
	}
}