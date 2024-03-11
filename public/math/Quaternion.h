//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	Quaternion(const float iw, const float ix, const float iy, const float iz) 
		: w(iw), x(ix),y(iy),z(iz)
	{
	}

	Quaternion(const Matrix3x3& m);
	Quaternion(const Vector4D& v);
	Quaternion(const float a, const Vector3D& axis);

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
Quaternion operator * (const Vector3D& v, const Quaternion& q);
Quaternion operator ! (const Quaternion &q);

// ------------------------------------------------------------------------------

// constructs quaternion with rotation around X axis
Quaternion		rotateX(float angle);

// constructs quaternion with rotation around Y axis
Quaternion		rotateY(float angle);

// constructs quaternion with rotation around Z axis
Quaternion		rotateZ(float angle);

// constructs quaternion with rotation in X-Y order
Quaternion		rotateXY(float x, float y);

// constructs quaternion with rotation in X-Y-Z order
Quaternion		rotateXYZ(float x, float y, float z);

// constructs quaternion with rotation in Z-X-Y order
Quaternion		rotateZXY(float x, float y, float z);

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
Vector3D		quaternionToEulers(const Quaternion& q, EQuatRotationSequence seq);

// recalculates w comp
void			renormalize(Quaternion& q);

// axis angle of quaternion
void			axisAngle(const Quaternion& q, Vector3D& axis, float& angle);

// compares quaternion with epsilon
bool			quaternionSimilar(const Quaternion& u, const Quaternion& v, const float eps);

// vector rotation
Vector3D		rotateVector( const Vector3D& p, const Quaternion& q );

static const Quaternion qidentity = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
