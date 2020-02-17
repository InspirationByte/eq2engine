//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Axis-aligned bounding box
//////////////////////////////////////////////////////////////////////////////////

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "Vector.h"
#include "FixedMath.h"

template<class T>
T dist_check(const T pn, const T bmin, const T bmax )
{
	T out = 0;
	T v = pn;

	if ( v < bmin )
	{
		T val = (bmin - v);
		out += val * val;
	}
	if ( v > bmax )
	{
		T val = (v - bmax);
		out += val * val;
	}

	return out;
};

template<class T, int TMAX>
struct TAABBox //BoundingBox
{
    TAABBox(const TVec3D<T>& v1, const TVec3D<T>& v2)
    {
        if ( v1.x < v2.x )
        {
            minPoint.x = v1.x;
            maxPoint.x = v2.x;
        }
        else
        {
            minPoint.x = v2.x;
            maxPoint.x = v1.x;
        }

        if ( v1.y < v2.y )
        {
            minPoint.y = v1.y;
            maxPoint.y = v2.y;
        }
        else
        {
            minPoint.y = v2.y;
            maxPoint.y = v1.y;
        }

        if ( v1.z < v2.z )
        {
            minPoint.z = v1.z;
            maxPoint.z = v2.z;
        }
        else
        {
            minPoint.z = v2.z;
            maxPoint.z = v1.z;
        }
    }

    TAABBox()
    {
        Reset();
    }

    void AddVertex(const TVec3D<T>& v)
    {
        if ( v.x < minPoint.x )
            minPoint.x = v.x;

        if ( v.x > maxPoint.x )
            maxPoint.x = v.x;

        if ( v.y < minPoint.y )
            minPoint.y = v.y;

        if ( v.y > maxPoint.y )
            maxPoint.y = v.y;

        if ( v.z < minPoint.z )
            minPoint.z = v.z;

        if ( v.z > maxPoint.z )
            maxPoint.z = v.z;
    }

	void AddVertices(const TVec3D<T> *v, int numVertices)
	{
		for (int i = 0; i < numVertices; i++)
			AddVertex(v[i]);
	}

	// FIXME: quite bad function
	void AddVertices(const void *v, int numVertices, int posDataOffset, int stride)
	{
		char* pData = (char*)v;

		for (int i = 0; i < numVertices; i++)
		{
			TVec3D<T> pos = *((TVec3D<T>*)(pData + stride*i + posDataOffset));
			AddVertex(pos);
		}
	}

	bool IsEmpty() const
	{
		return minPoint.x > maxPoint.x || minPoint.y > maxPoint.y || minPoint.z > maxPoint.z;
	}

    bool Contains( const TVec3D<T>& pos, T tolerance = 0 ) const
    {
        return pos.x >= minPoint.x-tolerance && pos.x <= maxPoint.x+tolerance &&
               pos.y >= minPoint.y-tolerance && pos.y <= maxPoint.y+tolerance &&
               pos.z >= minPoint.z-tolerance && pos.z <= maxPoint.z+tolerance;
    }

	float SquaredDistPointAABB( const TVec3D<T>& p ) const
	{
		// Squared distance
		T sq = 0.0;

		sq += dist_check( p.x, minPoint.x, maxPoint.x );
		sq += dist_check( p.y, minPoint.y, maxPoint.y );
		sq += dist_check( p.z, minPoint.z, maxPoint.z );

		return sq;
	}

	bool IntersectsSphere( const TVec3D<T>& center, T radius ) const
	{
		float squaredDistance = SquaredDistPointAABB( center );

		return squaredDistance <= (radius * radius);
	}

	bool Intersects ( const TAABBox<T, TMAX>& bbox, T tolerance = 0) const
	{
		bool overlap = true;
		overlap = (minPoint.x-tolerance > bbox.maxPoint.x || maxPoint.x+tolerance < bbox.minPoint.x) ? false : overlap;
		overlap = (minPoint.z-tolerance > bbox.maxPoint.z || maxPoint.z+tolerance < bbox.minPoint.z) ? false : overlap;
		overlap = (minPoint.y-tolerance > bbox.maxPoint.y || maxPoint.y+tolerance < bbox.minPoint.y) ? false : overlap;

		return overlap;
	}

