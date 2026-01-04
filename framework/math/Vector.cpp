//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Vector math base
//////////////////////////////////////////////////////////////////////////////////

#include "math_common.h"
#include "Vector.h"

#ifndef FLOAT16_BUILTIN
half::half(const float x)
{
	union 
	{
		float floatI;
		unsigned int i;
	};

	floatI = x;

//	unsigned int i = *((unsigned int *) &x);
	int e = ((i >> 23) & 0xFF) - 112;
	int m =  i & 0x007FFFFF;

	sh = (i >> 16) & 0x8000;
	if (e <= 0)
	{
		// Denorm
		m = ((m | 0x00800000) >> (1 - e)) + 0x1000;
		sh |= (m >> 13);
	} 
	else if (e == 143)
	{
		sh |= 0x7C00;
		if (m != 0)
		{
			// NAN
			m >>= 13;
			sh |= m | (m == 0);
		}
	} 
	else 
	{
		m += 0x1000;
		if (m & 0x00800000)
		{
			// Mantissa overflow
			m = 0;
			e++;
		}
		if (e >= 31)
		{
			// Exponent overflow
			sh |= 0x7C00;
		}
		else 
		{
			sh |= (e << 10) | (m >> 13);
		}
	}
}

half::operator float () const 
{
	union 
	{
		unsigned int s;
		float result;
	};

	s = (sh & 0x8000) << 16;
	unsigned int e = (sh >> 10) & 0x1F;
	unsigned int m = sh & 0x03FF;

	if (e == 0)
	{
		// +/- 0
		if (m == 0) return result;

		// Denorm
		while ((m & 0x0400) == 0)
		{
			m += m;
			e--;
		}
		e++;
		m &= ~0x0400;
	} 
	else if (e == 31)
	{
		// INF / NAN
		s |= 0x7F800000 | (m << 13);
		return result;
	}

	s |= ((e + 112) << 23) | (m << 13);

	return result;
}

#endif // !FLOAT16_BUILTIN

/* --------------------------------------------------------------------------------- */

//Helper method to emulate GLSL
float fract(float value)
{
    return (float)fmod(value, 1.0f);
}

void AngleVectors(const Vector3D& angles, Vector3D& forward)
{
	float cp,cy,sp,sy;
	SinCos(DEG2RAD(-angles.x),&sp,&cp);
	SinCos(DEG2RAD(-angles.y),&sy,&cy);

	forward.x = cp*sy;
	forward.y = sp;
	forward.z = cp*cy;
}

void AngleVectors(const Vector3D& angles, Vector3D& forward, Vector3D& right)
{
	float cp,cy,sp,sy,sr,cr;
	SinCos(DEG2RAD(-angles.x),&sp,&cp);
	SinCos(DEG2RAD(-angles.y),&sy,&cy);
	SinCos(DEG2RAD(-angles.z),&sr,&cr);

	forward.x = cp*sy;
	forward.y = sp;
	forward.z = cp*cy;

	const float cycr = cy*cr;
	const float sysr = sy*sr;

	right.x = cycr+sp*sysr;
	right.y = -cp*sr;
	right.z = sp*cy*sr-sy*cr;
}

void AngleVectors(const Vector3D& angles, Vector3D& forward, Vector3D& right, Vector3D& up)
{
	float cp,cy,sp,sy,sr,cr;
	SinCos(DEG2RAD(-angles.x),&sp,&cp);
	SinCos(DEG2RAD(-angles.y),&sy,&cy);
	SinCos(DEG2RAD(-angles.z),&sr,&cr);

	forward.x = cp*sy;
	forward.y = sp;
	forward.z = cp*cy;

	const float sycr = sy*cr;
	const float cycr = cy*cr;
	const float sysr = sy*sr;
	const float cysr = cy*sr;

	right.x = cycr+sp*sysr;
	right.y = -cp*sr;
	right.z = sp*cysr-sycr;

	up.x = cysr-sp*sycr;
	up.y = cp*cr;
	up.z = -sysr-sp*cycr;
}

