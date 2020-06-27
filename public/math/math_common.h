//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Common math and logics
//////////////////////////////////////////////////////////////////////////////////

#ifndef MATH_COMMON_H
#define MATH_COMMON_H

#include <math.h>
#include <float.h>

#define FLOATSIGNBITSET(f)		((*(const unsigned long *)&(f)) >> 31)
#define FLOATSIGNBITNOTSET(f)	((~(*(const unsigned long *)&(f))) >> 31)
#define FLOATNOTZERO(f)			((*(const unsigned long *)&(f)) & ~(1<<31) )
#define INTSIGNBITSET(i)		(((const unsigned long)(i)) >> 31)
#define INTSIGNBITNOTSET(i)		((~((const unsigned long)(i))) >> 31)

#ifndef NULL
#define NULL 0
#endif

#define F_EPS			0.0001f
#define V_MAX_COORD		393216.0f		// common maximum in the Equilibrium engine

#define roundf(x) floorf((x) + 0.5f)

#if defined(_INC_MINMAX)
#error Please remove minmax from includes
#endif

#undef min
#undef max

template <class T, class T2 = T, class TR = T>
inline TR min(T x, T2 y) {return (TR)((x < y)? x : y);}

template <class T, class T2 = T, class TR = T>
inline TR max(T x, T2 y) {return (TR)((x > y)? x : y);}

// Math routines done in optimized assembly math package routines
inline void SinCos( float radians, float *sine, float *cosine )
{
	// GCC handles better...

	//*sine = sin(radians);
	//*cosine = cos(radians);

#ifdef _WIN32
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

	*sine = sinf(radians);
	*cosine = cosf(radians);

/*
	register double __cosr, __sinr;
 	__asm __volatile__
    		("fsincos"
     	: "=t" (__cosr), "=u" (__sinr) : "0" (radians));

  	*sine = __sinr;
  	*cosine = __cosr;
	*/
#endif

}

// Math routines done in optimized assembly math package routines
inline void SinCos( float radians, double *sine, double *cosine )
{
	// GCC handles better...

	*sine = sin(radians);
	*cosine = cos(radians);

	/*
#ifdef _WIN32
	_asm
	{
		fld		DWORD PTR [radians]
		fsincos

		mov edx, DWORD PTR [cosine]
		mov eax, DWORD PTR [sine]

		fstp DWORD PTR [edx]
		fstp DWORD PTR [eax]
	}
#elif LINUX
	register double __cosr, __sinr;
 	__asm __volatile__
    		("fsincos"
     	: "=t" (__cosr), "=u" (__sinr) : "0" (radians));

  	*sine = __sinr;
  	*cosine = __cosr;
#endif*/
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

inline float sqrf( const float x )
{
    return x * x;
}

inline float sincf( const float x )
{
	return (x == 0)? 1 : sinf(x) / x;
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

#if !defined(_MSC_VER) || _MSC_VER < 1800
inline int round(float x)
{
	if (x > 0){
		return int(x + 0.5f);
	} else {
		return int(x - 0.5f);
	}
}
#endif // _MSC_VER

inline bool fsimilar( float a, float b, float cmp = F_EPS )
{
	return fabs(a-b) < cmp;
}

inline bool dsimilar( double a, double b, double cmp = F_EPS )
{
	return fabs(a-b) < cmp;
}

// Note: returns true for 0
inline bool IsPowerOf2(const int x)
{
	return (x & (x - 1)) == 0;
}

template<typename T>
inline void swap(T& a, T& b)
{
	T tmp = a;

	a = b;
	b = tmp;
}

// Remap a value in the range [A,B] to [C,D].
#define RemapVal( val, A, B, C, D) \
	(C + (D - C) * (val - A) / (B - A))

#define RemapValClamp( val, A, B, C, D) \
	clamp(C + (D - C) * (val - A) / (B - A), C, D)

#define POSITIVE_X 0
#define NEGATIVE_X 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_Z 4
#define NEGATIVE_Z 5

#endif // MATH_COMMON_H
