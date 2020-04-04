//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_Navigation.h"
#include "car.h"
#include "world.h"

#include "heightfield.h"


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
	
	g_pGameWorld->m_level.m_navGridSelector = path.gridSelector;

	IVector2D lastPointPos = path.points[0];
	IVector2D lastDir2D = path.points[1]-lastPointPos;

	short lastHeight = 0;

	for (int i = 2; i < path.points.numElem()-1; i++)
	{
		IVector2D pointPos = path.points[i];
		IVector2D dir2D(pointPos-lastPointPos);

		// check for height
		Vector3D posOn3D = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[i]);

		bool heightChanged = false;

		IVector2D hfieldTile;
		CLevelRegion* reg;
		if (g_pGameWorld->m_level.GetRegionAndTile(posOn3D, &reg, hfieldTile))
		{
			CHeightTileField* hfield = reg->GetHField(0);
			hfieldtile_t* tile = hfield->GetTile(hfieldTile.x, hfieldTile.y);

			if (tile)
			{
				int heightDiff = abs(lastHeight - tile->height);

				heightChanged = (heightDiff > 4);

				if(heightChanged)
					lastHeight = heightDiff;
			}
		}

		// process conditions
		bool isStraight = lastDir2D == dir2D;

		// store last data
		lastPointPos = pointPos;
		lastDir2D = dir2D;

		if (heightChanged)
			continue;

		if (!isStraight)
			continue;

		i--;
		path.points.removeIndex(i);
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

