#pragma once

struct Transform3D
{
	Quaternion		orientation{ qidentity };
	Vector3D		origin{ vec3_zero };
	Vector3D		scale{ 1.0f };

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