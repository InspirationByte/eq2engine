//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: SIMD operations
//////////////////////////////////////////////////////////////////////////////////

#ifndef _SIMD_H_
#define _SIMD_H_

#include "Vector.h"
#include <xmmintrin.h>
#include "cpuInstrDefs.h"

// useful constants in SSE packed float format:
extern __m128 Four_Zeros;									// 0 0 0 0
extern __m128 Four_Ones;									// 1 1 1 1
extern __m128 Four_PointFives;								// .5 .5 .5 .5
extern __m128 Four_Epsilons;								// FLT_EPSILON FLT_EPSILON FLT_EPSILON FLT_EPSILON

#undef pfacc
#undef pfmul
FORCEINLINE void planeDistance3DNow(const Vector4D *plane, const Vector4D *point, int *result)
{
	__asm 
	{
		mov eax, plane
		mov ebx, point
		mov ecx, result

		movq mm0, [eax]     // [nx, ny]
		movq mm1, [eax + 8] // [nz, d]

		movq mm2, [ebx]     // [px, py]
		movq mm3, [ebx + 8] // [pz, 1]

		pfmul mm0, mm2      // [nx * px, ny * py]
		pfmul mm1, mm3      // [nz * pz, d]

		pfacc mm0, mm1
		pfacc mm0, mm0

		movd [ecx], mm0
	}
}


FORCEINLINE v4sf dot4(v4sf u, v4sf v)
{
	v4sf m = mulps(u, v);
	v4sf f = shufps(m, m, SHUFFLE(2, 3, 0, 1));
	m = addps(m, f);
	f = shufps(m, m, SHUFFLE(1, 0, 3, 2));
	return addps(m, f);
}


// replicate a single floating point value to all 4 components of an sse packed float
static inline __m128 MMReplicate(float f)
{
	__m128 value=_mm_set_ss(f);
	return _mm_shuffle_ps(value,value,0);
}

// replicate a single 32 bit integer value to all 4 components of an m128
static inline __m128 MMReplicateI(int i)
{
	__m128 value=_mm_set_ss(*((float *)&i));;
	return _mm_shuffle_ps(value,value,0);
}

// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
// No error checking!
static inline __m128 MMReciprocal(__m128 x)
{
	__m128 ret=_mm_rcp_ps(x);
	// newton iteration is Y(n+1)=2*Y(N)-x*Y(n)^2
	ret=_mm_sub_ps(_mm_add_ps(ret,ret),_mm_mul_ps(x,_mm_mul_ps(ret,ret)));
	return ret;
}
// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
// 1/0 will result in a big but NOT infinite result
static inline __m128 MMReciprocalSaturate(__m128 x)
{
	__m128 zero_mask=_mm_cmpeq_ps(x,Four_Zeros);
	x=_mm_or_ps(x,_mm_and_ps(Four_Epsilons,zero_mask));
	__m128 ret=_mm_rcp_ps(x);
	// newton iteration is Y(n+1)=2*Y(N)-x*Y(n)^2
	ret=_mm_sub_ps(_mm_add_ps(ret,ret),_mm_mul_ps(x,_mm_mul_ps(ret,ret)));
	return ret;
}

// class FourVectors stores 4 independent vectors for use by sse processing. These vectors are
// stored in the format x x x x y y y y z z z z so that they can be efficiently accelerated by
// SSE. 
class FourVectors
{
public:
	__m128 x,y,z;											// x x x x y y y y z z z z

	inline void DuplicateVector(Vector3D const &v)	  //< set all 4 vectors to the same vector value
	{
		x=MMReplicate(v.x);
		y=MMReplicate(v.y);
		z=MMReplicate(v.z);
	}

	inline __m128 const & operator[](int idx) const
	{
		return *((&x)+idx);
	}

	inline __m128 & operator[](int idx)
	{
		return *((&x)+idx);
	}

