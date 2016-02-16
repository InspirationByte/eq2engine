//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech fixed point math
//////////////////////////////////////////////////////////////////////////////////

#ifndef FIXEDMATH_H
#define FIXEDMATH_H

//#define FLOAT_AS_FREAL

#include "dktypes.h"

#ifdef FLOAT_AS_FREAL
#include <limits.h>
#include <float.h>
#include <math.h>

#include "math/math_common.h"

typedef float FReal;

namespace FPmath
{
const static FReal PI = 3.141592654f;

inline FReal abs(const FReal& f)	{return ::fabs(f);}

inline FReal floor(const FReal& f) {return ::floor(f);}
inline FReal ceil(const FReal& f)	{return ::ceil(f);}

inline FReal sqrt(const FReal& f)	{return ::sqrtf(f);}
inline FReal sqrt2(const FReal& f)	{return ::rsqrtf(f);}

inline FReal exp(const FReal& base, const FReal& pow) {return ::exp(base*pow);}

inline FReal cos(const FReal& rad) {return ::cos(rad);}
inline FReal sin(const FReal& rad) {return ::sin(rad);}
inline FReal tan(const FReal& rad) {return ::tan(rad);}

inline FReal acos(const FReal& f) {return ::acosf(f);}
inline FReal asin(const FReal& f) {return ::asinf(f);}
inline FReal atan(const FReal& f) {return ::atanf(f);}
inline FReal atan2(const FReal& f1, const FReal& f2) {return ::atan2f(f1,f2);}
};

#else

/*
This is a FReal point structure with an adjustable decimal place
	Author:			Waylon Grange
	Modified by:	Ilya Shurumov
*/

const int FPMATH_DECIMAL_BITS	=   16;
const int FPMATH_ONE			=	(1 << FPMATH_DECIMAL_BITS);
const int FPMATH_HALF			=	(1 << (FPMATH_DECIMAL_BITS - 1));

struct FReal
{
	int raw; // actual number

	//for directly setting value, use with caution
	FReal(int x, char null)					: raw(x) { (void)null; }

	//Constructors
	FReal() {}
	FReal(const FReal& x)					: raw(x.raw) {}
	FReal(int x)							: raw(x << FPMATH_DECIMAL_BITS) {}
	FReal(short x)							: raw(((signed long)x) << FPMATH_DECIMAL_BITS) {}
	FReal(float x)							: raw((int)(x * FPMATH_ONE)) {}
	FReal(double x)							: raw((int)(x * FPMATH_ONE)) {}

	operator int() const					{ return raw >> FPMATH_DECIMAL_BITS; }
	operator short() const					{ return raw >> FPMATH_DECIMAL_BITS; }
	operator float() const					{ return (float)raw/(float)FPMATH_ONE; }
	operator bool()	const					{ return raw!=0; }

	//Setters
	FReal& operator =(const FReal& x)		{ raw=x.raw; return *this; }
	FReal& operator =(int x)				{ raw=FReal(x).raw; return *this; }
	FReal& operator =(short x)				{ raw=FReal(x).raw; return *this; }
	FReal& operator =(float x)				{ raw=FReal(x).raw; return *this; }

	//Addition and Surawtraction
	FReal operator +(const FReal& x) const	{ return FReal(raw+x.raw,0); }
	FReal operator +(int x) const			{ return FReal(raw+FReal(x).raw,0); }
	FReal operator +(short x) const			{ return FReal(raw+FReal(x).raw,0); }
	FReal operator +(float x) const			{ return FReal(raw+FReal(x).raw,0); }

	FReal& operator +=(FReal x)				{ *this = *this + x; return *this; }
	FReal& operator +=(int x)				{ *this = *this + x; return *this; }
	FReal& operator +=(short x)				{ *this = *this + x; return *this; }
	FReal& operator +=(float x)				{ *this = *this + x; return *this; }

	FReal operator -(const FReal& x) const	{ return FReal(raw-x.raw,0); }
	FReal operator -(int x) const			{ return FReal(raw-FReal(x).raw,0); }
	FReal operator -(short x) const			{ return FReal(raw-FReal(x).raw,0); }
	FReal operator -(float x) const			{ return FReal(raw-FReal(x).raw,0); }
	FReal operator -() const				{ return FReal(-raw,0); }

	FReal& operator -=(FReal x)				{ *this = *this - x; return *this; }
	FReal& operator -=(int x)				{ *this = *this - x; return *this; }
	FReal& operator -=(short x)				{ *this = *this - x; return *this; }
	FReal& operator -=(float x)				{ *this = *this - x; return *this; }

