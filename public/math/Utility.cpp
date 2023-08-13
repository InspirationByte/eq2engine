//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Math additional utilites
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "Utility.h"

int HashVector2D(const Vector2D& v, float tolerance)
{
	const IVector2D& fixed = v / tolerance;
	return (fixed.x * 12391) + (fixed.y * 14561);
}

int HashVector3D(const Vector3D& v, float tolerance)
{
	const IVector3D& fixed = v / tolerance;
	return (fixed.x * 12391) + (fixed.y * 14561) + (fixed.z * 18397);
}

float SnapFloat(float grid_spacing, float val)
{
	return round(val / grid_spacing) * grid_spacing;
}

Vector3D SnapVector(float grid_spacing, const Vector3D& vector)
{
	return Vector3D(
		SnapFloat(grid_spacing, vector.x),
		SnapFloat(grid_spacing, vector.y),
		SnapFloat(grid_spacing, vector.z));
}

bool PointToScreen(const Vector3D& point, Vector3D& screen, const Matrix4x4& worldToScreen, const Vector2D& screenDims)
{
	const Vector4D outMat = worldToScreen * Vector4D(point, 1.0f);

	const bool behind = (outMat.w < 0);
	const float zDiv = outMat.w == 0.0f ? 1.0f : (1.0f / outMat.w);

	screen.x = (screenDims.x * outMat.x * zDiv + screenDims.x) * 0.5f;
	screen.y = (screenDims.y - screenDims.y * outMat.y * zDiv) * 0.5f;
	screen.z = outMat.z;

	return behind;
}

bool PointToScreen(const Vector3D& point, Vector2D& screen, const Matrix4x4 & worldToScreen, const Vector2D &screenDims)
{
	Vector3D tmpScreen;
	const bool behind = PointToScreen(point, tmpScreen, worldToScreen, screenDims);

	screen = tmpScreen.xy();

	return behind;
}

void ScreenToDirection(const Vector3D& cam_pos, const Vector2D& point, const Vector2D& screensize, Vector3D& start, Vector3D& dir, const Matrix4x4& wwp, bool bIsOrthogonal)
{
	Volume frs;
	frs.LoadAsFrustum(wwp);

	const Vector3D farLeftUp = frs.GetFarLeftUp();
	const Vector3D lefttoright = frs.GetFarRightUp() - farLeftUp;
	const Vector3D uptodown = frs.GetFarLeftDown() - farLeftUp;

	const float dx = point.x / screensize.x;
	const float dy = point.y / screensize.y;

	if (bIsOrthogonal)
		start = cam_pos + (lefttoright * (dx-0.5f)) + (uptodown * (dy-0.5f));
	else
		start = cam_pos;

	dir = (farLeftUp + (lefttoright * dx) + (uptodown * dy)) - start;
}

Vector2D UVFromPointOnTriangle( const Vector3D& p1, const Vector3D& p2, const Vector3D& p3,
								const Vector2D &uv1, const Vector2D &uv2, const Vector2D &uv3,
								const Vector3D &point)
{
	// edges
	const Vector3D v0 = p3 - p1;
	const Vector3D v1 = p2 - p1;
	const Vector3D v2 = point - p1;

	// edge lengths
    const float dot00 = dot(v0, v0);
    const float dot01 = dot(v0, v1);
    const float dot02 = dot(v0, v2);
    const float dot11 = dot(v1, v1);
    const float dot12 = dot(v1, v2);

    // make barycentric coordinates
    const float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);

    const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    const Vector2D t2 = uv2-uv1;
    const Vector2D t1 = uv3-uv1;

    return uv1 + t1*u + t2*v;
}

Vector3D PointFromUVOnTriangle( const Vector3D& v1, const Vector3D& v2, const Vector3D& v3,
								 const Vector3D& t1, const Vector3D& t2, const Vector3D& t3,
								 const Vector2D& p)
{
	const float i = 1 / ((t2.x - t1.x) * (t3.y - t1.y) - (t2.y - t1.y) * (t3.x - t1.x));
	const float s = i * ( (t3.y - t1.y) * (p.x - t1.x) - (t3.x - t1.x) * (p.y - t1.y));
	const float t = i * (-(t2.y - t1.y) * (p.x - t1.x) + (t2.x - t1.x) * (p.y - t1.y));

	return Vector3D(v1 + s*(v2-v1) + t*(v3-v1));
}

