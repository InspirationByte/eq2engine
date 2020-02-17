//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Geometry tools
//////////////////////////////////////////////////////////////////////////////////

#ifndef GEOMTOOLS_H
#define GEOMTOOLS_H

#include "utils/DkList.h"
#include "math/DkMath.h"

// Get a vector3d from byte stream
Vector3D	UTIL_VertexAtPos(const ubyte* vertexBaseOffset, int stride, int vertexIndex);

// Compute normal of triangle
template <typename T>
void		ComputeTriangleNormal( const TVec3D<T> &v0, const TVec3D<T> &v1, const TVec3D<T> &v2, TVec3D<T>& normal);

// Compute a TBN space for triangle
template <typename T, typename T2>
void		ComputeTriangleTBN( const TVec3D<T> &v0, const TVec3D<T> &v1, const TVec3D<T> &v2, const TVec2D<T2> &t0, const TVec2D<T2> &t1, const TVec2D<T2> &t2, TVec3D<T>& normal, TVec3D<T>& tangent, TVec3D<T>& binormal);

// returns triangle area
template <typename T>
float		ComputeTriangleArea( const TVec3D<T> &v0, const TVec3D<T> &v1, const TVec3D<T> &v2 );

#include "GeomTools_inline.h"

/*

TODO: form macro for better usage

#define OBJ_ADD_REMAPPED_VERTEX(idx, vertex)			\
{														\
	if(vert_remap_table[ idx ] != -1)					\
	{													\
		indexdata.append( vert_remap_table[ idx ] );	\
	}													\
	else												\
	{													\
		int currVerts = vertexdata.numElem();			\
		vert_remap_table[idx] = currVerts;				\
		indexdata.append( currVerts );					\
		vertexdata.append( vertex );					\
	}													\
}

#define OBJ_ADD_VERTEX(vertex, name)					\
{														\
	int currVerts = vertexdata.numElem();				\
	vert_remap_table[idx] = currVerts;					\
	indexdata.append( currVerts );						\
	vertexdata.append( vertex );						\
}

*/


#endif // GEOMTOOLS_H
