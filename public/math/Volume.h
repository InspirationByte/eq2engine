//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Box
//				Frustum volume
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/ArrayRef.h"

enum EVolumePlane
{
	VOLUME_PLANE_LEFT	   = 0,
	VOLUME_PLANE_RIGHT,  //= 1,
	VOLUME_PLANE_TOP,    //= 2,
	VOLUME_PLANE_BOTTOM, //= 3,
	VOLUME_PLANE_FAR,    //= 4,
	VOLUME_PLANE_NEAR,   //= 5
};


class Volume
{
public:
	static constexpr const int VOLUME_PLANE_COUNT = 6;

	void				LoadAsFrustum(const Matrix4x4 &mvp);

	// precision constructor
	void				LoadAsFrustum(const Matrix4x4 &mvp, bool _PRECISION);

	void				LoadBoundingBox(const Vector3D &mins, const Vector3D &maxs);

	// precision constructor
	void				LoadBoundingBox(const Vector3D &mins, const Vector3D &maxs, bool _PRECISION);

	// returns back bounding box if not frustum.
	void				GetBBOXBack(Vector3D &mins, Vector3D &maxs) const;

	bool				IsPointInside(const Vector3D &pos) const;
	bool				IsSphereInside(const Vector3D& pos, const float radius) const;
	bool				IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps = 0.0f) const;
	bool				IsBoxInside(const Vector3D &mins, const Vector3D &maxs, const float eps = 0.0f) const;
	bool				IsBoxInside(const BoundingBox& box, const float eps = 0.0f) const;
	bool				IsTriangleInside(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2) const;

	bool				IsIntersectsRay(const Vector3D &start,const Vector3D &dir, Vector3D &intersectionPos, float eps = 0.0f) const;

	const Plane&		GetPlane(const int plane) const { return m_planes[plane]; }
	ArrayCRef<Plane>	GetPlanes() const { return ArrayCRef(m_planes, VOLUME_PLANE_COUNT); }

	void				SetupPlane(const Plane &pl, int n);

	Vector3D			GetFarRightDown() const;
	Vector3D			GetFarRightUp() const;
	Vector3D			GetFarLeftDown() const;
	Vector3D			GetFarLeftUp() const;

	static bool			IsPointInside(ArrayCRef<Plane> planes, const Vector3D& pos);
	static bool			IsSphereInside(ArrayCRef<Plane> planes, const Vector3D& pos, const float radius);
	static bool			IsBoxInside(ArrayCRef<Plane> planes, const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps = 0.0f);
	static bool			IsBoxInside(ArrayCRef<Plane> planes, const Vector3D& mins, const Vector3D& maxs, const float eps = 0.0f);
	static bool			IsBoxInside(ArrayCRef<Plane> planes, const BoundingBox& box, const float eps = 0.0f);
	static bool			IsTriangleInside(ArrayCRef<Plane> planes, const Vector3D& v0, const Vector3D& v1, const Vector3D& v2);
	static bool			IsIntersectsRay(ArrayCRef<Plane> planes, const Vector3D& start, const Vector3D& dir, Vector3D& intersectionPos, float eps = 0.0f);

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