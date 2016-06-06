//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Math additional utilites
//////////////////////////////////////////////////////////////////////////////////

#include "Matrix.h"
#include "Vector.h"
//#include "IShaderAPI.h"
#include "Volume.h"
#include "math_util.h"

bool PointToScreen(const Vector3D& point, Vector2D& screen, const Matrix4x4 &mvp, const Vector2D &screenDims)
{
	Matrix4x4 worldToScreen = mvp;

	bool behind = false;

	Vector4D outMat;
	Vector4D transpos(point,1.0f);

	outMat = worldToScreen*transpos;

	if(outMat.w < 0)
		behind = true;

	float zDiv = outMat.w == 0.0f ? 1.0f :
		(1.0f / outMat.w);

	//int vW,vH;
	//g_pShaderAPI->GetViewportDimensions(vW,vH);

	screen.x = ((screenDims.x * outMat.x * zDiv) + screenDims.x) / 2;
	screen.y = ((screenDims.y - (screenDims.y * (outMat.y * zDiv)))) / 2;

	return behind;
}

bool PointToScreen_Z(const Vector3D& point, Vector3D& screen, const Matrix4x4 &wwp, const Vector2D &screenDims)
{
	Matrix4x4 worldToScreen = wwp;
	bool behind = false;

	Vector4D outMat;
	Vector4D transpos(point,1.0f);

	outMat = worldToScreen*transpos;

	if(outMat.w < 0)
		behind = true;

	float zDiv = outMat.w == 0.0f ? 1.0f :
		(1.0f / outMat.w);


	screen.x = ((screenDims.x * outMat.x * zDiv) + screenDims.x) / 2;
	screen.y = ((screenDims.y - (screenDims.y * (outMat.y * zDiv)))) / 2;
	screen.z = outMat.z;

	return behind;
}

void ConvertMatrix(const Matrix4x4 &srcMat, double outMat[16])
{
	Matrix4x4 srcTranspose = transpose(srcMat);

	// Loop and convert to double format
	const float * srcData = (const float*)srcTranspose.rows;
	for(int i=0; i< 16; i++)
	{
		outMat[i] = srcData[i];
	}
}

void ScreenToDirection(const Vector3D& cam_pos, const Vector2D& point, const Vector2D& screensize, Vector3D& start, Vector3D& dir, const Matrix4x4& wwp, bool bIsOrthogonal)
{
	Volume frs;
	frs.LoadAsFrustum(wwp);

	Vector3D farLeftUp = frs.GetFarLeftUp();
	Vector3D lefttoright = frs.GetFarRightUp() - farLeftUp;
	Vector3D uptodown = frs.GetFarLeftDown() - farLeftUp;

	float dx = point.x / screensize.x;
	float dy = point.y / screensize.y;

	if (bIsOrthogonal)
		start = cam_pos + (lefttoright * (dx-0.5f)) + (uptodown * (dy-0.5f));
	else
		start = cam_pos;

	dir = (farLeftUp + (lefttoright * dx) + (uptodown * dy)) - start;
}

// Returns true if a box intersects with a sphere
bool IsBoxIntersectingSphere( const Vector3D& boxMin, const Vector3D& boxMax, const Vector3D& center, float radius )
{
	float dmin = 0;
	float sr2 = radius*radius;

	// x axis
	if(center.x < boxMin.x)
		dmin += ((center.x-boxMin.x)*(center.x-boxMin.x));
	else if(center.x > boxMax.x)
		dmin += (((center.x-boxMax.x))*((center.x-boxMax.x)));

	// y axis
	if(center.y < boxMin.y)
		dmin +=((center.y-boxMin.y)*(center.y-boxMin.y));
	else if(center.y > boxMax.y)
		dmin +=(((center.y-boxMax.y))*((center.y-boxMax.y)));

	// z axis
	if(center.z < boxMin.z)
		dmin += ((center.z-boxMin.z)*(center.z-boxMin.z));
	else if(center.z>boxMax.z)
		dmin += (((center.z-boxMax.z))*((center.z-boxMax.z)));

	return (dmin <= sr2);
}

bool Is2DPointInside2DBox(const Vector2D& point, const Vector2D& boxMin, const Vector2D& boxMax )
{
	return point >= boxMin && point <= boxMax;
}

bool IsBox1GreaterThanBox2( const Vector3D& box1Min, const Vector3D& box1Max, const Vector3D& box2Min, const Vector3D& box2Max )
{
	Vector3D box1Center = (box1Min + box1Max) * 0.5f;
	Vector3D box2Center = (box2Min + box2Max) * 0.5f;

	Vector3D box1Size = (box1Max + box1Center);
	Vector3D box2Size = (box2Max + box2Center);

	return length(box1Size) > length(box2Size);
}

