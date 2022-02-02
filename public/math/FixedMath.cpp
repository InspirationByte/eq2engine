//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point math
//////////////////////////////////////////////////////////////////////////////////

#include "FixedMath.h"
#include "Vector.h"

#ifndef FLOAT_AS_FREAL

FReal FPmath::abs(const FReal& f)
{
    if (f < 0) return -f; return f;
}

FReal FPmath::sign(const FReal& f)
{
	return ((f.raw > 0) ? 1 : (f.raw < 0)? -1 : 0);
}

FReal FPmath::floor(const FReal& f)
{
    return (f & (-1 << FPMATH_DECIMAL_BITS));
}

FReal FPmath::ceil(const FReal& f)
{
    return (f & (FPMATH_ONE -1)) ? (f & (-1 << FPMATH_DECIMAL_BITS)) + 1 : f;
}

FReal FPmath::sqrt(const FReal& f)
{
    FReal s,r;

    if (f < FPMATH_ONE)
	{
		return 0;
	}

    s = f >> 2;

    do
    {
        r=s;
        s = ( s + f/s) >> 1;
    } while (r != s );

    return r;
}

//A fast but less acurate method
//This method is very cheap and runs near the spped of a divide operation!
//Use where possible
FReal FPmath::sqrt2(const FReal& f)
{
    int op=f.raw;
    int res=0;

    if (f < FPMATH_ONE)
	{
		return 0;
	}

    /* "one" starts at the highest power of four <= than the argument. */
    int one = 1 << 30;  /* second-to-top bit set */
    while (one > op) one >>= 2;

    while (one != 0)
	{
        if (op >= res + one)
		{
            op = op - (res + one);
            res = res + (one << 1);  // <-- faster than 2 * one
        }
        res >>= 1;
        one >>= 2;
    }

    res <<= 8;
    return FReal(res,0);
}


// Return an approx to sin(rad)
// It has a max absolute error of 5e-9
// according to Hastings, Approximations For Digital Computers.
// Since we are using fixed-point the accuracy is more around 5e-6
FReal FPmath::sin(const FReal& rad)
{
    FReal x = (rad * 2 ) / FPmath::PI;

    //These improve accuracy
    while (x >  1) {x-=2;}
    while (x < -1) {x+=2;}

    FReal x2 = x * x;
    return ((((x2 * FReal(10,0) - FReal(306,0)) * x2 + FReal(5222,0)) * x2
        - FReal(42333,0)) * x2 + FReal(102942,0)) * x;
}


FReal FPmath::cos(const FReal& rad)
{
    return FPmath::sin(rad+ (PI>>1));
}

FReal FPmath::tan( const FReal& rad)
{
    //TODO: there is probibly a faster way to compute this
    FReal t = cos(rad);
    if (t == 0)
        return t; //TODO: I really should return something instead of 0 for NaN

    return sin(rad)/t;
}

#endif // FLOAT_AS_FREAL