	//Multiplication and Division
	FReal operator *(const FReal& x) const	{ return FReal(int64(raw)*int64(x.raw) >> FPMATH_DECIMAL_BITS, 0); }//{ return FReal((int)(((long long)raw*(long long)x.raw)>>DECIMAL_BITS),0); }
	FReal operator *(int x) const			{ return FReal(raw*x,0); }
	FReal operator *(short x) const			{ return FReal(raw*x,0); }
	FReal operator *(float x) const			{ return FReal((int64(raw)*int64(FReal(x).raw) >> FPMATH_DECIMAL_BITS),0); }

	FReal& operator *=(FReal x)				{ *this = *this * x; return *this; }
	FReal& operator *=(int x)				{ *this = *this * x; return *this; }
	FReal& operator *=(short x)				{ *this = *this * x; return *this; }
	FReal& operator *=(float x)				{ *this = *this * x; return *this; }

	FReal operator /(const FReal& x) const	{ return FReal(((int64(raw) << FPMATH_DECIMAL_BITS) / x.raw), 0);} //{ return FReal(((((long long)raw)<<(DECIMAL_BITS<<1))/x.raw)>>DECIMAL_BITS,0); }
	FReal operator /(int x) const			{ return FReal(raw/x,0); }
	FReal operator /(short x) const			{ return FReal(raw/x,0); }
	FReal operator /(float x) const			{ return FReal(((int64(raw) << FPMATH_DECIMAL_BITS) / FReal(x).raw), 0);} //{ return FReal(((((long long)raw)<<(DECIMAL_BITS<<1))/FReal(x).raw)>>DECIMAL_BITS,0); }

	FReal& operator /=(FReal x)				{ *this = *this / x; return *this; }
	FReal& operator /=(int x)				{ *this = *this / x; return *this; }
	FReal& operator /=(short x)				{ *this = *this / x; return *this; }
	FReal& operator /=(float x)				{ *this = *this / x; return *this; }

	FReal operator %(FReal x) const			{ return FReal(raw%x.raw,0); }
	FReal operator %(int x) const			{ return FReal(raw%FReal(x).raw,0); }
	FReal operator %(short x) const			{ return FReal(raw%FReal(x).raw,0); }
	FReal operator %(float x) const			{ return FReal(raw%FReal(x).raw,0); }

	//rawit operators
	FReal operator &(const FReal& x) const	{ return FReal(raw & x.raw, 0); }
	FReal operator &(int x) const			{ return FReal(raw & x,0); }
	FReal operator |(const FReal& x) const	{ return FReal(raw | x.raw,0); }
	FReal operator |(int x) const			{ return FReal(raw | x,0); }
	FReal operator ^(const FReal& x) const	{ return FReal(raw ^ x.raw); }
	FReal operator ^(int x) const			{ return FReal(raw ^ x,0); }
	FReal operator <<(int x) const			{ return FReal(raw<<x,0); }
	FReal operator >>(int x) const			{ return FReal(raw>>x,0); }
	FReal operator !() const				{ return FReal(!raw,0); }

	//Comparitors
	//TODO: It may rawe good to add a level of error to the float checks in the future
	bool operator ==(const FReal& x) const	{ return raw==x.raw; }
	bool operator ==(const int x) const		{ return raw==FReal(x).raw; }
	bool operator ==(const short x) const	{ return raw==FReal(x).raw; }
	bool operator ==(const float x) const	{ return raw==FReal(x).raw; }

	bool operator !=(const FReal& x) const	{ return raw!=x.raw; }
	bool operator !=(const int x) const		{ return raw!=FReal(x).raw; }
	bool operator !=(const short x) const	{ return raw!=FReal(x).raw; }
	bool operator !=(const float x) const	{ return raw!=FReal(x).raw; }

	bool operator <=(const FReal& x) const	{ return raw<=x.raw; }
	bool operator <=(const int x) const		{ return raw<=FReal(x).raw; }
	bool operator <=(const short x) const	{ return raw<=FReal(x).raw; }
	bool operator <=(const float x) const	{ return raw<=FReal(x).raw; }

	bool operator >=(const FReal& x) const	{ return raw>=x.raw; }
	bool operator >=(const int x) const		{ return raw>=FReal(x).raw; }
	bool operator >=(const short x) const	{ return raw>=FReal(x).raw; }
	bool operator >=(const float x) const	{ return raw>=FReal(x).raw; }

	bool operator <(const FReal& x) const	{ return raw<x.raw; }
	bool operator <(const int x) const		{ return raw<FReal(x).raw; }
	bool operator <(const short x) const	{ return raw<FReal(x).raw; }
	bool operator <(const float x) const	{ return raw<FReal(x).raw; }

	bool operator >(const FReal& x) const	{ return raw>x.raw; }
	bool operator >(const int x) const		{ return raw>FReal(x).raw; }
	bool operator >(const short x) const	{ return raw>FReal(x).raw; }
	bool operator >(const float x) const	{ return raw>FReal(x).raw; }
};

