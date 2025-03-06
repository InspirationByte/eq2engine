//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Axis-aligned bounding box
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <class T>
struct TAABBox3D
{
	static constexpr const int VertexCount = 8;

	TVec3D<T>	minPoint;
	TVec3D<T>	maxPoint;

	TAABBox3D();
	TAABBox3D(T minX, T minY, T minZ, T maxX, T maxY, T maxZ);
	TAABBox3D(const TVec3D<T>& v1, const TVec3D<T>& v2);

	bool				IsValid() const;

	void				Reset();
	void				AddVertex(const TVec3D<T>& v);
	void				AddVertices(const TVec3D<T>* v, int numVertices);
	void				AddVertices(const void* v, int numVertices, int posDataOffset, int stride);

	void				Fix();
	void				Merge(const TAABBox3D<T>& box);
	void				Expand(T value);
	void				Expand(const TVec3D<T>& value);

	bool				Contains(const TVec3D<T>& pos, T tolerance = 0) const;

	bool				FullyInside(const TAABBox3D<T>& box, T tolerance = 0) const;
	bool				Intersects(const TAABBox3D<T>& bbox, T tolerance = 0) const;
	bool				IntersectsRay(const TVec3D<T>& rayStart, const TVec3D<T>& rayDir, T& tnear, T& tfar) const;
	bool				IntersectsSphere(const TVec3D<T>& center, T radius) const;

	const TVec3D<T>&	GetMinPoint() const { return minPoint; }
	const TVec3D<T>&	GetMaxPoint() const { return maxPoint; }

	TVec3D<T>			GetCenter() const { return (minPoint + maxPoint) / static_cast<T>(2); }
	TVec3D<T>			GetSize() const { return maxPoint - minPoint; }
	TVec3D<T>			ClampPoint( const TVec3D<T>& pos ) const { return clamp(pos, minPoint, maxPoint); }

	TVec3D<T>			GetVertex(int index) const;
};

template <class T>
TAABBox3D<T>::TAABBox3D()
{
	Reset();
}

#define SET_MINMAX(vmin, vmax, v1, v2) \
	if (v1 < v2) {				\
		vmin = v1; vmax = v2;	\
	} else {					\
		vmin = v2; vmax = v1;	\
	}

#define _MIN(a, b) ((a < b) ? a : b)
#define _MAX(a, b) ((a > b) ? a : b)

template <class T>
TAABBox3D<T>::TAABBox3D(T minX, T minY, T minZ, T maxX, T maxY, T maxZ)
{
	SET_MINMAX(minPoint.x, maxPoint.x, minX, maxX);
	SET_MINMAX(minPoint.y, maxPoint.y, minY, maxY);
	SET_MINMAX(minPoint.z, maxPoint.z, minZ, maxZ);
}

template <class T>
TAABBox3D<T>::TAABBox3D(const TVec3D<T>& v1, const TVec3D<T>& v2)
{
	SET_MINMAX(minPoint.x, maxPoint.x, v1.x, v2.x);
	SET_MINMAX(minPoint.y, maxPoint.y, v1.y, v2.y);
	SET_MINMAX(minPoint.z, maxPoint.z, v1.z, v2.z);
}

template <class T>
void TAABBox3D<T>::AddVertex(const TVec3D<T>& v)
{
	minPoint.x = _MIN(minPoint.x, v.x);
	minPoint.y = _MIN(minPoint.y, v.y);
	minPoint.z = _MIN(minPoint.z, v.z);

	maxPoint.x = _MAX(maxPoint.x, v.x);
	maxPoint.y = _MAX(maxPoint.y, v.y);
	maxPoint.z = _MAX(maxPoint.z, v.z);
}

#undef SET_MINMAX
#undef _MIN
#undef _MAX

template <class T>
void TAABBox3D<T>::AddVertices(const TVec3D<T>* v, int numVertices)
{
	for (int i = 0; i < numVertices; i++)
		AddVertex(v[i]);
}

template <class T>
void TAABBox3D<T>::AddVertices(const void* v, int numVertices, int posDataOffset, int stride)
{
	char* pData = (char*)v;

	for (int i = 0; i < numVertices; i++)
	{
		TVec3D<T> pos = *((TVec3D<T>*)(pData + stride * i + posDataOffset));
		AddVertex(pos);
	}
}

template <class T>
bool TAABBox3D<T>::IsValid() const
{
	return minPoint.x <= maxPoint.x || minPoint.y <= maxPoint.y || minPoint.z <= maxPoint.z;
}