	inline void operator+=(FourVectors const &b)			//< add 4 vectors to another 4 vectors
	{
		x=_mm_add_ps(x,b.x);
		y=_mm_add_ps(y,b.y);
		z=_mm_add_ps(z,b.z);
	}
	inline void operator-=(FourVectors const &b)			//< subtract 4 vectors from another 4
	{
		x=_mm_sub_ps(x,b.x);
		y=_mm_sub_ps(y,b.y);
		z=_mm_sub_ps(z,b.z);
	}

	inline void operator*=(FourVectors const &b)			//< scale all four vectors per component scale
	{
		x=_mm_mul_ps(x,b.x);
		y=_mm_mul_ps(y,b.y);
		z=_mm_mul_ps(z,b.z);
	}

	inline void operator*=(float scale)						//< uniformly scale all 4 vectors
	{
		__m128 scalepacked=MMReplicate(scale);
		x=_mm_mul_ps(x,scalepacked);
		y=_mm_mul_ps(y,scalepacked);
		z=_mm_mul_ps(z,scalepacked);
	}
	
	inline void operator*=(__m128 scale)					//< scale 
	{
		x=_mm_mul_ps(x,scale);
		y=_mm_mul_ps(y,scale);
		z=_mm_mul_ps(z,scale);
	}

	inline __m128 operator*(FourVectors const &b) const		//< 4 dot products
	{
		__m128 dot=_mm_mul_ps(x,b.x);
		dot=_mm_add_ps(dot,_mm_mul_ps(y,b.y));
		dot=_mm_add_ps(dot,_mm_mul_ps(z,b.z));
		return dot;
	}

	inline __m128 operator*(Vector3D const &b) const			//< dot product all 4 vectors with 1 vector
	{
		__m128 dot=_mm_mul_ps(x,MMReplicate(b.x));
		dot=_mm_add_ps(dot,_mm_mul_ps(y,MMReplicate(b.y)));
		dot=_mm_add_ps(dot,_mm_mul_ps(z,MMReplicate(b.z)));
		return dot;
	}


	inline void Product(FourVectors const &b)				//< component by component mul
	{
		x=_mm_mul_ps(x,b.x);
		y=_mm_mul_ps(y,b.y);
		z=_mm_mul_ps(z,b.z);
	}
	inline void MakeReciprocal(void)						//< (x,y,z)=(1/x,1/y,1/z)
	{
		x=MMReciprocal(x);
		y=MMReciprocal(y);
		z=MMReciprocal(z);
	}

	inline void MakeReciprocalSaturate(void)				//< (x,y,z)=(1/x,1/y,1/z), 1/0=1.0e23
	{
		x=MMReciprocalSaturate(x);
		y=MMReciprocalSaturate(y);
		z=MMReciprocalSaturate(z);
	}

	// X(),Y(),Z() - get at the desired component of the i'th (0..3) vector.
	inline float const &X(int idx) const
	{
#ifdef LINUX
		return ((l_m128*)&x)->m128_f32[idx];
#else
		return x.m128_f32[idx];
#endif
	}

	inline float const &Y(int idx) const
	{
#ifdef LINUX
		return ((l_m128*)&y)->m128_f32[idx];
#else
		return y.m128_f32[idx];
#endif
	}
	inline float const &Z(int idx) const
	{
#ifdef LINUX
		return ((l_m128*)&z)->m128_f32[idx];
#else
		return z.m128_f32[idx];
#endif
	}

	// X(),Y(),Z() - get at the desired component of the i'th (0..3) vector.
	inline float &X(int idx)
	{
#ifdef LINUX
		return ((l_m128*)&x)->m128_f32[idx];
#else
		return x.m128_f32[idx];
#endif
	}

	inline float &Y(int idx)
	{
#ifdef LINUX
		return ((l_m128*)&y)->m128_f32[idx];
#else
		return y.m128_f32[idx];
#endif
	}
	inline float &Z(int idx)
	{
#ifdef LINUX
		return ((l_m128*)&z)->m128_f32[idx];
#else
		return z.m128_f32[idx];
#endif
	}

	inline Vector3D Vec(int idx) const						//< unpack one of the vectors
	{
		return Vector3D(X(idx),Y(idx),Z(idx));
	}
	
	FourVectors(void)
	{
	}