void CAINavigationManipulator::Setup(const Vector3D&driveTargetPos, const Vector3D& targetVelocity, CEqCollisionObject* excludeCollObj)
{
	m_driveTarget = driveTargetPos;
	m_driveTargetVelocity = targetVelocity;
	m_excludeColl = excludeCollObj;
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

float CAINavigationManipulator::GetPointVelocityFactor(int startSeg, const Vector3D& position, const Vector3D& velocity)
{
	if (m_path.points.numElem() == 0)
		return 1.0f;

	float velocityFactor = 1.0f;

	float velLinear = length(velocity);

	while (startSeg < m_path.points.numElem() - 2)
	{
		const Vector3D& currPointPos = m_path.points[startSeg].xyz();
		const Vector3D& nextPointPos = m_path.points[startSeg + 1].xyz();

		Vector3D segVec = nextPointPos-currPointPos;

		float segLen = length(segVec);

		velocityFactor = dot(fastNormalize(segVec), fastNormalize(velocity));

		if (velLinear > segLen)
		{
			velLinear -= segLen;
			startSeg++;
		}
		else
			break;
	}

	return velocityFactor;
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

		float segLen = length(nextPointPos-currPointPos);

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

	Vector3D carFrontPos = carPos + carForward * carBodySize.z;

	Plane frontBackCheckPlane(carForward, -dot(carForward, carPos));

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;
	float speedFromWheels = car->GetSpeedWheels();

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

	const float AI_LOWSPEED_CURVE		= 0.8f;
	const float AI_LOWSPEED_END			= 10.0f;	// 8 meters per second

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_CURVE);

	const float AI_OBSTACLE_SPHERE_RADIUS = carBodySize.x*2.0f;
	const float AI_OBSTACLE_SPHERE_SPEED_SCALE = 0.005f;

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
		Vector3D carPosOnSegmentByCarPos = lerp(carPosSegmentStart.xyz(), carPosSegmentEnd.xyz(), carPosSegmentProj);
		debugoverlay->Box3D(carPosOnSegmentByCarPos - 0.5f, carPosOnSegmentByCarPos + 0.5f, ColorRGBA(1, 0, 1, 1.0f), fDt);

		float distFromSegmentByCarPos = length(carPosOnSegmentByCarPos - carPos);

		// distance from path segment start
		float distFromPathSegStart = pathSegmentProj * pathSegmentLength;

		float pathPosOnSegment = distFromPathSegStart + carBodySize.z*2.0f;
		
		Vector3D steeringDir = carForward;
		Vector3D brakeDir = carForward;

		float pathVelocityFactor = 1.0f - GetPointVelocityFactor(segmentByCarPosition, carPos, carVelocity);

		// brake is computed first
		{
			float brakeDistanceOnSegment = brakeDistAtCurSpeed * 0.45f;

			int pathIdx = segmentByCarPosition;
			Vector3D positionA = GetAdvancedPointByDist(pathIdx, pathPosOnSegment + brakeDistanceOnSegment);

			pathIdx = segmentByCarPosition;
			Vector3D positionB = GetAdvancedPointByDist(pathIdx, pathPosOnSegment + brakeDistanceOnSegment + 1.0f);

			// this should be from car pos
			positionA = lerp(carFrontPos, positionA, RemapValClamp(speedMPS, 0.0f, 20.0f, 0.0f, 1.0f));

			brakeDir = fastNormalize(positionB - positionA);

			//debugoverlay->Box3D(positionA - 0.25f, positionA + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);
			//debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			//debugoverlay->Box3D(positionB - 0.25f, positionB + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);
		};

		// compute steering
		{
			float brakeDistanceOnSegment = brakeDistAtCurSpeed * 0.45f * pathVelocityFactor;

			// steering target position is based on the middle point of the A and B
			int pathIdx = segmentByCarPosition;
			Vector3D positionA = GetAdvancedPointByDist(pathIdx, pathPosOnSegment + brakeDistanceOnSegment);

			pathIdx = segmentByCarPosition;
			Vector3D positionB = GetAdvancedPointByDist(pathIdx, pathPosOnSegment + brakeDistanceOnSegment + 1.0f);

			// this should be from car pos
			positionA = lerp(carFrontPos, positionA, RemapValClamp(speedMPS, 0.0f, 25.0f, 0.0f, 1.0f));

			//debugoverlay->Box3D(positionA - 0.25f, positionA + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);
			//debugoverlay->Line3D(positionA, positionB, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
			//debugoverlay->Box3D(positionB - 0.25f, positionB + 0.25f, ColorRGBA(1, 0, 0, 1.0f), fDt);

			// calculate correction
			const float AI_STEERING_PATH_CORRECTION_DISTANCE = 9.0f;
			const float AI_SEGMENT_STEERING_CORRECTION_RADIUS = 3.0f; // distance to segment
			const float AI_BRAKE_PATH_CORRECTION_CURVE = 1.25f;

			pathIdx = segmentByCarPosition;
			Vector3D pathCorrectionPoint = GetAdvancedPointByDist(pathIdx, distFromPathSegStart + AI_STEERING_PATH_CORRECTION_DISTANCE);
			
			float pathNarrownessFactor = 1.0f - GetNarrownessAt(positionA, AI_STEERING_PATH_CORRECTION_DISTANCE);

			// calculate steering. correctionSteeringDir used when car is too far from segment point
			Vector3D correctionSteeringDir = fastNormalize(pathCorrectionPoint - carPos);
			Vector3D pathSteeringDir = fastNormalize(positionB - positionA);
			
			float steeringDirVsCorrection = pow(1.0f - fabs(dot(pathSteeringDir, correctionSteeringDir)), 0.25f);
			float pathCorrectionFactor = RemapValClamp(distFromSegmentByCarPos, 0.0f, AI_SEGMENT_STEERING_CORRECTION_RADIUS, 0.0f, 1.0f);
			
			float correctionToPath = clamp(pathCorrectionFactor * steeringDirVsCorrection + pathNarrownessFactor, 0.0f, 1.0f);

			// store output steering target
			m_outSteeringTargetPos = positionA;

			// steering
			steeringDir = lerp(pathSteeringDir, correctionSteeringDir, correctionToPath);

			float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 2.0f, 15.0f);

			// trace forward from car using speed
			g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
				carFrontPos, carFrontPos + carForward * forwardTraceDistanceBySpeed, steeringTargetColl,
				traceContents,
				&collFilter);

			if (steeringTargetColl.fract < 1.0f)
			{
				debugoverlay->Line3D(steeringTargetColl.position, carPos, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
				debugoverlay->Sphere3D(steeringTargetColl.position, traceShapeRadius, ColorRGBA(1, 1, 0, 1.0f), fDt);

				float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 0.55f;

				Vector3D collPoint(steeringTargetColl.position);

				Plane dPlane(carRight, -dot(carPos, carRight));
				float posSteerFactor = -dPlane.Distance(collPoint);

				steeringDir += carRight * posSteerFactor * (1.0f - steeringTargetColl.fract) * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;
				steeringDir = normalize(steeringDir);
			}
		}

		// finalize steering
		Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

		const float AI_BRAKE_CURVE = 1.0f;
		const float AI_BRAKE_LIMIT = 0.9f;
		const float AI_BRAKE_PATH_FACTOR = 0.8f;

		float brakeFactor = powf((1.0f - fabs(dot(brakeDir, carForward))) + pathVelocityFactor* AI_BRAKE_PATH_FACTOR, AI_BRAKE_CURVE*weatherBrakePow);

		if (brakeFactor < 0.25f)
			brakeFactor = 0.0f;

		brakeFactor *= lowSpeedFactor;

		float accelFactor = (1.0f - brakeFactor);
		brakeFactor = min(brakeFactor, AI_BRAKE_LIMIT);

		handling.acceleration = 0.5f + accelFactor*0.5f;
		handling.braking = (brakeFactor-0.5f)*2.0f;

		if (speedFromWheels < -0.1f)
		{
			handling.acceleration = 1.0f;
			handling.braking = 0.0f;
		}

		handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z);
		handling.steering = clamp(handling.steering, -1.0f, 1.0f);

		// forcing auto handbrake on low speeds
		handling.autoHandbrake = (fabs(handling.steering) > 0.35f || lowSpeedFactor < 0.5f);

		/*
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
		*/
	}
}