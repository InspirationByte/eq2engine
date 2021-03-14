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
#include <LinearMath/btTransform.h>

#include "math/Vector.h"
#include "math/Matrix.h"

///< converts Equilibrium units to Bullet units
///< input: any typed value with standard operators
#define EQ2BULLET(x) x

///< converts bullet physics units to Equilibrium units
///< input: any typed value with standard operators
#define BULLET2EQ(x) x

namespace EqBulletUtils
{
inline void ConvertDKToBulletVectors(btVector3& out, const Vector3D &v)
{
	out.setValue(v[0], v[1], v[2]);
}

inline void ConvertBulletToDKVectors(Vector3D& out, const btVector3 &v)
{
	out.x = v.m_floats[0];
	out.y = v.m_floats[1];
	out.z = v.m_floats[2];
}

inline void ConvertPositionToBullet(btVector3& out, const Vector3D &v)
{
	out.setValue(EQ2BULLET(v[0]), EQ2BULLET(v[1]), EQ2BULLET(v[2]));
}

inline void ConvertPositionToEq(Vector3D& out, const btVector3 &v)
{
	out.x = BULLET2EQ(v.m_floats[0]);
	out.y = BULLET2EQ(v.m_floats[1]);
	out.z = BULLET2EQ(v.m_floats[2]);
}

inline void ConvertMatrix4ToBullet(btTransform& out, const Matrix4x4 &matrix)
{
	out.setFromOpenGLMatrix(matrix);
}

inline void ConvertMatrix4ToEq(Matrix4x4& out, const btTransform &transform)
{
	transform.getOpenGLMatrix((float*)&out);
}

inline void ConvertQuaternionToBullet(btQuaternion& out, const Quaternion &quat)
{
	out.setValue(quat.x,quat.y,quat.z,quat.w);
}

inline void ConvertQuaternionToEq(Quaternion& out, const btQuaternion &quat)
{
	out.w = quat.getW();
	out.x = quat.getX();
	out.y = quat.getY();
	out.z = quat.getZ();
}

}

#endif // BULLET_CONVERT_H
