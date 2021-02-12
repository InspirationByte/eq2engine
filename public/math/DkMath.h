//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium mathematics header
//////////////////////////////////////////////////////////////////////////////////

#ifndef DKMATH_H
#define DKMATH_H

#include "Vector.h"
#include "Matrix.h"
#include "Random.h"
#include "BoundingBox.h"

#ifdef _WIN32
#define isnan _isnan
#endif // WIN32

#include "Volume.h"

// also add fixed point functionality
#include "FVector.h"

#define PI				3.14159265358979323846
#define PI_F			((float)PI)
#define PI_OVER_TWO_F	1.57079633

#ifndef RAD2DEG
	#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / PI_F) )
#endif

#ifndef DEG2RAD
	#define DEG2RAD( x  )  ( (float)(x) * (float)(PI_F / 180.f) )
#endif

#ifndef VDEG2RAD
	#define VDEG2RAD( v )  Vector3D(DEG2RAD(v.x),DEG2RAD(v.y),DEG2RAD(v.z))
#endif

#ifndef VRAD2DEG
	#define VRAD2DEG( v )  Vector3D(RAD2DEG(v.x),RAD2DEG(v.y),RAD2DEG(v.z))
#endif

#endif // DKMATH_H