extern inline FReal operator +(int x, const FReal& y)	{ return FReal(x)+y; }
extern inline FReal operator +(short x, const FReal& y) { return FReal(x)+y; }
extern inline FReal operator +(float x, const FReal& y) { return FReal(x)+y; }

extern inline int& operator +=(int& x, const FReal& y)		{ x=FReal(x)+y; return x;}
extern inline short& operator +=(short& x, const FReal& y)	{ x=FReal(x)+y; return x;}
extern inline float& operator +=(float& x, const FReal& y)	{ x=FReal(x)+y; return x;}

extern inline FReal operator -(int x, const FReal& y)		{ return FReal(x)-y; }
extern inline FReal operator -(short x, const FReal& y)		{ return FReal(x)-y; }
extern inline FReal operator -(float x, const FReal& y)		{ return FReal(x)-y; }

extern inline int& operator -=(int& x, const FReal& y)		{ x=(FReal)x-y; return x;}
extern inline short& operator -=(short& x, const FReal& y)	{ x=(FReal)x-y; return x;}
extern inline float& operator -=(float& x, const FReal& y)	{ x=(FReal)x-y; return x;}

extern inline FReal operator *(int x, const FReal& y)		{ return FReal(x)*y; }
extern inline FReal operator *(short x, const FReal& y)		{ return FReal(x)*y; }
extern inline FReal operator *(float x, const FReal& y)		{ return FReal(x)*y; }

extern inline int& operator *=(int& x, const FReal& y)		{ x=(FReal)x*y; return x;}
extern inline short& operator *=(short& x, const FReal& y)	{ x=(FReal)x*y; return x;}
extern inline float& operator *=(float& x, const FReal& y)	{ x=(FReal)x*y; return x;}

extern inline FReal operator /(int x, const FReal& y)		{ return FReal(x)/y; }
extern inline FReal operator /(short x, const FReal& y)		{ return FReal(x)/y; }
extern inline FReal operator /(float x, const FReal& y)		{ return FReal(x)/y; }

extern inline int& operator /=(int& x, const FReal& y)		{ x=(FReal)x/y; return x;}
extern inline short& operator /=(short& x, const FReal& y)	{ x=(FReal)x/y; return x;}
extern inline float& operator /=(float& x, const FReal& y)	{ x=(FReal)x/y; return x;}

extern inline bool operator ==(int x, const FReal& y)		{ return FReal(x)==y; }
extern inline bool operator ==(short x, const FReal& y)		{ return FReal(x)==y; }
extern inline bool operator ==(float x, const FReal& y)		{ return FReal(x)==y; }

extern inline bool operator !=(int x, const FReal& y)		{ return FReal(x)!=y; }
extern inline bool operator !=(short x, const FReal& y)		{ return FReal(x)!=y; }
extern inline bool operator !=(float x, const FReal& y)		{ return FReal(x)!=y; }

extern inline bool operator <=(int x, const FReal& y)		{ return FReal(x)<=y; }
extern inline bool operator <=(short x, const FReal& y)		{ return FReal(x)<=y; }
extern inline bool operator <=(float x, const FReal& y)		{ return FReal(x)<=y; }

extern inline bool operator >=(int x, const FReal& y)		{ return FReal(x)>=y; }
extern inline bool operator >=(short x, const FReal& y)		{ return FReal(x)>=y; }
extern inline bool operator >=(float x, const FReal& y)		{ return FReal(x)>=y; }

extern inline bool operator <(int x, const FReal& y)		{ return FReal(x)<y; }
extern inline bool operator <(short x, const FReal& y)		{ return FReal(x)<y; }
extern inline bool operator <(float x, const FReal& y)		{ return FReal(x)<y; }

extern inline bool operator >(int x, const FReal& y)		{ return FReal(x)>y; }
extern inline bool operator >(short x, const FReal& y)		{ return FReal(x)>y; }
extern inline bool operator >(float x, const FReal& y)		{ return FReal(x)>y; }

namespace FPmath
{
const static FReal PI = 3.141592654f;

FReal abs(const FReal& f);
FReal sign(const FReal& f);

FReal floor(const FReal& f);
FReal ceil(const FReal& f);

FReal sqrt(const FReal& f);
FReal sqrt2(const FReal& f);

FReal exp(const FReal& base, const FReal& pow);

FReal cos(const FReal& rad);
FReal sin(const FReal& rad);
FReal tan(const FReal& rad);

FReal acos(const FReal& f);
FReal asin(const FReal& f);
FReal atan(const FReal& f);
FReal atan2(const FReal& f1, const FReal& f2);
};

#endif

#endif // FIXEDMATH_H
