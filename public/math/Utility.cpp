//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Math additional utilites
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "Utility.h"

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
	points.sort([](const Vector2D& a, const Vector2D& b) {
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

struct OrientBox2D
{
	Vector2D x;
	float length[2];
	float theta;
	
	Vector2D axis[2];
	float dist[2];
	float limit[2];
};

bool OrientedBoxIntersection2D(OrientBox2D body[2], float* boxOverlap = nullptr)
{
	// calc axes of each box
	for (int i = 0; i < 2; i++)
	{
		const float as = sinf(body[i].theta);
		const float ac = cosf(body[i].theta);

		body[i].axis[0].x = as;
		body[i].axis[0].y = ac;

		body[i].axis[1].y = -as;
		body[i].axis[1].x = ac;
	}

	const float dtheta = body[1].theta - body[0].theta;

	const float as = sinf(dtheta);
	const float ac = sinf(dtheta + M_PI_HALF_F);

	const Vector2D delta = body[0].x - body[1].x;

	int k = 0;

	// do SAT tests for each axis
	for (int i = 1; i >= 0; --i)
	{
		for (int j = 1; j >= 0; --j)
		{
			body[i].dist[j] = dot(body[i].axis[j], delta);
			body[i].limit[j] = body[i].length[j] + (body[k].length[j] * ac + body[k].length[1-j] * as);

			if (body[i].dist[j] < -body[i].limit[j] || body[i].dist[j] > body[i].limit[j])
				return false;
		}

		k++;
	}

	// calc overlap if needed
	if (boxOverlap)
	{
		float xover, zover;
		float tmp = fabs(dot(body[0].axis[0], body[1].axis[0]));

		if (tmp > F_EPS)
			xover = fabs(fabs(body[1].dist[0]) - fabs(body[1].limit[0])) / tmp;
		else
			xover = 0.0f;

		tmp = fabs(dot(body[0].axis[0], body[1].axis[1]));

		if (tmp > F_EPS)
			zover = fabs(fabs(body[1].dist[1]) - fabs(body[1].limit[1])) / tmp;
		else
			zover = xover;

		if (xover < -F_EPS)
			*boxOverlap = zover;
		else if (zover < xover)
			*boxOverlap = zover;
		else
			*boxOverlap = xover;
	}

	return true;
}

// converts angles in [-180, 180] in Radians
float ConstrainAnglePI(float x)
{
	x = fmodf(x + M_PI_F, M_PI_2_F);

	if (x < 0)
		x += M_PI_2_F;

	return x - M_PI_F;
}

// converts angles to [0, 360] in Radians
float ConstrainAngle2PI(float x)
{
	x = fmodf(x, M_PI_2_F);

	if (x < 0)
		x += M_PI_2_F;

	return x;
}

// normalizes angles in [-180, 180]
float ConstrainAngle180(float x)
{
    x = fmodf(x + 180, 360);

    if (x < 0)
        x += 360;

    return x - 180;
}

// normalizes angles in [0, 360]
float ConstrainAngle360(float x)
{
    x = fmodf(x, 360);

    if (x < 0)
        x += 360;

    return x;
}

// computes angle difference (degrees)
float AngleDiff(float a, float b)
{
	float dif = fmodf(b - a + 180.0f, 360.0f);

    if (dif < 0.0f)
        dif += 360.0f;

	return dif - 180.0f;
}

// computes angle difference (Radians)
float AngleDiffRad(float a, float b)
{
	return fmodf(b - a + M_PI_F, M_PI_2_F) - M_PI_F;
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

// normalizes angles in [-180, 180]
Vector3D NormalizeAngles180(const Vector3D& angles)
{
	Vector3D ang;

	ang.x = ConstrainAngle180(angles.x);
	ang.y = ConstrainAngle180(angles.y);
	ang.z = ConstrainAngle180(angles.z);

	return ang;
}

float NormalizeAngle360(float angle)
{
    float newAngle = angle;

    while (newAngle <= -360)
		newAngle += 360.0f;

    while (newAngle > 360)
		newAngle -= 360.0f;

    return newAngle;
}

// normalizes angles in [0, 360]
Vector3D NormalizeAngles360(const Vector3D& angles)
{
	Vector3D ang;

	ang.x = NormalizeAngle360(angles.x);
	ang.y = NormalizeAngle360(angles.y);
	ang.z = NormalizeAngle360(angles.z);

	return ang;
}
