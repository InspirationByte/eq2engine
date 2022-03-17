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
#include "Volume.h"
#include "FVector.h"

#define M_PI_D				double(3.14159265358979323846264338327950288)
#define M_PI_F				((float)M_PI_D)
#define M_PI_OVER_TWO_F		float(1.57079633)

#ifndef IsNaN
#	define IsNaN(x)			((x) != (x))
#endif

#ifndef RAD2DEG
#	define RAD2DEG( x  )	( (float)(x) * (float)(180.f / M_PI_F) )
#endif

#ifndef DEG2RAD
#	define DEG2RAD( x  )	( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

#ifndef VDEG2RAD
#	define VDEG2RAD( v )	Vector3D(DEG2RAD(v.x),DEG2RAD(v.y),DEG2RAD(v.z))
#endif

#ifndef VRAD2DEG
#	define VRAD2DEG( v )	Vector3D(RAD2DEG(v.x),RAD2DEG(v.y),RAD2DEG(v.z))
#endif

#endif // DKMATH_H