template <class T>
bool TAABBox3D<T>::Contains(const TVec3D<T>& pos, T tolerance) const
{
	return  pos.x >= minPoint.x - tolerance && pos.x <= maxPoint.x + tolerance &&
			pos.y >= minPoint.y - tolerance && pos.y <= maxPoint.y + tolerance &&
			pos.z >= minPoint.z - tolerance && pos.z <= maxPoint.z + tolerance;
}

template <class T>
bool TAABBox3D<T>::FullyInside(const TAABBox3D<T>& box, T tolerance) const
{
	if (box.minPoint >= minPoint - tolerance && box.minPoint <= maxPoint + tolerance &&
		box.maxPoint <= maxPoint + tolerance && box.maxPoint >= minPoint - tolerance)
		return true;

	return false;
}

template <class T>
bool TAABBox3D<T>::Intersects(const TAABBox3D<T>& bbox, T tolerance) const
{
	bool overlap = true;
	overlap = (minPoint.x - tolerance > bbox.maxPoint.x || maxPoint.x + tolerance < bbox.minPoint.x) ? false : overlap;
	overlap = (minPoint.z - tolerance > bbox.maxPoint.z || maxPoint.z + tolerance < bbox.minPoint.z) ? false : overlap;
	overlap = (minPoint.y - tolerance > bbox.maxPoint.y || maxPoint.y + tolerance < bbox.minPoint.y) ? false : overlap;

	return overlap;
}

template <class T>
bool TAABBox3D<T>::IntersectsRay(const TVec3D<T>& rayStart, const TVec3D<T>& rayDir, T& tnear, T& tfar) const
{
	TVec3D<T> T_1, T_2; // vectors to hold the T-values for every direction
	T t_near = -static_cast<T>(F_INFINITY);
	T t_far = static_cast<T>(F_INFINITY);
	const T EPS = static_cast<T>(F_EPS);

	for (int i = 0; i < 3; i++)
	{
		if (abs(rayDir[i]) < EPS)
		{
			// ray parallel to planes in this direction
			if ((rayStart[i] < minPoint[i]) || (rayStart[i] > maxPoint[i]))
				return false; // parallel AND outside box : no intersection possible
		}
		else
		{
			const float oneByRayDir = 1.0f / rayDir[i];

			// ray not parallel to planes in this direction
			T_1[i] = (minPoint[i] - rayStart[i]) * oneByRayDir;
			T_2[i] = (maxPoint[i] - rayStart[i]) * oneByRayDir;

			if (T_1[i] > T_2[i])
				QuickSwap(T_1[i], T_2[i]);

			if (T_1[i] > t_near)
				t_near = T_1[i];

			if (T_2[i] < t_far)
				t_far = T_2[i];

			if ((t_near > t_far) || (t_far < static_cast<T>(0)))
				return false;
		}
	}

	tnear = t_near;
	tfar = t_far;

	return true;
}

template <class T>
bool TAABBox3D<T>::IntersectsSphere(const TVec3D<T>& center, T radius) const
{
	T dmin = static_cast<T>(0);

	for (int i = 0; i < 3; ++i)
	{
		if (center[i] < minPoint[i])
		{
			const T cmin = center[i] - minPoint[i];
			dmin += M_SQR(cmin);
		}
		else if (center[i] > maxPoint[i])
		{
			const T cmax = center[i] - maxPoint[i];
			dmin += M_SQR(cmax);
		}
	}

	return dmin <= M_SQR(radius);
}

template <class T>
void TAABBox3D<T>::Reset()
{
	minPoint = TVec3D<T>(static_cast<T>(F_INFINITY));
	maxPoint = TVec3D<T>(-static_cast<T>(F_INFINITY));
}

template <class T>
TVec3D<T> TAABBox3D<T>::GetVertex(int index) const
{
	return TVec3D<T>(index & 1 ? maxPoint.x : minPoint.x,
					 index & 2 ? maxPoint.y : minPoint.y,
					 index & 4 ? maxPoint.z : minPoint.z);
}

template <class T>
void TAABBox3D<T>::Merge(const TAABBox3D<T>& box)
{
	AddVertex(box.minPoint);
	AddVertex(box.maxPoint);
}

template <class T>
void TAABBox3D<T>::Fix()
{
	const TVec3D<T> mins = minPoint;
	const TVec3D<T> maxs = maxPoint;

	Reset();

	AddVertex(mins);
	AddVertex(maxs);
}

template <class T>
void TAABBox3D<T>::Expand(T value)
{
	maxPoint += value;
	minPoint -= value;
}

template <class T>
void TAABBox3D<T>::Expand(const TVec3D<T>& value)
{
	maxPoint += value;
	minPoint -= value;
}

typedef TAABBox3D<float>	BoundingBox;
typedef TAABBox3D<int>		IBoundingBox;