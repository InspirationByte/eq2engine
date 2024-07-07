#include "math_common.h"
#include "Transform.h"

Vector3D Transform3D::forward() const
{
	return rotateVector(vec3_forward, orientation);
}

Vector3D Transform3D::right() const
{
	return rotateVector(vec3_right, orientation);
}

Vector3D Transform3D::up() const
{
	return rotateVector(vec3_up, orientation);
}

Matrix4x4 Transform3D::toMatrix() const
{
	Matrix4x4 newTransform = orientation * scale3(scale.x, scale.y, scale.z);
	newTransform.setTranslationTransposed(origin);
	return newTransform;
}

Transform3D	Transform3D::operator!() const
{
	Transform3D invTrs;
	invTrs.orientation = inverse(orientation);
	invTrs.origin = -origin;
	invTrs.scale = 1.0f / invTrs.scale;
	return invTrs;
}
 
// rotates vector by transform
template <typename T>
TVec3D<T> rotateVector(const TVec3D<T>& in, const Transform3D& trs)
{
	return rotateVector(Vector3D(in), trs.orientation);
}

// transforms point by transform
template <typename T>
TVec3D<T> transformPoint(const TVec3D<T>& in, const Transform3D& trs)
{
	const Vector3D pointRot = rotateVector(in, trs) * trs.scale;
	return trs.origin + pointRot;
}

template Vector3D rotateVector(const Vector3D& in, const Transform3D& trs);
template Vector3D transformPoint(const Vector3D& in, const Transform3D& trs);
template FVector3D rotateVector(const FVector3D& in, const Transform3D& trs);
template FVector3D transformPoint(const FVector3D& in, const Transform3D& trs);