	/// LoadAndSwizzle - load 4 Vectors into a FourVectors, perofrming transpose op
	inline void LoadAndSwizzle(Vector3D const &a, Vector3D const &b, Vector3D const &c, Vector3D const &d)
	{
		__m128 row0=_mm_loadu_ps(&(a.x));
		__m128 row1=_mm_loadu_ps(&(b.x));
		__m128 row2=_mm_loadu_ps(&(c.x));
		__m128 row3=_mm_loadu_ps(&(d.x));
		// now, matrix is:
		// x y z ?
        // x y z ?
        // x y z ?
        // x y z ?
		_MM_TRANSPOSE4_PS(row0,row1,row2,row3);
		x=row0;
		y=row1;
		z=row2;
	}

	/// LoadAndSwizzleAligned - load 4 Vectors into a FourVectors, perofrming transpose op.
	/// all 4 vectors muyst be 128 bit boundary
	inline void LoadAndSwizzleAligned(Vector3D const &a, Vector3D const &b, Vector3D const &c, Vector3D const &d)
	{
		__m128 row0=_mm_load_ps(&(a.x));
		__m128 row1=_mm_load_ps(&(b.x));
		__m128 row2=_mm_load_ps(&(c.x));
		__m128 row3=_mm_load_ps(&(d.x));
		// now, matrix is:
		// x y z ?
        // x y z ?
        // x y z ?
        // x y z ?
		_MM_TRANSPOSE4_PS(row0,row1,row2,row3);
		x=row0;
		y=row1;
		z=row2;
	}

	/// return the squared length of all 4 vectors
	inline __m128 length2(void) const
	{
		return (*this)*(*this);
	}

	/// return the approximate length of all 4 vectors. uses the sse sqrt approximation instr
	inline __m128 length(void) const
	{
		return _mm_sqrt_ps(length2());
	}

	/// normalize all 4 vectors in place. not mega-accurate (uses sse reciprocal approximation instr)
	inline void VectorNormalizeFast(void)
	{
		__m128 mag_sq=(*this)*(*this);						// length^2
		(*this) *= _mm_rsqrt_ps(mag_sq);					// *(1.0/sqrt(length^2))
	}

	/// normnalize all 4 vectors in place. uses newton iteration for higher precision results than
	/// from VectorNormalizeFast.
	inline void VectorNormalize(void)
	{
		static __m128 FourHalves={0.5,0.5,0.5,0.5};
		static __m128 FourThrees={3.,3.,3.,3.};
		__m128 mag_sq=(*this)*(*this);						// length^2
		__m128 guess=_mm_rsqrt_ps(mag_sq);
		// newton iteration for 1/sqrt(x) : y(n+1)=1/2 (y(n)*(3-x*y(n)^2));
        guess=_mm_mul_ps(guess,_mm_sub_ps(FourThrees,_mm_mul_ps(mag_sq,_mm_mul_ps(guess,guess))));
		guess=_mm_mul_ps(Four_PointFives,guess);
		(*this) *= guess;
	}

	/// construct a FourVectors from 4 separate Vectors
	inline FourVectors(Vector3D const &a, Vector3D const &b, Vector3D const &c, Vector3D const &d)
	{
		LoadAndSwizzle(a,b,c,d);
	}
	
};

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE __m128 LoadZeroSIMD( void )
{
	return Four_Zeros;
}

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE __m128 LoadOneSIMD( void )
{
	return Four_Ones;
}

/// replicate a single 32 bit integer value to all 4 components of an m128
FORCEINLINE __m128 ReplicateIX4( int i )
{
	__m128 value = _mm_set_ss( * ( ( float *) &i ) );;
	return _mm_shuffle_ps( value, value, 0);
}


FORCEINLINE __m128 ReplicateX4( float flValue )
{
	__m128 value = _mm_set_ss( flValue );
	return _mm_shuffle_ps( value, value, 0 );
}

FORCEINLINE __m128 MinSIMD( const __m128 & a, const __m128 & b )				// min(a,b)
{
	return _mm_min_ps( a, b );
}

