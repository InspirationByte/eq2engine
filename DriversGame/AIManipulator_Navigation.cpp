//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_Navigation.h"
#include "car.h"

#include "world.h"


const float AI_PATH_TIME_TO_UPDATE = 5.0f;			// every 5 seconds
const float AI_PATH_TIME_TO_UPDATE_FORCE = 0.1f;

const float AI_PATH_COMPLETION_TO_FASTER_SEARCH = 0.35f;	// percent
const float AI_PATH_COMPLETION_TO_FULL_RENEW = 0.5f;		// percent

const float AI_SEGMENT_VALID_RADIUS = 2.0f; // distance to segment

const int AI_PATH_SEARCH_ITERATIONS = 2048;
const int AI_PATH_SEARCH_DETAILED_MAX_RADIUS = 100.0f;

ConVar ai_debug_navigator("ai_debug_navigator", "0");
ConVar ai_debug_path("ai_debug_path", "0");
ConVar ai_debug_search("ai_debug_search", "0");

void DebugDrawPath(pathFindResult3D_t& path, float time = -1.0f)
{
	if(ai_debug_path.GetBool() && path.points.numElem())
	{
		Vector3D lastLinePos = path.points[0].xyz();

		for (int i = 1; i < path.points.numElem(); i++)
		{
			const Vector3D& pointPos = path.points[i].xyz();

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

void pathFindResult3D_t::InitFrom(pathFindResult_t& path, CEqCollisionObject* ignore)
{
	g_pGameWorld->m_level.m_navGridSelector = path.gridSelector;

	start = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.start);
	end = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.end);

	points.clear();

	btSphereShape _sphere(2.5f);
	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(ignore);

	CollisionData_t coll;

	Vector3D prevPoint = start;

	for(int i = 0; i < path.points.numElem(); i++)
	{
		Vector4D point(g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[i]), 0.0f);

		if (g_pPhysics->TestConvexSweep(&_sphere, identity(), prevPoint, point.xyz(), coll, OBJECTCONTENTS_SOLID_OBJECTS, &collFilter))
		{
			point = Vector4D(point.xyz() + coll.normal * 2.0f, 0.0f);
		}

		// store narowness factor
		point.w = coll.fract;

		points.append(point);

		prevPoint = point.xyz();
	}

	g_pGameWorld->m_level.m_navGridSelector = 0;
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

void CAINavigationManipulator::SetPath(pathFindResult_t& newPath, const Vector3D& searchPos, CCar* ignoreObj)
{
	SimplifyPath(newPath);

	m_path.InitFrom(newPath, ignoreObj->GetPhysicsBody());

	m_pathPointIdx = 0;
	m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE;

	m_lastSuccessfulSearchPos = searchPos;
	m_searchFails = 0;

	DebugDrawPath(m_path);

	m_init = true;
}

Vector3D CAINavigationManipulator::GetPoint(int startSeg, float distFromSegment, int& outSeg, Vector3D& outDir, float& outSegLen, float& outDistFromSegment)
{
	Vector3D currPointPos;
	Vector3D nextPointPos;

	outSeg = startSeg;
	outDistFromSegment = distFromSegment;

	if (m_path.points.numElem() == 0)
		return 0.0f;

	while (outSeg < m_path.points.numElem() - 2)
	{
		currPointPos = m_path.points[outSeg].xyz();
		nextPointPos = m_path.points[outSeg + 1].xyz();

		float segLen = length(currPointPos - nextPointPos);

		if (outDistFromSegment > segLen)
		{
			outDistFromSegment -= segLen;
			outSeg++;
		}
		else
			break;
	}

	currPointPos = m_path.points[outSeg].xyz();
	nextPointPos = m_path.points[outSeg + 1].xyz();

	outDir = fastNormalize(nextPointPos - currPointPos);
	outSegLen = length(currPointPos - nextPointPos);

	return currPointPos;
}

Vector3D CAINavigationManipulator::GetAdvancedPointByDist(int& startSeg, float distFromSegment)
{
	Vector3D dir(0.0f);
	float segLen = 0.0f;

	Vector3D point = GetPoint(startSeg, distFromSegment, startSeg, dir, segLen, distFromSegment);

	return point + dir*distFromSegment;
}

float CAINavigationManipulator::GetNarrownessAt(const Vector3D& pos, float distToSegment)
{
	int segIdx = FindSegmentByPosition(pos, distToSegment);

	if (segIdx == -1)
		return -1;

	float narr1 = m_path.points.inRange(segIdx - 1) ? m_path.points[segIdx - 1].w : 1.0f;
	float narr2 = m_path.points.inRange(segIdx + 1) ? m_path.points[segIdx + 1].w : 1.0f;

	return narr1 * m_path.points[segIdx].w * narr2;
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
		currPointPos = m_path.points[segment].xyz();
		nextPointPos = m_path.points[segment + 1].xyz();

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
		currPointPos = m_path.points[segment].xyz();
		nextPointPos = m_path.points[segment + 1].xyz();

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

void CAINavigationManipulator::ForceUpdatePath()
{
	m_timeToUpdatePath = 0.0f;
}

void CAINavigationManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	Vector3D carPos = car->GetOrigin();
	Vector3D carBodySize = car->m_conf->physics.body_size;

	const Vector3D& carVelocity = car->GetVelocity();
	
	// use vehicle world matrix
	const Matrix4x4& bodyMat = car->m_worldMatrix;

	Vector3D carForward = car->GetForwardVector();
	Vector3D carRight = car->GetRightVector();

	Plane frontBackCheckPlane(carForward, -dot(carForward, carPos));

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;

	float brakeDistancePerSec = car->m_conf->GetBrakeEffectPerSecond()*0.5f;
	float brakeToStopTime = speedMPS / brakeDistancePerSec*2.0f;				// time to stop from the current speed
	float brakeDistAtCurSpeed = brakeDistancePerSec*brakeToStopTime;			// the brake distance in one second

	// search on timeout active here
	//if(!doesHaveStraightPath)
	{
		//
		// Get the last point on the path
		//
		int currSeg = FindSegmentByPosition(carPos, AI_SEGMENT_VALID_RADIUS);
		Vector3D pathTarget = (currSeg != -1) ? m_path.points[currSeg].xyz() : m_driveTarget;

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

		ubyte targetNavPoint = g_pGameWorld->m_level.Nav_GetTileAtGlobalPoint( g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(m_driveTarget) );

		if(targetNavPoint > 0)
			m_targetSuccessfulSearchPos = m_driveTarget;

		if( m_timeToUpdatePath <= 0.0f)
		{
			Vector3D searchStart(carPos);

			int trials = m_searchFails;

			// try search from last successfull search point to not get stuck
			if(m_searchFails > 0)
				searchStart = m_lastSuccessfulSearchPos;

			// if we're too far, search using fast grid
			bool fastSearch = (length(m_targetSuccessfulSearchPos - searchStart) > AI_PATH_SEARCH_DETAILED_MAX_RADIUS);

			if(fastSearch) // change the grid selector
				g_pGameWorld->m_level.m_navGridSelector = 1;

			pathFindResult_t newPath;
			if( g_pGameWorld->m_level.Nav_FindPath(m_targetSuccessfulSearchPos, searchStart, newPath, AI_PATH_SEARCH_ITERATIONS, fastSearch))
			{
				SetPath(newPath, carPos, car);
			}
			else
			{
				m_searchFails++;

				if(m_searchFails > 1)
					m_searchFails = 0;

				m_timeToUpdatePath = AI_PATH_TIME_TO_UPDATE_FORCE;
			}

			// reset grid selectr
			g_pGameWorld->m_level.m_navGridSelector = 0;

			if(ai_debug_search.GetBool())
				debugoverlay->TextFadeOut(0, color4_white, 10.0f, "path search result: %d points, tries: %d", newPath.points.numElem(), trials);
		}
	}

	if(!m_init)
	{
		memset(&handling, 0, sizeof(handling));
		return;
	}

	//
	// Introduce handling basics
	//

	float tireFrictionModifier = 0.0f;
	{
		int numWheels = car->GetWheelCount();

		for (int i = 0; i < numWheels; i++)
		{
			CCarWheel& wheel = car->GetWheel(i);

			if (wheel.GetSurfaceParams())
				tireFrictionModifier += wheel.GetSurfaceParams()->tirefriction;
		}

		tireFrictionModifier /= float(numWheels);
	}

	float weatherBrakeDistModifier = s_weatherBrakeDistanceModifier[g_pGameWorld->m_envConfig.weatherType] * (1.0f-tireFrictionModifier);
	float weatherBrakePow = s_weatherBrakeCurve[g_pGameWorld->m_envConfig.weatherType] * tireFrictionModifier;

	const float AI_LOWSPEED_CURVE		= 6.5f;
	const float AI_LOWSPEED_END			= 8.0f;	// 8 meters per second

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_CURVE);

	// forcing auto handbrake on low speeds
	if(lowSpeedFactor < 0.1f)
		handling.autoHandbrake = true;

	//handling.autoHandbrake = true;

	const float AI_OBSTACLE_SPHERE_RADIUS = carBodySize.x*2.0f;
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

	if(m_path.points.numElem() > 0 && m_pathPointIdx != -1 && m_pathPointIdx < m_path.points.numElem()-2)
	{
		int segmentByCarPosition = FindSegmentByPosition(carPos, AI_SEGMENT_VALID_RADIUS);
		
		// segment must be corrected
		if(segmentByCarPosition > m_pathPointIdx)
			m_pathPointIdx = segmentByCarPosition;

		if(segmentByCarPosition == -1)
			segmentByCarPosition = m_pathPointIdx;

		// the path segment by m_pathPointIdx
		const Vector4D& pathSegmentStart = m_path.points[m_pathPointIdx];
		const Vector4D& pathSegmentEnd = m_path.points[m_pathPointIdx + 1];

		float pathSegmentLength = length(pathSegmentStart - pathSegmentEnd);
		float pathSegmentProj = lineProjection(pathSegmentStart.xyz(), pathSegmentEnd.xyz(), carPos);

		// the path segment by segmentByCarPosition
		const Vector4D& carPosSegmentStart = m_path.points[segmentByCarPosition];
		const Vector4D& carPosSegmentEnd = m_path.points[segmentByCarPosition + 1];

		float carPosSegmentLength = length(carPosSegmentStart - carPosSegmentEnd);
		float carPosSegmentProj = lineProjection(carPosSegmentStart.xyz(), carPosSegmentEnd.xyz(), carPos);

		// calculate distance between car position and point on the segment by car position
		// to 
		Vector3D carPosOnSegmentByCarPos = lerp(carPosSegmentStart.xyz(), carPosSegmentEnd.xyz(), carPosSegmentProj);
		float distFromSegmentByCarPos = length(carPosOnSegmentByCarPos - carPos);

		// distance from path segment start
		float distFromPathSegStart = pathSegmentProj * pathSegmentLength;

		debugoverlay->Box3D(carPosOnSegmentByCarPos - 0.5f, carPosOnSegmentByCarPos + 0.5f, ColorRGBA(1, 0, 1, 1.0f), fDt);

		const float AI_STEERING_TARGET_CONST_DISTANCE = 0.5f;

		const float AI_STEERING_TARGET_DISTANCE_A = 0.5f;
		const float AI_STEERING_TARGET_DISTANCE_B = 0.6f;

		const float AI_BRAKE_TARGET_CONST_DISTANCE = 0.5f;

		const float AI_BRAKE_TARGET_DISTANCE_A = 1.2f;
		const float AI_BRAKE_TARGET_DISTANCE_B = 1.25f;

		const float	AI_BRAKE_SPEED_DISTANCE_FACTOR_A = 0.9f;
		const float	AI_BRAKE_SPEED_DISTANCE_FACTOR_B = 1.2f;
		
		const float AI_STEERING_PATH_CORRECTION_DISTANCE = 9.0f;
		const float AI_SEGMENT_STEERING_CORRECTION_RADIUS = 2.5f; // distance to segment
		const float AI_BRAKE_PATH_CORRECTION_CURVE = 1.25f;

		// brake curve and limits
		const float AI_BRAKE_CURVE = 0.65f;
		const float AI_BRAKE_LIMIT = 0.9f;

		float steeringBySpeedModifier = RemapValClamp(speedMPS, 2.0f, 20.0f, 0.5f, 5.0f);
		float brakeBySpeedModifier = RemapValClamp(speedMPS, 2.0f, 20.0f, 0.0f, 4.0f);

		const float AI_STEERING_BY_SPEED_CURVE = 1.8f;
		steeringBySpeedModifier = pow(steeringBySpeedModifier, AI_STEERING_BY_SPEED_CURVE);

		//float correctionBySpeedModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.0f, 0.5f);

		int pathIdx = segmentByCarPosition;

		// steering target position is based on the middle point of the A and B
		Vector3D positionA = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_A*steeringBySpeedModifier);

		pathIdx = segmentByCarPosition;
		Vector3D positionB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_TARGET_CONST_DISTANCE + AI_STEERING_TARGET_DISTANCE_B*steeringBySpeedModifier);

		float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 6.0f, 25.0f);

		// calculate steering angle for applying brakes
		pathIdx = segmentByCarPosition;

		Vector3D brakePointADir(0.0f);
		float brakePointASegLen = 0.0f;
		float brakePointADistFromSegment = distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_A * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR_A;

		Vector3D brakePointSeg = GetPoint(pathIdx, 
			brakePointADistFromSegment,
			pathIdx, brakePointADir, brakePointASegLen, brakePointADistFromSegment);

		Vector3D brakePointA = brakePointSeg + brakePointADir * brakePointADistFromSegment;

		pathIdx = segmentByCarPosition;
		Vector3D brakePointB = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_BRAKE_TARGET_CONST_DISTANCE + AI_BRAKE_TARGET_DISTANCE_B * brakeBySpeedModifier + brakeDistAtCurSpeed * weatherBrakeDistModifier * AI_BRAKE_SPEED_DISTANCE_FACTOR_B);

		pathIdx = segmentByCarPosition;
		Vector3D pathCorrectionPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_PATH_CORRECTION_DISTANCE);

		float pathNarrownessFactor = 1.0f - GetNarrownessAt(positionA, AI_STEERING_PATH_CORRECTION_DISTANCE);

		//debugoverlay->TextFadeOut(0, ColorRGBA(1, 1, 0, 1), 10.0f, "pathNarrownessFactor: %.2f\n", pathNarrownessFactor);

		// calculate steering. correctionSteeringDir used when car is too far from segment point
		Vector3D correctionSteeringDir = fastNormalize(pathCorrectionPoint - carPos);
		Vector3D pathSteeringDir = fastNormalize(positionB - positionA);

		float steeringDirVsCorrection = pow(1.0f - fabs(dot(pathSteeringDir, correctionSteeringDir)), 0.25f);

		float pathCorrectionFactor = RemapValClamp(distFromSegmentByCarPos, 0.0f, AI_SEGMENT_STEERING_CORRECTION_RADIUS, 0.0f, 1.0f);

		// steering
		Vector3D steeringDir = lerp(pathSteeringDir, correctionSteeringDir, clamp(pathCorrectionFactor * steeringDirVsCorrection + pathNarrownessFactor, 0.0f, 1.0f));

		// trace forward from car using speed
		g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
			carPos, carPos + carForward * forwardTraceDistanceBySpeed, steeringTargetColl,
			traceContents,
			&collFilter);

		if (steeringTargetColl.fract < 1.0f)
		{
			float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 1.5f;

			Vector3D collPoint(steeringTargetColl.position);

			Plane dPlane(carRight, -dot(carPos, carRight));
			float posSteerFactor = -dPlane.Distance(collPoint);

			Vector3D collPointToSteeringDir(positionA - collPoint);

			float correctionFactor = saturate(dot(pathSteeringDir, collPointToSteeringDir));

			steeringDir += correctionFactor * carRight * posSteerFactor * (1.0f - steeringTargetColl.fract) * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;
			steeringDir = normalize(steeringDir);

			debugoverlay->Sphere3D(lerp(carPos, carPos + carForward * forwardTraceDistanceBySpeed, steeringTargetColl.fract), traceShapeRadius, ColorRGBA(1, 1, 0, 1.0f), fDt);
		}

		const float AI_BRAKE_SEGMENTLENGTH_FACTOR = AI_BRAKE_TARGET_DISTANCE_A*brakeBySpeedModifier + AI_BRAKE_TARGET_DISTANCE_B*brakeBySpeedModifier;

		// brake
		Vector3D brakeDir = lerp(fastNormalize(brakePointB - brakePointA + brakePointADir*brakePointASegLen * AI_BRAKE_SEGMENTLENGTH_FACTOR), correctionSteeringDir, pow(pathCorrectionFactor, AI_BRAKE_PATH_CORRECTION_CURVE) * steeringDirVsCorrection);

		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		Vector3D carMoveDir = fastNormalize(carVelocity);

		// make a test for complex steering
		float complexSteeringFactor = 0.0f;
		
		{
			const float AI_COMPLEX_STEERING_ANGLE_POW = 0.7f;
			const float AI_COMPLEX_STEERING_DISTANCE_MIN = 2.0f;
			const float AI_COMPLEX_STEERING_DISTANCE_SCALE = 0.25f;

			const float AI_COMPLEX_STEERING_MIN = 0.25f;

			const float AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE = 20.0f;	// meters away on path from car position

			float complexSteeringDistanceModifier = RemapValClamp(speedMPS, 0.0f, 30.0f, 0.1f, 1.0f);

			pathIdx = m_pathPointIdx;
			Vector3D complexSteeringFinalPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + complexSteeringDistanceModifier*AI_COMPLEX_STEERING_PROVIDENCE_DISTANCE);

			debugoverlay->Box3D(complexSteeringFinalPoint - 0.25f, complexSteeringFinalPoint + 0.25f, ColorRGBA(0, 0, 1, 1.0f), fDt);

			Vector3D prevSegmentDir = carMoveDir;
			
			for(int i = segmentByCarPosition+1; i < pathIdx; i++)
			{
				// the path segment by m_pathPointIdx
				const Vector3D& segmentStart = m_path.points[i-1].xyz();
				const Vector3D& segmentEnd = m_path.points[i].xyz();

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

		const float AI_BRAKE_ACCEL_PRESSING_CURVE = 0.85f;
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
		handling.acceleration -= complexSteeringFactor * lowSpeedFactor;

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