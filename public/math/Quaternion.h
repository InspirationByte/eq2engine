//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Quaternion math class
//////////////////////////////////////////////////////////////////////////////////

#ifndef QUATERNION_H
#define QUATERNION_H

#ifndef FORWARD_DECLARED
template<class T>
struct TMat4;

template<class T>
struct TVec3D;

template<class T>
struct TVec4D;
#else
#include "Matrix.h"
#endif // FORWARD_DECLARED

struct Quaternion
{
	float x,y,z,w;

	Quaternion() {}
	Quaternion(const float Wx, const float Wy);
	Quaternion(const float Wx, const float Wy, const float Wz);
	Quaternion(const float iw, const float ix, const float iy, const float iz)
	{
		w = iw;
		x = ix;
		y = iy;
		z = iz;
	}
	Quaternion(const TMat4<float> &m);
	Quaternion(const TVec4D<float> &v);
	Quaternion(const float a, const TVec3D<float>& axis);


	operator const float *() const
	{
		return &x;
	}

	TVec4D<float> asVector4D();

	void operator += (const Quaternion &v);
	void operator -= (const Quaternion &v);
	void operator *= (const float s);
	void operator /= (const float d);

	void normalize();
	void fastNormalize();

	bool isNan() const;
};

Quaternion operator + (const Quaternion &u, const Quaternion &v);
Quaternion operator - (const Quaternion &u, const Quaternion &v);
Quaternion operator - (const Quaternion &v);
Quaternion operator * (const Quaternion &u, const Quaternion &v);
Quaternion operator * (const float scalar, const Quaternion &v);
Quaternion operator * (const Quaternion &v, const float scalar);
Quaternion operator / (const Quaternion &v, const float dividend);
Quaternion operator * (const TVec3D<float>& v, const Quaternion &q);

// ------------------------------------------------------------------------------

// interpolates quaternion
Quaternion	slerp(const Quaternion &q0, const Quaternion &q1, const float t);
Quaternion	scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t);

// length of quaternion
float		length(const Quaternion &q);

// returns euler angles of quaternion
TVec3D<float>	eulers(const Quaternion &q);

// recalculates w comp
void		renormalize(Quaternion& q);

// axis angle of quaternion
void		axisAngle(Quaternion& q, TVec3D<float> &axis, float &angle);

// compares quaternion with epsilon
bool		compare_epsilon(const Quaternion &u, const Quaternion &v, const float eps);

// vector rotation
//Vector4D	quatRotate(Vector3D &p, Quaternion &q )

// quaternion identity
Quaternion	identity();

#endif // QUATERNION_H
