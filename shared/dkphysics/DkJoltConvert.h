#pragma once

namespace Convert
{
JPH::Float3 ToFloat3(const Vector3D& v);
Vector3D 	FromFloat3(const JPH::Float3& v);

JPH::Vec3 	ToVec3(const Vector3D& v);
Vector3D 	FromVec3(const JPH::Vec3& v);

JPH::Vec4 	ToVec4(const Vector4D& v);
Vector4D 	FromVec4(const JPH::Vec4& v);

JPH::Quat 	ToQuat(const Quaternion& v) ;
Quaternion 	FromQuat(const JPH::Quat& v);

JPH::Mat44 	ToMat44(const Matrix4x4& m);
Matrix4x4 	FromMat44(const JPH::Mat44& m);

JPH::Mat44 	ToMat44Transposed(const Matrix4x4& m);
Matrix4x4 	FromMat44Transposed(const JPH::Mat44& m);
}