//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Spline functions
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "Spline.h"

static Vector3D BezierCubicPoint(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3, float t)
{
	const float t_2 = sqr(t);
	const float t_3 = t_2 * t;
	const float oneMinusT = 1.0f - t;
	const float oneMinusT_2 = sqr(oneMinusT);
	const float oneMinusT_3 = oneMinusT_2 * oneMinusT;
	return oneMinusT_3 * p0 
		+ 3.0f * oneMinusT_2 * t * p1 
		+ 3.0f * oneMinusT * t_2 * p2
		+ t_3 * p3;
}

static Vector3D BezierCubicTangent(const Vector3D& d0, const Vector3D& d1, const Vector3D& d2, float t)
{
	const float t_2 = sqr(t);
	const float oneMinusT = 1.0f - t;
	const float oneMinusT_2 = sqr(oneMinusT);

	return 3.0f * oneMinusT_2 * d0
		 + 3.0f * 2.0f * t * oneMinusT * d1
		 + 3.0f * t_2 * d2;
}

Vector3D Spline3DPositionAtLocalTime(ArrayCRef<Spline3dPoint> points, int startPtIdx, float t)
{
	const int p0Idx = startPtIdx % points.numElem();
	const int p3Idx = (startPtIdx + 1) % points.numElem();

	const Vector3D p0 = points[p0Idx].position;
	const Vector3D p1 = points[p0Idx].tangents[TANGENT_AFTER];
	const Vector3D p2 = points[p3Idx].tangents[TANGENT_BEFORE];
	const Vector3D p3 = points[p3Idx].position;

	return BezierCubicPoint(p0, p1, p2, p3, t);
}

Vector3D Spline3DTangentAtLocalTime(ArrayCRef<Spline3dPoint> points, int startPtIdx, float t)
{
	const int p0Idx = startPtIdx % points.numElem();
	const int p3Idx = (startPtIdx + 1) % points.numElem();

	const Vector3D p0 = points[p0Idx].position;
	const Vector3D p1 = points[p0Idx].tangents[TANGENT_AFTER];
	const Vector3D p2 = points[p3Idx].tangents[TANGENT_BEFORE];
	const Vector3D p3 = points[p3Idx].position;

	const Vector3D d0 = p1 - p0;
	const Vector3D d1 = p2 - p1;
	const Vector3D d2 = p3 - p2;

	return BezierCubicTangent(d0, d1, d2, t);
}

int CSpline3d::GetSegmentIndexAndLocalTime(float time, float& localTime) const
{
	const int fromPt = SegmentIndexByLocalTime(time);
	if (fromPt == m_points.numElem() - 1)
	{
		localTime = 0.0f;
		return fromPt;
	}

	const float localDuration = m_points[fromPt + 1].time - m_points[fromPt].time;
	localTime = (time - m_points[fromPt].time) / localDuration;

	return fromPt;
}

// spline samplers
Vector3D CSpline3d::PositionAtTime(float time) const
{
	float localTime = 0.0f;
	const int startPtIdx = GetSegmentIndexAndLocalTime(time, localTime);

	return Spline3DPositionAtLocalTime(m_points, startPtIdx, localTime);
}

Vector3D CSpline3d::TangentAtTime(float time) const
{
	float localTime = 0.0f;
	const int startPtIdx = GetSegmentIndexAndLocalTime(time, localTime);

	return Spline3DTangentAtLocalTime(m_points, startPtIdx, localTime);
}

float CSpline3d::DistanceAtTime(float time) const
{
	if (time < F_EPS)
		return 0.0f;

	auto distComparator = [](const Vector2D& a, float t) -> int {
		return (a.x > t) - (a.x < t);
		};
	const int idx = arraySortedFindIndexExt<SortedFind::LAST_LEQUAL>(m_distances, time, distComparator);
	if (idx == -1 || idx == m_distances.numElem() - 1)
		return m_distances.back().y;

	const Vector2D firstDist = m_distances[idx];
	const Vector2D nextDist = m_distances[idx+1];
	const float factor = (time - firstDist.x) / (nextDist.x - firstDist.x);
	return lerp(firstDist.y, nextDist.y, factor);
}