	// warning, this is a size-dependent!
	bool FullyInside ( const TAABBox<T, TMAX>& box, T tolerance = 0) const
	{
		if(	box.minPoint >= minPoint-tolerance && box.minPoint <= maxPoint+tolerance &&
			box.maxPoint <= maxPoint+tolerance && box.maxPoint >= minPoint-tolerance)
			return true;

		return false;
	}

	bool IntersectsRay(const TVec3D<T>& rayStart, const TVec3D<T>& rayDir, T& tnear, T& tfar) const
	{
		TVec3D<T> T_1, T_2; // vectors to hold the T-values for every direction
		T t_near	= (T)-TMAX;
		T t_far		= (T)TMAX;

		for (int i = 0; i < 3; i++)
		{
			if (rayDir[i] == 0)
			{
				// ray parallel to planes in this direction
				if ((rayStart[i] < minPoint[i]) || (rayStart[i] > maxPoint[i]))
					return false; // parallel AND outside box : no intersection possible
			}
			else
			{
				// ray not parallel to planes in this direction
				T_1[i] = (minPoint[i] - rayStart[i]) / rayDir[i];
				T_2[i] = (maxPoint[i] - rayStart[i]) / rayDir[i];

				if(T_1[i] > T_2[i])
					swap(T_1, T_2);

				if (T_1[i] > t_near)
					t_near = T_1[i];

				if (T_2[i] < t_far)
					t_far = T_2[i];

				if( (t_near > t_far) || (t_far < 0) )
					return false;
			}
		}

		tnear = t_near;
		tfar = t_far;

		return true;
	}

    void Reset()
    {
        minPoint = TVec3D<T>((T)TMAX);
		maxPoint = TVec3D<T>(-(T)TMAX);
    }

	const TVec3D<T>& GetMinPoint() const
	{
		return minPoint;
	}

	const TVec3D<T>& GetMaxPoint() const
	{
		return maxPoint;
	}

    TVec3D<T> GetVertex(int index) const
    {
        return TVec3D<T>( index & 1 ? maxPoint.x : minPoint.x,
                          index & 2 ? maxPoint.y : minPoint.y,
                          index & 4 ? maxPoint.z : minPoint.z );
    }

	TVec3D<T> GetCenter() const
	{
		return (minPoint + maxPoint) / (T)2;
	}

	TVec3D<T> GetSize() const
	{
		return maxPoint - minPoint;
	}

	TVec3D<T> ClampPoint( const TVec3D<T>& pos ) const
	{
		return clamp(pos, minPoint, maxPoint);
	}

	// adds other bbox to this box
    void Merge( const TAABBox<T, TMAX>& box )
    {
        if ( box.minPoint.x < minPoint.x )
           minPoint.x = box.minPoint.x;

        if ( box.minPoint.y < minPoint.y )
           minPoint.y = box.minPoint.y;

        if ( box.minPoint.z < minPoint.z )
           minPoint.z = box.minPoint.z;

        if ( box.maxPoint.x > maxPoint.x )
           maxPoint.x = box.maxPoint.x;

        if ( box.maxPoint.y > maxPoint.y )
           maxPoint.y = box.maxPoint.y;

        if ( box.maxPoint.z > maxPoint.z )
           maxPoint.z = box.maxPoint.z;
    }

	// fixes bounds if they are broken
	void Fix()
	{
		TVec3D<T> mins = minPoint;
		TVec3D<T> maxs = maxPoint;

		Reset();

		AddVertex(mins);
		AddVertex(maxs);
	}

	TVec3D<T>	minPoint;
	TVec3D<T>	maxPoint;
};

typedef TAABBox<float, (int)V_MAX_COORD> BoundingBox;
typedef TAABBox<FReal, 32767>		FBoundingBox;

#endif //BOUNDINGBOX_H
