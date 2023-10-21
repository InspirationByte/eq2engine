//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Common math and logics
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/common_types.h"
#include <math.h>
#include <float.h>

enum ECubeSide : int
{
	POSITIVE_X = 0,
	NEGATIVE_X,
	POSITIVE_Y,
	NEGATIVE_Y,
	POSITIVE_Z,
	NEGATIVE_Z,

	FIRST = 0,
	LAST = 5
};

#ifndef IsNaN
#	define IsNaN(x)				((x) != (x))
#endif

#define FLOATSIGNBITSET(f)		((*(const unsigned long *)&(f)) >> 31)
#define FLOATSIGNBITNOTSET(f)	((~(*(const unsigned long *)&(f))) >> 31)
#define FLOATNOTZERO(f)			((*(const unsigned long *)&(f)) & ~(1<<31) )
#define INTSIGNBITSET(i)		(((const unsigned long)(i)) >> 31)
#define INTSIGNBITNOTSET(i)		((~((const unsigned long)(i))) >> 31)

constexpr const float F_EPS			= 0.00001f;
constexpr const float F_INFINITY	= 1900000.0f;
constexpr const float F_UNDEF		= 888.888f;

constexpr const double M_PI_D		= 3.14159265358979323846264338327950288;
constexpr const float M_PI_F		= float(M_PI_D);
constexpr const float M_PI_HALF_F	= M_PI_F * 0.5f;
constexpr const float M_PI_2_F		= M_PI_F * 2.0f;

constexpr const float M_RAD2DEG		= (180.f / M_PI_F);
constexpr const float M_DEG2RAD		= (M_PI_F / 180.f);

#define RAD2DEG( x )			( (x) * M_RAD2DEG )
#define DEG2RAD( x )			( (x) * M_DEG2RAD )
#define M_SQR( x )				sqr(x)

// Math routines done in optimized assembly math package routines
inline void SinCos( float radians, float *sine, float *cosine )
{
#if defined(_WIN32) && !defined(_WIN64)
	_asm
	{
		fld		DWORD PTR [radians]
		fsincos

		mov edx, DWORD PTR [cosine]
		mov eax, DWORD PTR [sine]

		fstp DWORD PTR [edx]
		fstp DWORD PTR [eax]
	}
#else
	*sine = sin(radians);
	*cosine = cos(radians);
#endif
}

// Math routines done in optimized assembly math package routines
inline void SinCos( double radians, double *sine, double *cosine )
{
	// GCC handles better...
	*sine = sin(radians);
	*cosine = cos(radians);
}

// It computes a fast 1 / sqrtf(v) approximation
inline float rsqrtf( float v )
{
	float v_half = v * 0.5f;
    int i = *(int *) &v;
    i = 0x5f3759df - (i >> 1);
    v = *(float *) &i;
    return v * (1.5f - v_half * v * v);
}

inline float sincf( const float x )
{
	return (x == 0.0f) ? 1.0f : sinf(x) / x;
}

inline float intAdjustf(const float x, const float diff = 0.01f)
{
	float f = roundf(x);

	return (fabsf(f - x) < diff)? f : x;
}

inline unsigned int getClosestPowerOfTwo(const unsigned int x)
{
	unsigned int i = 1;
	while (i < x) i += i;

	if (4 * x < 3 * i) i >>= 1;
	return i;
}

inline unsigned int getUpperPowerOfTwo(const unsigned int x)
{
	unsigned int i = 1;
	while (i < x) i += i;

	return i;
}

inline unsigned int getLowerPowerOfTwo(const unsigned int x)
{
	unsigned int i = 1;
	while (i <= x) i += i;

	return i >> 1;
}

inline bool fsimilar( float a, float b, float cmp = F_EPS )
{
	return fabs(a-b) < cmp;
}

inline bool dsimilar( double a, double b, double cmp = F_EPS )
{
	return fabs(a-b) < cmp;
}

// Note: returns true for 0
inline bool isPowerOf2(const int x)
{
	return (x & (x - 1)) == 0;
}

template <typename T, typename T2>
inline T clamp(const T v, const T2 c0, const T2 c1)
{
	return min((T)max((T)v, (T)c0), (T)c1);
}

// Remap a value in the range [A,B] to [C,D].
inline float RemapVal(float val, float A, float B, float C, float D)
{
	return 	(C + (D - C) * (val - A) / (B - A));
}

// Remap a value in the range [A,B] to [C,D] and clamp result to [C,D]
inline float RemapValClamp(float val, float A, float B, float C, float D)
{
	if (val < A) return C;
	if (val > B) return D;
	return C + (D - C) * (val - A) / (B - A);
}

template<typename T>
inline constexpr T sqr(const T x)
{
	return x * x;
}

#include "Vector.h"
#include "FVector.h"
#include "Matrix.h"
#include "BoundingBox.h"
#include "Quaternion.h"
#include "Plane.h"
#include "Volume.h"
#include "Rectangle.h"

#include "Vector.inl"
#include "Matrix.inl"