bool IsPointInCone( Vector3D &pt, Vector3D &origin, Vector3D &axis, float cosAngle, float cone_length )
{
	const Vector3D delta = pt - origin;

	const float dist = length(delta);
	const float fdot = dot( normalize(delta), axis );

	if ( fdot < cosAngle )
		return false;

	if ( dist * fdot > cone_length )
		return false;

	return true;
}

// checks rat for triangle intersection
bool IsRayIntersectsTriangle(const Vector3D& pt1, const Vector3D& pt2, const Vector3D& pt3, const Vector3D& linept, const Vector3D& vect, float& fraction, bool doTwoSided)
{
	// get triangle edges
	const Vector3D edge1 = pt2-pt1;
	const Vector3D edge2 = pt3-pt1;

	// find normal
	const Vector3D pvec = cross(vect, edge2);

	const float det = dot(edge1, pvec);

	Vector3D uvCoord;

	if(!doTwoSided)
	{
		// Use culling
		if(det < F_EPS)
			return false;

		Vector3D tvec = linept-pt1;
		uvCoord.x = dot(tvec, pvec);

		if(uvCoord.x < 0.0f || uvCoord.x > det)
			return false;

		Vector3D qvec = cross(tvec, edge1);
		uvCoord.y = dot(vect, qvec);

		if(uvCoord.y < 0.0f || uvCoord.y + uvCoord.x > det)
			return false;

		float inv_det = 1.0f / det;

		fraction = dot(edge2, qvec) * inv_det;
	}
	else
	{
		// No culling
		if(det > -F_EPS && det < F_EPS)
			return false;

		float inv_det = 1.0f / det;

		Vector3D tvec = linept-pt1;
		uvCoord.x = dot(tvec, pvec)*inv_det;

		if(uvCoord.x < 0.0f || uvCoord.x > 1.0f)
			return false;

		Vector3D qvec = cross(tvec, edge1);
		uvCoord.y = dot(vect, qvec) * inv_det;

		if(uvCoord.y < 0.0f || uvCoord.y + uvCoord.x > 1.0f)
			return false;

		fraction = dot(edge2, qvec) * inv_det;
	}

	return true;
}

bool LineIntersectsLine2D(const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE, Vector2D& isectPoint)
{
	const Vector2D A = lAE - lAB;
	const Vector2D B = lBE - lBB;

	const float det = A.x * B.y - A.y * B.x;

	if (det < F_EPS && det > -F_EPS)
		return false;

	const Vector2D D = lBB - lAB;

	const float oneByDet = 1.0f / det;
	const float s = (A.x * D.y - A.y * D.x) * oneByDet;

	isectPoint.x = lBB.x + s * B.x;
	isectPoint.y = lBB.y + s * B.y;

	return true;
}

bool LineSegIntersectsLineSeg2D(const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE, Vector2D& isectPoint)
{
	const Vector2D A = lAE - lAB;
	const Vector2D B = lBE - lBB;

	const float det = A.x * B.y - A.y * B.x;

	if (det < F_EPS && det > -F_EPS)
		return false;

	const Vector2D D = lBB - lAB;

	const float oneByDet = 1.0f / det;
	const float r = (B.y * D.x - B.x * D.y) * oneByDet;
	const float s = (A.x * D.y - A.y * D.x) * oneByDet;

	isectPoint.x = lBB.x + s * B.x;
	isectPoint.y = lBB.y + s * B.y;

	return !(r < 0.0f || r > 1.0f || s < 0.0f || s > 1.0f);
}

