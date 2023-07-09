//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Plane
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EClassifyPlane
{
	CP_FRONT = 0,
	CP_ONPLANE,
	CP_BACK,
};

template <class T>
struct TPlane
{
	TPlane() {}
	TPlane(const T x, const T y, const T z, const T o)
	{
		normal = TVec3D<T>(x, y, z);
		T invLen = (T)rsqrtf(lengthSqr(normal));
		normal *= invLen;
		offset = o * invLen;
	}

	// precision constructor
	TPlane(const T x, const T y, const T z, const T o, bool _PRECISION)
	{
		normal = TVec3D<T>(x, y, z);
		T invLen = (T)1.0 / length(normal);
		normal *= invLen;
		offset = o * invLen;
	}

	TPlane(const TVec3D<T>& n, const T o)
	{
		normal = n;

		T invLen = (T)rsqrtf(lengthSqr(normal));

		normal *= invLen;
		offset = o * invLen;
	}

	// precision constructor
	TPlane(const TVec3D<T>& n, const T o, bool _PRECISION)
	{
		normal = n;

		T invLen = (T)1.0 / length(normal);

		normal *= invLen;
		offset = o * invLen;
	}

	TPlane(const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2)
	{
		normal = fastNormalize(cross(v2 - v1, v0 - v1));

		offset = -dot(normal, v0);
	}

	// precision constructor
	TPlane(const TVec3D<T>& v0, const TVec3D<T>& v1, const TVec3D<T>& v2, bool _PRECISION)
	{
		normal = normalize(cross(v2 - v1, v0 - v1));
		offset = -dot(normal, v0);
	}

	T Distance(const TVec3D<T> &pos) const
	{
		return dot(normal, pos) + offset;
	}

	EClassifyPlane ClassifyPoint(const TVec3D<T> &pos, const T eps = F_EPS) const
	{
		T fDist = Distance(pos);

		if (fDist > eps)
			return CP_FRONT;
		else if (fDist < -eps)
			return CP_BACK;

		return CP_ONPLANE;
	}

	bool GetIntersectionWithPlane(const TPlane<T>& plane, TVec3D<T>& p, TVec3D<T>& dir, const T eps = 0.0f) const
	{
		const TVec3D<T> axis = cross(normal, plane.normal);
		const T det = lengthSqr(axis);

		// check for parallelity
		if (abs(det) > eps)
		{
			// calculate the final (point, normal)
			p = ((cross(axis, plane.normal) * offset) + (cross(normal, axis) * plane.offset)) / det;
			dir = axis;

			return true;
		}

		return false;
	}

	bool GetIntersectionWithPlanes(const TPlane<T>& o1, const TPlane<T>& o2, TVec3D<T>& outPoint) const
	{
		T	fDenom = dot(normal, cross(o1.normal, o2.normal));

		if (fabs(fDenom) < F_EPS)
		{
			return false;
		}

		outPoint = ((cross(o1.normal, o2.normal)) * -offset - (cross(o2.normal, normal)) * o1.offset - (cross(normal, o1.normal)) * o2.offset) / fDenom;

		return true;
	}

	bool GetIntersectionWithRay(const TVec3D<T>& rayPos, const TVec3D<T>& rayDir, TVec3D<T>& outIntersection, T& dist) const
	{
		// check direction for perpendicular
		T t2 = dot(normal, rayDir);

		if (t2 == 0)
			return false;

		T t = -(Distance(rayPos) / t2);
		outIntersection = rayPos + (rayDir * t);

		return true;
	}

	bool GetIntersectionWithRay(const TVec3D<T>& rayPos, const TVec3D<T>& rayDir, TVec3D<T>& outIntersection) const
	{
		T dummy;
		return GetIntersectionWithRay(rayPos, rayDir, outIntersection, dummy);
	}

	bool GetIntersectionWithLine(const TVec3D<T>& lineStart, const TVec3D<T>& lineEnd, TVec3D<T>& outIntersection, T &fraction) const
	{
		TVec3D<T> lineVec = lineEnd - lineStart;
		T invLineLen = (T)rsqrtf(lengthSqr(lineVec));

		TVec3D<T> lineDir = lineVec * invLineLen;

		T fDenom = -dot(normal, lineDir);

		// is not perpendicular or backfacing?
		if (fDenom <= 0)
			return false;

		T distToPlane = Distance(lineStart);
		T distToCollPoint = distToPlane / fDenom;

		outIntersection = lineStart + (lineDir * distToCollPoint);
		fraction = distToCollPoint * invLineLen;

		// if end point is too far
		if (fraction > 1.0f)
		{
			fraction = 1.0f;
			return false;
		}

		return true;
	}

	bool GetIntersectionWithLine(const TVec3D<T>& lineStart, const TVec3D<T>& lineEnd, TVec3D<T>& outIntersection) const
	{
		T dummy;
		return GetIntersectionWithLine(lineStart, lineEnd, outIntersection, dummy);
	}

	bool CompareEpsilon(const TPlane<T>& other, T fDistEps, T fNormalEps) const
	{
		if (abs(offset - other.offset) > fDistEps)
			return false;

		for (int i = 0; i < 3; ++i)
		{
			if (abs(normal[i] - other.normal[i]) > fNormalEps)
				return false;
		}

		return true;
	}

	TVec3D<T>	normal;
	T			offset;
};

template <typename T>
inline TPlane<T> operator * (const TMat4<T> &m, const TPlane<T> &v)
{
	TVec4D<T> o(v.normal * v.offset, 1.0);
	TVec3D<T> n(v.normal);

	o = m * o;
	n = m * n;

	return TPlane<T>(n, dot(o.xyz(), n));
}

typedef TPlane<float> Plane;