float CSpline3d::TimeAtDistance(float distance) const
{
	if (distance < F_EPS)
		return 0.0f;

	auto distComparator = [](const Vector2D& a, float d) -> int {
		return (a.y > d) - (a.y < d);
		};
	const int idx = arraySortedFindIndexExt<SortedFind::LAST_LEQUAL>(m_distances, distance, distComparator);
	if (idx == -1 || idx == m_distances.numElem() - 1)
		return m_distances.back().x;

	const Vector2D firstDist = m_distances[idx];
	const Vector2D nextDist = m_distances[idx + 1];
	const float factor = (distance - firstDist.y) / (nextDist.y - firstDist.y);
	return lerp(firstDist.x, nextDist.x, factor);
}

// distance utils
void CSpline3d::UpdateDistances()
{
	m_distances.clear();
	if (m_points.numElem() < 2)
		return;

	const float oneByStepsPerSegment = 1.0f / float(m_stepsPerSegment);
	const int numPoints = m_points.numElem() - (m_loop ? 0 : 1);
	m_distances.reserve(numPoints * m_stepsPerSegment + 1);

	float length = 0.0f;
	const Vector3D firstPt = Spline3DPositionAtLocalTime(m_points, 0, 0.0f);
	Vector3D prevPt = firstPt;
	for (int i = 0; i < numPoints; ++i)
	{
		for (int j = 0; j < m_stepsPerSegment; ++j)
		{
			const float t = float(j) * oneByStepsPerSegment;
			const Vector3D point = Spline3DPositionAtLocalTime(m_points, i, t);
			length += distance(prevPt, point);
			m_distances.append(Vector2D(i + t, length));
			prevPt = point;
		}
	}

	const Vector3D lastPt = Spline3DPositionAtLocalTime(m_points, numPoints-1, 1.0f);
	length += distance(prevPt, lastPt);
	m_distances.append(Vector2D(numPoints, length));
}

Vector3D CSpline3d::PositionAtDistance(float distance) const
{
	const float t = TimeAtDistance(distance);

	const int startPtIdx = (int)t;
	const float localTime = t - floor(t);

	return Spline3DPositionAtLocalTime(m_points, startPtIdx, localTime);
}

Vector3D CSpline3d::TangentAtDistance(float distance) const
{
	const float t = TimeAtDistance(distance);

	const int startPtIdx = (int)t;
	const float localTime = t - floor(t);

	return Spline3DTangentAtLocalTime(m_points, startPtIdx, localTime);
}


// segments
float CSpline3d::GetSegmentLength(int segIdx) const
{
	if (m_loop)
		segIdx = segIdx % m_points.numElem();

	const int cp = segIdx * m_stepsPerSegment;
	const int np = (segIdx + 1) * m_stepsPerSegment;

	return m_distances[np].y - m_distances[cp].y;
}

int CSpline3d::SegmentIndexByLocalTime(float time) const
{
	ASSERT(time >= 0.0f);
	if (time > m_duration)
	{
		if (!m_loop)
			return m_points.numElem()-1;
		time -= floorf(time / m_duration) * m_duration;
	}

	for (int i = 0; i < m_points.numElem(); ++i) 
	{
		if (time < m_points[i].time)
			return i - 1;
	}
	return m_points.numElem() - 1;
}

int CSpline3d::SegmentIndexByDistance(float dist) const
{
	ASSERT(dist >= 0.0f);
	const float splineLen = GetLength();
	if (dist > splineLen)
	{
		if (!m_loop)
			return m_points.numElem()-1;
		dist -= floorf(dist / splineLen) * splineLen;
	}

	auto distComparator = [](const Vector2D& a, float d) -> int {
		return (a.y > d) - (a.y < d);
	};

	const int idx = arraySortedFindIndexExt<SortedFind::LAST_LEQUAL>(m_distances, dist, distComparator);
	return m_distances[idx].x;
}
