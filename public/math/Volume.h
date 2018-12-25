//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Box
//				Frustum volume
//////////////////////////////////////////////////////////////////////////////////

#ifndef VOLUME_H
#define VOLUME_H

#include "Matrix.h"
#include "Vector.h"
#include "math_common.h"

enum ClassifyPlane_e
{
	CP_FRONT = 0,
	CP_ONPLANE,
	CP_BACK,
};

template <class T>
struct Plane_t
{
	Plane_t(){}
	Plane_t(const T x, const T y, const T z, const T o)
	{
		normal = TVec3D<T>(x, y, z);
		T invLen = (T)rsqrtf(lengthSqr(normal));
		normal *= invLen;
		offset = o * invLen;
	}

	Plane_t(const TVec3D<T>& n, const T o)
	{
		normal = n;

		T invLen = (T)rsqrtf(lengthSqr(normal));

		normal *= invLen;
		offset = o * invLen;
	}

	Plane_t(const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2)
	{
		normal = fastNormalize(cross(v2-v1, v0-v1));

		offset = -dot(normal, v0);
	}

	T Distance(const TVec3D<T> &pos) const
	{
		return dot(normal, pos) + offset;
	}

	ClassifyPlane_e ClassifyPoint(const TVec3D<T> &pos) const
	{
		T fDist = Distance(pos);

		if(fDist > 0.0001f)
			return CP_FRONT;
		else if(fDist < -0.0001f)
			return CP_BACK;

		return CP_ONPLANE;
	}

	ClassifyPlane_e ClassifyPointEpsilon(const TVec3D<T> &pos, const T eps)
	{
		T fDist = Distance(pos);

		if(fDist > eps)
			return CP_FRONT;
		else if(fDist < -eps)
			return CP_BACK;

		return CP_ONPLANE;
	}

	bool GetIntersectionWithPlanes(const Plane_t<T>& o1,const Plane_t<T>& o2, TVec3D<T>& outPoint) const
	{
		T	fDenom = dot(normal, cross(o1.normal, o2.normal));

		if ( fabs( fDenom ) < 0.0001f )
		{
			return false;
		}

		outPoint = ( ( cross (o1.normal, o2.normal ) ) * -offset - ( cross (o2.normal, normal) ) * o1.offset - (cross (normal, o1.normal ) ) * o2.offset ) / fDenom;

		return true;
	}

	bool GetIntersectionWithRay(const TVec3D<T>& rayPos, const TVec3D<T>& rayDir, TVec3D<T>& outIntersection) const
	{
		// check direction for perpendicular
		T t2 = dot(normal, rayDir);

		if (t2 == 0)
			return false;

		T t = -(Distance(rayPos) / t2);
		outIntersection = rayPos + (rayDir * t);

		return true;
	}

	bool GetIntersectionLineFraction(const TVec3D<T>& lineStart, const TVec3D<T>& lineEnd, TVec3D<T>& outIntersection, T &fraction) const
	{
		TVec3D<T> lineVec = lineEnd - lineStart;
		T invLineLen = (T)rsqrtf(lengthSqr(lineVec));

		TVec3D<T> lineDir = lineVec * invLineLen;

		T fDenom = -dot(normal, lineDir);

		// is not perpendicular or backfacing?
		if ( fDenom <= 0 )
			return false;

		T distToPlane		= Distance( lineStart );
		T distToCollPoint	= distToPlane / fDenom;

		outIntersection	= lineStart + ( lineDir * distToCollPoint );
		fraction = distToCollPoint * invLineLen;

		// if end point is too far
		if(fraction > 1.0f)
		{
			fraction = 1.0f;
			return false;
		}

		return true;
	}

	bool CompareEpsilon(const Plane_t<T>& other, T fDistEps, T fNormalEps) const
	{
		if(abs(offset - other.offset) > fDistEps)
			return false;

		if(abs(normal.x - other.normal.x) > fNormalEps)
			return false;

		if(abs(normal.y - other.normal.y) > fNormalEps)
			return false;

		if(abs(normal.z - other.normal.z) > fNormalEps)
			return false;

		return true;
	}

	TVec3D<T>	normal;
	T			offset;
};

template <typename T>
inline Plane_t<T> operator * (const TMat4<T> &m, const Plane_t<T> &v)
{
	TVec4D<T> o(v.normal * v.offset, 1.0);
	TVec3D<T> n(v.normal);

	o = m * o;
	n = m * n;

	return Plane_t<T>(n, dot(o.xyz(), n));
}

typedef Plane_t<float> Plane;

#define VOLUME_PLANE_LEFT   0
#define VOLUME_PLANE_RIGHT  1
#define VOLUME_PLANE_TOP    2
#define VOLUME_PLANE_BOTTOM 3
#define VOLUME_PLANE_FAR    4
#define VOLUME_PLANE_NEAR   5

class Volume
{
public:
    void			LoadAsFrustum(const Matrix4x4 &mvp);
	void			LoadBoundingBox(Vector3D &mins, Vector3D &maxs);

	// returns back bounding box if not frustum.
	void			GetBBOXBack(Vector3D &mins, Vector3D &maxs) const;

	bool			IsPointInside(const Vector3D &pos) const;
	bool			IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps = 0.0f) const;
	bool			IsBoxInside(const Vector3D &mins, const Vector3D &maxs, const float eps = 0.0f) const;
	bool			IsTriangleInside(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2) const;
	bool			IsSphereInside(const Vector3D &pos, const float radius) const;

	bool			IsIntersectsRay(const Vector3D &start,const Vector3D &dir, Vector3D &intersectionPos, float eps = 0.0f) const;

	const Plane&	GetPlane(const int plane) const { return m_planes[plane]; }

	void			SetupPlane(Plane &pl, int n);

	Vector3D		GetFarRightDown() const;
	Vector3D		GetFarRightUp() const;
	Vector3D		GetFarLeftDown() const;
	Vector3D		GetFarLeftUp() const;

protected:
	Plane			m_planes[6];
};

inline Volume operator * (const Matrix4x4 &m, const Volume &v)
{
	Volume out;
	for(int i = 0; i < 6; i++)
	{
		Plane pl = m * v.GetPlane(i);
		out.SetupPlane(pl, i);
	}
	return out;
}

#endif // VOLUME_H
