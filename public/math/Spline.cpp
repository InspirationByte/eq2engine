//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Spline functions
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "Spline.h"

// Quadratic
Vector3D BezierSpline::GetPoint(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, float t)
{
	t = clamp(t, 0.0f, 1.0f);
	const float oneMinusT = 1.0f - t;
	return oneMinusT * oneMinusT * p0 + 2.0f * oneMinusT * t * p1 + M_SQR(t) * p2;
}

// Quadratic
Vector3D BezierSpline::GetFirstDerivative(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, float t)
{
	return
		2.0f * (1.0f - t) * (p1 - p0) +
		2.0f * t * (p2 - p1);
}

// Cubic
Vector3D BezierSpline::GetPoint(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3, float t)
{
	t = clamp(t, 0.0f, 1.0f);
	const float oneMinusT = 1.0f - t;
	return
		oneMinusT * oneMinusT * oneMinusT * p0 +
		3.0f * oneMinusT * oneMinusT * t * p1 +
		3.0f * oneMinusT * t * t * p2 +
		t * t * t * p3;
}

// Cubic
Vector3D BezierSpline::GetFirstDerivative(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3, float t)
{
	t = clamp(t, 0.0f, 1.0f);
	const float oneMinusT = 1.0f - t;
	return
		3.0f * oneMinusT * oneMinusT * (p1 - p0) +
		6.0f * oneMinusT * t * (p2 - p1) +
		3.0f * t * t * (p3 - p2);
}

int BezierSpline::GetCurveCount() const
{
	return (points.numElem() - 1) / 3;
}

void BezierSpline::TimeToCurveIndex(float& t, int& i) const
{
	if (t >= 1.0f)
	{
		t = 1.0f;
		i = points.numElem() - 4.0;
	}
	else
	{
		t = clamp(t, 0.0f, 1.0f) * GetCurveCount();
		i = (int)t;
		t -= i;
		i *= 3;
	}
}

Vector3D BezierSpline::GetPoint(float t) const
{
	int i;
	TimeToCurveIndex(t, i);
	return GetPoint(points[i], points[i + 1], points[i + 2], points[i + 3], t);
}

Vector3D BezierSpline::GetVelocity(float t) const
{
	int i;
	TimeToCurveIndex(t, i);
	return GetFirstDerivative(points[i], points[i + 1], points[i + 2], points[i + 3], t);
}

void BezierSpline::Reset()
{
	points.setNum(4);
	points[0] = Vector3D(1.0f, 0.0f, 0.0f);
	points[1] = Vector3D(2.0f, 0.0f, 0.0f);
	points[2] = Vector3D(3.0f, 0.0f, 0.0f);
	points[3] = Vector3D(4.0f, 0.0f, 0.0f);

	pointModes.setNum(2);
	pointModes[0] = FREE;
	pointModes[1] = FREE;
}

Vector3D BezierSpline::GetControlPoint(int index) const
{
	return points[index];
}

void BezierSpline::SetControlPoint(int index, Vector3D point)
{
	if (index % 3 == 0)
	{
		const Vector3D delta = point - points[index];
		if (index > 0)
			points[index - 1] += delta;

		if (index + 1 < points.numElem())
			points[index + 1] += delta;
	}
	points[index] = point;
	EnforceMode(index);
}

BezierSpline::PointMode BezierSpline::GetControlPointMode(int index) const
{
	return pointModes[(index + 1) / 3];
}

void BezierSpline::SetControlPointMode(int index, PointMode mode)
{
	pointModes[(index + 1) / 3] = mode;
	EnforceMode(index);
}

void BezierSpline::EnforceMode(int index)
{
	const int modeIndex = (index + 1) / 3;
	const PointMode mode = pointModes[modeIndex];
	if (mode == FREE || modeIndex == 0 || modeIndex == pointModes.numElem() - 1)
		return;

	int middleIndex = modeIndex * 3;
	int fixedIndex, enforcedIndex;

	if (index <= middleIndex)
	{
		fixedIndex = middleIndex - 1;
		enforcedIndex = middleIndex + 1;
	}
	else
	{
		fixedIndex = middleIndex + 1;
		enforcedIndex = middleIndex - 1;
	}


	Vector3D middle = points[middleIndex];
	Vector3D enforcedTangent = middle - points[fixedIndex];
	if (mode == ALIGNED)
		enforcedTangent = normalize(enforcedTangent) * distance(middle, points[enforcedIndex]);

	points[enforcedIndex] = middle + enforcedTangent;
}