//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Inline code for conversion of matrices and vectors
//////////////////////////////////////////////////////////////////////////////////

#ifndef BULLET_CONVERT_H
#define BULLET_CONVERT_H

// Bullet headers
///Math library & Utils
#include <bullet/LinearMath/btTransform.h>

#include "math/Vector.h"
#include "math/Matrix.h"

namespace EqBulletUtils
{

///< converts Equilibrium units to Bullet units
///< input: any typed value with standard operators
inline btScalar EQ2BULLET(float x)
{
	return x; // * (T)METERS_PER_UNIT
}

///< converts bullet physics units to Equilibrium units
///< input: any typed value with standard operators
inline float BULLET2EQ(btScalar x)
{
	return x; // * (T)(1.0f/METERS_PER_UNIT))
}

inline btVector3 ConvertDKToBulletVectors(const Vector3D &v)
{
	return btVector3(v[0],v[1],v[2]);
}

inline Vector3D ConvertBulletToDKVectors(const btVector3 &v)
{
	return Vector3D(v.m_floats[0],v.m_floats[1],v.m_floats[2]);
}

inline btVector3 ConvertPositionToBullet(const Vector3D &v)
{
	return btVector3(EQ2BULLET(v[0]),EQ2BULLET(v[1]),EQ2BULLET(v[2]));
}

inline Vector3D ConvertPositionToEq(const btVector3 &v)
{
	return Vector3D(BULLET2EQ(v.m_floats[0]),BULLET2EQ(v.m_floats[1]),BULLET2EQ(v.m_floats[2]));
}

inline btTransform ConvertMatrix4ToBullet(const Matrix4x4 &matrix)
{
	btTransform out;
	out.setFromOpenGLMatrix(matrix);
	//out.setOrigin(out.getOrigin());

	return out;
}

inline Matrix4x4 ConvertMatrix4ToEq(const btTransform &transform)
{
	btTransform temp = transform;
	temp.setOrigin(temp.getOrigin());

	Matrix4x4 out;
	temp.getOpenGLMatrix((float*)&out);

	return out;
}

inline btQuaternion ConvertQuaternionToBullet(const Quaternion &quat)
{
	btQuaternion out;
	out.setValue(quat.x,quat.y,quat.z,quat.w);

	return out;
}

inline Quaternion ConvertQuaternionToEq(const btQuaternion &quat)
{
	Quaternion out(quat.getW(), quat.getX(),quat.getY(),quat.getZ());

	return out;
}

}

#endif // BULLET_CONVERT_H
