#pragma once

struct Transform3D
{
	Vector3D		t{ vec3_zero };
	Quaternion		r{ qidentity };
	Vector3D		s{ 1.0f };

	Transform3D() = default;

	Transform3D(const Vector3D& origin, const Quaternion& orientation = qidentity, const Vector3D& scale = Vector3D(1.0f));
	Transform3D(const Vector3D& origin, const Vector3D& eulerAnglesXYZ = vec3_zero, const Vector3D& scale = Vector3D(1.0f));
	Transform3D(const Matrix4x4& matrix);

	Vector3D		forward() const;
	Vector3D		right() const;
	Vector3D		up() const;

	Matrix4x4		toMatrix() const;
	Transform3D		operator!() const;
};

// rotates vector by transform
template <typename T>
TVec3D<T> rotateVector(const TVec3D<T>& in, const Transform3D& trs);

// transforms point by transform
template <typename T>
TVec3D<T> transformPoint(const TVec3D<T>& in, const Transform3D& trs);

// transforms point by transform (inverse)
template <typename T>
TVec3D<T> transformPointInverse(const TVec3D<T>& in, const Transform3D& trs);

// inverts transform
Transform3D	inverse(const Transform3D& trs);