FORCEINLINE __m128 MaxSIMD( const __m128 & a, const __m128 & b )				// max(a,b)
{
	return _mm_max_ps( a, b );
}


FORCEINLINE __m128 AndSIMD( const __m128 & a, const __m128 & b )				// a & b
{
	return _mm_and_ps( a, b );
}

FORCEINLINE __m128 AndNotSIMD( const __m128 & a, const __m128 & b )			// a & ~b
{
	return _mm_andnot_ps( a, b );
}

FORCEINLINE __m128 XorSIMD( const __m128 & a, const __m128 & b )				// a ^ b
{
	return _mm_xor_ps( a, b );
}

FORCEINLINE __m128 OrSIMD( const __m128 & a, const __m128 & b )				// a | b
{
	return _mm_or_ps( a, b );
}

FORCEINLINE __m128 AddSIMD( const __m128 & a, const __m128 & b )				// a+b
{
	return _mm_add_ps( a, b );
};

FORCEINLINE __m128 SubSIMD( const __m128 & a, const __m128 & b )				// a-b
{
	return _mm_sub_ps( a, b );
};

FORCEINLINE __m128 MulSIMD( const __m128 & a, const __m128 & b )				// a*b
{
	return _mm_mul_ps( a, b );
};

FORCEINLINE __m128 DivSIMD( const __m128 & a, const __m128 & b )				// a/b
{
	return _mm_div_ps( a, b );
};

FORCEINLINE __m128 MaddSIMD( const __m128 & a, const __m128 & b, const __m128 & c )				// a*b + c
{
	return AddSIMD( MulSIMD(a,b), c );
}

FORCEINLINE __m128 MsubSIMD( const __m128 & a, const __m128 & b, const __m128 & c )				// c - a*b
{
	return SubSIMD( c, MulSIMD(a,b) );
};

FORCEINLINE __m128 NegSIMD(const __m128 &a) // negate: -a
{
	return SubSIMD(LoadZeroSIMD(),a);
}

FORCEINLINE int TestSignSIMD( const __m128 & a )								// mask of which floats have the high bit set
{
	return _mm_movemask_ps( a );
}

FORCEINLINE bool IsAnyNegative( const __m128 & a )							// (a.x < 0) || (a.y < 0) || (a.z < 0) || (a.w < 0)
{
	return (0 != TestSignSIMD( a ));
}

FORCEINLINE __m128 CmpEqSIMD( const __m128 & a, const __m128 & b )				// (a==b) ? ~0:0
{
	return _mm_cmpeq_ps( a, b );
}

FORCEINLINE __m128 CmpGtSIMD( const __m128 & a, const __m128 & b )				// (a>b) ? ~0:0
{
	return _mm_cmpgt_ps( a, b );
}

FORCEINLINE __m128 CmpGeSIMD( const __m128 & a, const __m128 & b )				// (a>=b) ? ~0:0
{
	return _mm_cmpge_ps( a, b );
}

FORCEINLINE __m128 CmpLtSIMD( const __m128 & a, const __m128 & b )				// (a<b) ? ~0:0
{
	return _mm_cmplt_ps( a, b );
}

FORCEINLINE __m128 CmpLeSIMD( const __m128 & a, const __m128 & b )				// (a<=b) ? ~0:0
{
	return _mm_cmple_ps( a, b );
}

FORCEINLINE __m128 LoadAlignedSIMD( const void *pSIMD )
{
	return _mm_load_ps( reinterpret_cast< const float *> ( pSIMD ) );
}

FORCEINLINE __m128 ReciprocalEstSIMD( const __m128 & a )				// 1/a, more or less
{
	return _mm_rcp_ps( a );
}

/// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
/// No error checking!
FORCEINLINE __m128 ReciprocalSIMD( const __m128 & a )					// 1/a
{
	__m128 ret = ReciprocalEstSIMD( a );
	// newton iteration is: Y(n+1) = 2*Y(n)-a*Y(n)^2
	ret = SubSIMD( AddSIMD( ret, ret ), MulSIMD( a, MulSIMD( ret, ret ) ) );
	return ret;
}

#endif
