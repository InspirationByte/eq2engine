#include "core/core_common.h"

#include "DkJoltPCH.h"
#include "DkJoltConvert.h"

namespace Convert
{
JPH::Float3 ToFloat3(const Vector3D& v) { return JPH::Float3(v.x, v.y, v.z); }
Vector3D 	FromFloat3(const JPH::Float3& v) { return Vector3D(v[0], v[1], v[2]); }

JPH::Vec3 	ToVec3(const Vector3D& v) { return JPH::Vec3(v.x, v.y, v.z); }
Vector3D 	FromVec3(const JPH::Vec3& v) { return Vector3D(v[0], v[1], v[2]); }

JPH::Vec4 	ToVec4(const Vector4D& v) { return JPH::Vec4(v.x, v.y, v.z, v.w); }
Vector4D 	FromVec4(const JPH::Vec4& v) { return Vector4D(v[0], v[1], v[2], v[3]); }

JPH::Quat 	ToQuat(const Quaternion& v) { return JPH::Quat(v.x, v.y, v.z, v.w); }
Quaternion 	FromQuat(const JPH::Quat& v) { return Quaternion(v.mValue[3], v.mValue[0], v.mValue[1], v.mValue[2]); }

JPH::Mat44 	ToMat44(const Matrix4x4& m)
{
	return JPH::Mat44(ToVec4(m.r1), ToVec4(m.r2), ToVec4(m.r3), ToVec4(m.r4));
}

Matrix4x4 	FromMat44(const JPH::Mat44& m)
{
	return Matrix4x4(FromVec4(m.GetColumn4(0)), FromVec4(m.GetColumn4(1)), FromVec4(m.GetColumn4(2)), FromVec4(m.GetColumn4(3)));
}

JPH::Mat44 	ToMat44Transposed(const Matrix4x4& m)
{
	Matrix4x4 tm = transpose(m);
	return JPH::Mat44(ToVec4(tm.r1), ToVec4(tm.r2), ToVec4(tm.r3), ToVec4(tm.r4));
}

Matrix4x4 	FromMat44Transposed(const JPH::Mat44& m)
{
	JPH::Mat44 tm = m.Transposed();
	return Matrix4x4(FromVec4(tm.GetColumn4(0)), FromVec4(tm.GetColumn4(1)), FromVec4(tm.GetColumn4(2)), FromVec4(tm.GetColumn4(3)));
}
}