bool IsBoxIntersectingBox( const Vector3D& box1Min, const Vector3D& box1Max, const Vector3D& box2Min, const Vector3D& box2Max )
{
	bool MinXYIntersectsBoxXY = Is2DPointInside2DBox(box1Min.xy(), box2Min.xy(), box2Max.xy());
	bool MaxXYIntersectsBoxXY = Is2DPointInside2DBox(box1Max.xy(), box2Min.xy(), box2Max.xy());

	bool MinYZIntersectsBoxYZ = Is2DPointInside2DBox(box1Min.yz(), box2Min.yz(), box2Max.yz());
	bool MaxYZIntersectsBoxYZ = Is2DPointInside2DBox(box1Max.yz(), box2Min.yz(), box2Max.yz());

	bool MinXZIntersectsBoxXZ = Is2DPointInside2DBox(box1Min.xz(), box2Min.xz(), box2Max.xz());
	bool MaxXZIntersectsBoxXZ = Is2DPointInside2DBox(box1Max.xz(), box2Min.xz(), box2Max.xz());

	return (MinXYIntersectsBoxXY || MinYZIntersectsBoxYZ || MinXZIntersectsBoxXZ) ||
		(MaxXYIntersectsBoxXY || MaxYZIntersectsBoxYZ || MaxXZIntersectsBoxXZ);
}

bool IsBoxInsideBox( const Vector3D& box1Min, const Vector3D& box1Max, const Vector3D& box2Min, const Vector3D& box2Max )
{
	return (box1Min.x < box2Max.x) && (box1Max.x > box2Min.x) &&
	(box1Min.y < box2Max.y) && (box1Max.y > box2Min.y) &&
	(box1Min.z < box2Max.z) && (box1Max.z > box2Min.z);
}

Vector2D UVFromPointOnTriangle( const Vector3D& p1, const Vector3D& p2, const Vector3D& p3,
								const Vector2D &uv1, const Vector2D &uv2, const Vector2D &uv3,
								const Vector3D &point)
{
	// edges
	Vector3D v0 = p3 - p1;
	Vector3D v1 = p2 - p1;
	Vector3D v2 = point - p1;

	// edge lengths
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    // make barycentric coordinates
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);

    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    Vector2D t2 = uv2-uv1;
    Vector2D t1 = uv3-uv1;

    return uv1 + t1*u + t2*v;
}

Vector3D PointFromUVOnTriangle( const Vector3D& v1, const Vector3D& v2, const Vector3D& v3,
								 const Vector3D& t1, const Vector3D& t2, const Vector3D& t3,
								 const Vector2D& p)
{
	float i = 1 / ((t2.x - t1.x) * (t3.y - t1.y) - (t2.y - t1.y) * (t3.x - t1.x));
	float s = i * ( (t3.y - t1.y) * (p.x - t1.x) - (t3.x - t1.x) * (p.y - t1.y));
	float t = i * (-(t2.y - t1.y) * (p.x - t1.x) + (t2.x - t1.x) * (p.y - t1.y));

	return Vector3D(v1 + s*(v2-v1) + t*(v3-v1));
}

float TriangleArea( const Vector3D& v1, const Vector3D& v2, const Vector3D& v3)
{
	// compute edge lengths
	float d1 = length(v1 - v2);
	float d2 = length(v2 - v3);
	float d3 = length(v3 - v1);

	float h = (d1 + d2 + d3) * 0.5f;

	return sqrt(h * (h - d1) * (h - d2) * (h - d3));
}

bool IsPointInCone( Vector3D &pt, Vector3D &origin, Vector3D &axis, float cosAngle, float cone_length )
{
	Vector3D delta = pt - origin;

	float dist = length(delta);

	float fdot = dot( normalize(delta), axis );

	if ( fdot < cosAngle )
		return false;

	if ( dist * fdot > cone_length )
		return false;

	return true;
}

Vector3D NormalOfTriangle(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2)
{
	//Calculate vectors along polygon sides
	Vector3D side0 = v0 - v1;
	Vector3D side1 = v2 - v1;

	//Calculate normal
	Vector3D normal = cross(side1, side0);
	return normalize(normal);
}

// Returns MATRIX_RIGHTHANDED if matrix is right-handed, else returns MATRIX_LEFTHANDED.
MatrixHandedness_e UTIL_GetMatrixHandedness(const Matrix4x4& matrix)
{
  return (dot(cross(matrix.rows[0].xyz(), matrix.rows[1].xyz()), matrix.rows[3].xyz()) > 0.0) ? MATRIX_LEFTHANDED : MATRIX_RIGHTHANDED;
}

#define INTERSECTION_EPS 0.0001

// checks rat for triangle intersection
bool IsRayIntersectsTriangle(const Vector3D& pt1, const Vector3D& pt2, const Vector3D& pt3, const Vector3D& linept, const Vector3D& vect, float& fraction, bool doTwoSided)
{
	Vector3D uvCoord;

	// get triangle edges
	Vector3D edge1 = pt2-pt1;
	Vector3D edge2 = pt3-pt1;

	// find normal
	Vector3D pvec = cross(vect, edge2);

	float det = dot(edge1, pvec);

	if(!doTwoSided)
	{
		// Use culling
		if(det < INTERSECTION_EPS)
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
		if(det > -INTERSECTION_EPS && det < INTERSECTION_EPS)
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

// compares floats (for array sort)
int UTIL_CompareFloats(float f1, float f2)
{
	if(f1 < f2)
		return -1;

	else if(f1 > f2)
		return 1;

	return 0;
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
    float dif = fmodf(b - a + 180,360);

    if (dif < 0)
        dif += 360;

    return dif - 180;
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