Vector3D VectorAngles(const Vector3D& forward)
{
	const float y = fmodf(-atan2f(forward.x, forward.z) + M_PI_2_F, M_PI_2_F);

	const float z1 = sqrtf(forward.x * forward.x + forward.z * forward.z);
	const float x = fmodf(atan2f(z1, forward.y) - M_PI_F * 0.5f + M_PI_2_F, M_PI_2_F);

	return Vector3D(x, y, 0.0f) * M_RAD2DEG;
}

void VectorVectors( const Vector3D &forward, Vector3D &right, Vector3D &up )
{
	if (forward.x == 0 && forward.y == 0)
	{
		// pitch 90 degrees up/down from identity
		right = Vector3D(-forward.z, 0.0f, 0.0f);
		up = vec3_up;
		return;
	}

	right = normalize(cross( forward, vec3_forward ));
	up = normalize(cross( right, forward ));
}

unsigned int toRGBA(const MColor& u)
{
	return (int(u.r * 255.0f) | (int(u.g * 255.0f) << 8) | (int(u.b * 255.0f) << 16) | (int(u.a * 255.0f) << 24));
}

unsigned int toBGRA(const MColor& u)
{
	return (int(u.b * 255) | (int(u.g * 255) << 8) | (int(u.r * 255) << 16) | (int(u.a * 255) << 24));
}

MColor rgbeToRGB(unsigned char *rgbe)
{
	if (rgbe[3])
	{
		Vector3D clr(rgbe[0], rgbe[1], rgbe[2]);
		clr *= ldexpf(1.0f, rgbe[3] - (int)(128 + 8));
		return MColor(clr);
	} 
	else
		return MColor(0, 0, 0);
}

unsigned int rgbToRGBE8(const MColor &rgb)
{
	float v = max(rgb.r, rgb.g);
	v = max(v, rgb.b);

	if (v < 1e-32f)
	{
		return 0;
	} 
	else 
	{
		int ex;
		float m = frexpf(v, &ex) * 256.0f / v;

		unsigned int r = (unsigned int) (m * rgb.r);
		unsigned int g = (unsigned int) (m * rgb.g);
		unsigned int b = (unsigned int) (m * rgb.b);
		unsigned int e = (unsigned int) (ex + 128);

		return r | (g << 8) | (b << 16) | (e << 24);
	}
}

unsigned int rgbToRGB9E5(const MColor &rgb)
{
	float v = max(rgb.r, rgb.g);
	v = max(v, rgb.b);

	if (v < 1.52587890625e-5f)
	{
		return 0;
	} 
	else if (v < 65536)
	{
		int ex;
		float m = frexpf(v, &ex) * 512.0f / v;

		unsigned int r = (unsigned int) (m * rgb.r);
		unsigned int g = (unsigned int) (m * rgb.g);
		unsigned int b = (unsigned int) (m * rgb.b);
		unsigned int e = (unsigned int) (ex + 15);

		return r | (g << 9) | (b << 18) | (e << 27);
	} 
	else
	{
		unsigned int r = (rgb.r < 65536)? (unsigned int) (rgb.r * (1.0f / 128.0f)) : 0x1FF;
		unsigned int g = (rgb.g < 65536)? (unsigned int) (rgb.g * (1.0f / 128.0f)) : 0x1FF;
		unsigned int b = (rgb.b < 65536)? (unsigned int) (rgb.b * (1.0f / 128.0f)) : 0x1FF;
		unsigned int e = 31;

		return r | (g << 9) | (b << 18) | (e << 27);
	}
}

uint lerpRGBAPacked(uint a, uint b, float v)
{ 
   constexpr uint MASK1 = 0x00ff00ff; 
   constexpr uint MASK2 = 0xff00ff00; 

   const uint f2 = 256 * v;
   const uint f1 = 256 - f2;

   return   ((((( a & MASK1 ) * f1 ) + ( ( b & MASK1 ) * f2 )) >> 8 ) & MASK1 ) 
          | ((((( a & MASK2 ) * f1 ) + ( ( b & MASK2 ) * f2 )) >> 8 ) & MASK2 );
}