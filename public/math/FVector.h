//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed vector math part
//////////////////////////////////////////////////////////////////////////////////

#ifndef FVECTOR_H
#define FVECTOR_H

#include "FixedMath.h"

template<typename T> struct TVec2D;
template<typename T> struct TVec3D;
template<typename T> struct TVec4D;
template<typename T> struct TMat2;
template<typename T> struct TMat3;
template<typename T> struct TMat4;
template<typename T> struct Plane_t;
template<typename T, int TMAX> struct TAABBox;

// fixed vectors
typedef TVec2D<FReal>	FVector2D;
typedef TVec3D<FReal>	FVector3D;
typedef TVec4D<FReal>	FVector4D;

// fixed matrices
typedef TMat2<FReal>	FMatrix2x2;
typedef TMat3<FReal>	FMatrix3x3;
typedef TMat4<FReal>	FMatrix4x4;

typedef Plane_t<FReal>	FPlane;
typedef TAABBox<FReal, 32767>				FBoundingBox;

// define some functions that only can be used correctly on fixed point variables



#endif // FVECTOR_H