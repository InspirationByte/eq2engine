//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Quaternion math class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//--------------------------------------------------------
// Note: 
//--------------------------------------------------------
// return values of res[] depends on rotSeq.
// i.e.
// for rotSeq zyx, 
// x = res[0], y = res[1], z = res[2]
// for rotSeq xyz
// z = res[0], y = res[1], x = res[2]
// ...
enum EQuatRotationSequence
{
	QuatRot_zyx,
	QuatRot_zxy,
	QuatRot_yxz,
	QuatRot_yzx,
	QuatRot_xyz,
	QuatRot_xzy,
};

struct Quaternion
{
	float x,y,z,w;

	Quaternion() {}
	Quaternion(const float Wx, const float Wy);
	Quaternion(const float Wx, const float Wy, const float Wz);
	Quaternion(const float iw, const float ix, const float iy, const float iz) 
		: w(iw), x(ix),y(iy),z(iz)
	{
	}

	Quaternion(const Matrix3x3& m);
	Quaternion(const Vector4D& v);
	Quaternion(const float a, const TVec3D<float>& axis);

	operator float *() const { return (float *) &x; }

	Vector4D& asVector4D() const;

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
Quaternion operator * (float scalar, const Quaternion &v);
Quaternion operator * (const Quaternion &v, const float scalar);
Quaternion operator / (const Quaternion &v, const float dividend);
Quaternion operator * (const TVec3D<float>& v, const Quaternion& q);
Quaternion operator ! (const Quaternion &q);

// ------------------------------------------------------------------------------

// interpolates quaternion
Quaternion		slerp(const Quaternion &q0, const Quaternion &q1, const float t);
Quaternion		scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t);

// finds inverse of quaternion
Quaternion		inverse(const Quaternion& q);

// length of quaternion
float			length(const Quaternion &q);

// returns euler angles of quaternion
Vector3D		eulersXYZ(const Quaternion &q);

// stores euler angles of quaternion to res
void			quaternionToEulers(const Quaternion& q, EQuatRotationSequence seq, float res[3]);

// recalculates w comp
void			renormalize(Quaternion& q);

// axis angle of quaternion
void			axisAngle(const Quaternion& q, Vector3D&axis, float &angle);

// compares quaternion with epsilon
bool			compare_epsilon(const Quaternion &u, const Quaternion &v, const float eps);

// vector rotation
Vector3D		rotateVector( const Vector3D& p, const Quaternion& q );

// quaternion identity
Quaternion		identity();
