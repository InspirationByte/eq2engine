//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Math additional utilites
//////////////////////////////////////////////////////////////////////////////////

#pragma once

int HashVector2D(const Vector2D& v, float tolerance);
int HashVector3D(const Vector3D& v, float tolerance);

float SnapFloat(float grid_spacing, float val);
Vector3D SnapVector(float grid_spacing, const Vector3D& vector);

bool PointToScreen(const Vector3D& point, Vector2D& screen, const Matrix4x4& worldToScreen, const Vector2D &screenDims);
bool PointToScreen(const Vector3D& point, Vector3D& screen, const Matrix4x4& worldToScreen, const Vector2D &screenDims);
void ScreenToDirection(	const Vector3D& cam_pos, const Vector2D& point, const Vector2D& screensize,
										Vector3D& start, Vector3D& dir, const Matrix4x4& wwp, bool bIsOrthogonal = false);

//---------------------------------------------------------------------------------

// uv point from triangle and 3d position
Vector2D UVFromPointOnTriangle(  const Vector3D& p1, const Vector3D& p2, const Vector3D& p3,
											const Vector2D &uv1, const Vector2D &uv2, const Vector2D &uv3,
											const Vector3D &point);
// point from triangle and uv position
Vector3D PointFromUVOnTriangle(  const Vector3D& v1, const Vector3D& v2, const Vector3D& v3,
											const Vector3D& t1, const Vector3D& t2, const Vector3D& t3,
											const Vector2D& p);

// point in cone
bool IsPointInCone(Vector3D &pt, Vector3D &origin, Vector3D &axis, float cosAngle, float cone_length );

// checks triangle-ray intersection
bool IsRayIntersectsTriangle(const Vector3D& pt1, const Vector3D& pt2, const Vector3D& pt3, 
											const Vector3D& linept, const Vector3D& vect, 
											float& fraction, bool doTwoSided = false);

//---------------------------------------------------------------------------------

bool LineIntersectsLine2D(const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE, Vector2D& isectPoint);
bool LineSegIntersectsLineSeg2D(const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE, Vector2D& isectPoint);

//---------------------------------------------------------------------------------

void ConvexHull2D(Array<Vector2D>& points, Array<Vector2D>& out);

//---------------------------------------------------------------------------------

float RectangleAlongAxisSeparation(const Vector2D& pos, const Vector2D& axis, float extent,
	const Vector2D& otherDir, const Vector2D& otherPerpDir, const Vector2D& otherExtents);

// Returns separation distance (negative = rectangles overlapping)
float RectangleRectangleSeparation(
	const Vector2D& position0, const Vector2D& direction0, const Vector2D& perp0, const Vector2D& extents0,
	const Vector2D& position1, const Vector2D& direction1, const Vector2D& perp1, const Vector2D& extents1);

//---------------------------------------------------------------------------------

// Compute normal of triangle
template <typename T>
void ComputeTriangleNormal(const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2, 
											TVec3D<T>& normal);

// Compute a TBN space for triangle
template <typename T, typename T2>
void ComputeTriangleTBN(	const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2, 
										const TVec2D<T2>& t0, const TVec2D<T2>& t1, const TVec2D<T2>& t2, 
										TVec3D<T>& normal, TVec3D<T>& tangent, TVec3D<T>& binormal);

// returns triangle area
template <typename T>
float ComputeTriangleArea(const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2);

#include "TriangleUtil.inl"

//---------------------------------------------------------------------------------

// converts angles in [-180, 180] in Radians
float ConstrainAnglePI(float x);

// converts angles in [-180, 180]
float ConstrainAngle180(float x);

// normalizes angles vector in [-180, 180]
Vector3D NormalizeAngles180(const Vector3D& angles);

// computes angle difference (degrees)
float AngleDiff(float a, float b);

// computes angle difference (Radians)
float AngleDiffRad(float a, float b);

// computes angles difference (degrees)
Vector3D AnglesDiff(const Vector3D& a, const Vector3D& b);

// computes angles difference between two 2D direction vectors (degrees)
float VecAngleDiff(const Vector2D& dirA, const Vector2D& dirB);

// Fourier square wave function
template <int N>
inline float FT_SquareWave( float x )
{
	float val = 0.0;

	for(float i = 0; i < N; i++)
	{
		float v1 = (1+2.0f*i);
		float v2 = (i+i+1)*2.0f;
		val += (1.0f / v1) * sinf(v2*x);
	}

	return val;
}

// torsional spring function with zero offset
// inline because MSVC doesn't let me do forward declaration
template<typename T>
inline void SpringFunction(T& value, T& velocity, float spring_const, float spring_damp, float fDt)
{
	value += velocity * fDt;
	float damping = 1 - (spring_damp * fDt);

	if ( damping < 0 )
		damping = 0.0f;

	velocity *= damping;

	// torsional spring
	float springForceMagnitude = spring_const * fDt;
	springForceMagnitude = clamp(springForceMagnitude, 0.0f, 2.0f );
	velocity -= value * springForceMagnitude;
}