static float orient2D(const Vector2D& O, const Vector2D& A, const Vector2D& B)
{
	return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

static int cmpFloat(float a, float b)
{
	return (a > b) - (a < b);
}

void ConvexHull2D(Array<Vector2D>& points, Array<Vector2D>& hull)
{
	const int n = points.numElem();
	if (n <= 3)
		return;

	hull.assureSize(2 * n);

	// sort points lexicographically
	quickSort(points, [](const Vector2D& a, const Vector2D& b) {
		int cmp = cmpFloat(a.x, b.x);
		return cmp == 0 ? cmpFloat(a.y, b.y) : cmp;
	});

	// lower hull
	int k = 0;
	for (int i = 0; i < n; ++i)
	{
		while (k >= 2 && orient2D(hull[k - 2], hull[k - 1], points[i]) < F_EPS)
			k--;
		hull[k++] = points[i];
	}

	// upper hull
	for (int i = n - 1, t = k + 1; i > 0; --i)
	{
		while (k >= t && orient2D(hull[k - 2], hull[k - 1], points[i - 1]) < F_EPS)
			k--;
		hull[k++] = points[i - 1];
	}

	hull.setNum(k - 1);
}

float RectangleAlongAxisSeparation(const Vector2D& pos, const Vector2D& axis, float extent,
	const Vector2D& otherDir, const Vector2D& otherPerpDir, const Vector2D& otherExtents)
{
	float result = fabs(dot(pos, axis)) - extent;
	result -= fabs(dot(otherDir, axis) * otherExtents[0]);
	result -= fabs(dot(otherPerpDir, axis) * otherExtents[1]);
	return result;
}

float RectangleRectangleSeparation(
	const Vector2D& position0, const Vector2D& direction0, const Vector2D& perp0, const Vector2D& extents0,
	const Vector2D& position1, const Vector2D& direction1, const Vector2D& perp1, const Vector2D& extents1)
{
	const Vector2D localPosition = position0 - position1;
	float result = RectangleAlongAxisSeparation(localPosition, direction0, extents0[0], direction1, perp1, extents1);

	float v = RectangleAlongAxisSeparation(localPosition, perp0, extents0[1], direction1, perp1, extents1);
	result = (result <= v) ? v : result;

	v = RectangleAlongAxisSeparation(localPosition, direction1, extents1[0], direction0, perp0, extents0);
	result = (result <= v) ? v : result;

	v = RectangleAlongAxisSeparation(localPosition, perp1, extents1[1], direction0, perp0, extents0);
	result = (result <= v) ? v : result;

	return result;
}

// converts angles in [-180, 180] in Radians
float ConstrainAnglePI(float x)
{
	// NOTE: we add full circle to ensure that values close to -2PI work alright
	return fmodf(x + M_PI_F + M_PI_2_F, M_PI_2_F) - M_PI_F;
}

// normalizes angles in [-180, 180]
float ConstrainAngle180(float x)
{
	// NOTE: we add full circle to ensure that values close to -360 work alright
	return fmodf(x + 180.0f + 360.0f, 360.0f) - 180.0f;
}

// normalizes angles vector in [-180, 180]
Vector3D NormalizeAngles180(const Vector3D& angles)
{
	Vector3D ang;

	ang.x = ConstrainAngle180(angles.x);
	ang.y = ConstrainAngle180(angles.y);
	ang.z = ConstrainAngle180(angles.z);

	return ang;
}


// computes angle difference (degrees)
float AngleDiff(float a, float b)
{
	return ConstrainAngle180(b - a);
}

// computes angle difference (Radians)
float AngleDiffRad(float a, float b)
{
	return ConstrainAnglePI(b - a);
}

float VecAngleDiff(const Vector2D& dirA, const Vector2D& dirB)
{
	return RAD2DEG(atan2f(dirA.x * dirB.y - dirA.y * dirB.x, dirA.x * dirB.x + dirA.y * dirB.y));
}

// computes angles difference (degrees)
Vector3D AnglesDiff(const Vector3D& a, const Vector3D& b)
{
	Vector3D angDiff;

	angDiff.x = AngleDiff(a.x, b.x);
	angDiff.y = AngleDiff(a.y, b.y);
	angDiff.z = AngleDiff(a.z, b.z);

	return angDiff;
}