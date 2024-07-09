#include "math_common.h"
#include "Transform.h"

Transform3D::Transform3D(const Vector3D& origin, const Quaternion& orientation, const Vector3D& scale)
	: t(origin)
	, r(orientation)
	, s(scale)
{
}

Transform3D::Transform3D(const Matrix4x4& matrix)
{
	t = matrix.getTranslationComponent();

	Matrix3x3 rotMatrix = matrix.getRotationComponent();
	s.x = length(rotMatrix.rows[0]);
	s.y = length(rotMatrix.rows[1]);
	s.z = length(rotMatrix.rows[2]);

	const float invScaleX = (s.x > F_EPS) ? 1.0f / s.x : FLT_MAX;
	const float invScaleY = (s.y > F_EPS) ? 1.0f / s.y : FLT_MAX;
	const float invScaleZ = (s.z > F_EPS) ? 1.0f / s.z : FLT_MAX;

	rotMatrix.rows[0] *= invScaleX;
	rotMatrix.rows[1] *= invScaleY;
	rotMatrix.rows[2] *= invScaleZ;
	r = rotMatrix;
}

Transform3D::Transform3D(const Vector3D& origin, const Vector3D& eulerAnglesXYZ, const Vector3D& scale)
	: t(origin)
	, r(rotateXYZ(eulerAnglesXYZ.x, eulerAnglesXYZ.y, eulerAnglesXYZ.z))
	, s(scale)
{
}

Vector3D Transform3D::forward() const
{
	return rotateVector(vec3_forward, r);
}

Vector3D Transform3D::right() const
{
	return rotateVector(vec3_right, r);
}

Vector3D Transform3D::up() const
{
	return rotateVector(vec3_up, r);
}

Matrix4x4 Transform3D::toMatrix() const
{
	Matrix4x4 newTransform = r * scale3(s.x, s.y, s.z);
	newTransform.setTranslationTransposed(t);
	return newTransform;
}

Transform3D	Transform3D::operator!() const
{
	Transform3D invTrs;
	invTrs.r = inverse(r);
	invTrs.t = -t;
	invTrs.s = 1.0f / s;
	return invTrs;
}

Transform3D	inverse(const Transform3D& trs)
{
	return !trs;
}
 
template <typename T>
TVec3D<T> rotateVector(const TVec3D<T>& in, const Transform3D& trs)
{
	return rotateVector(Vector3D(in), trs.r);
}

template <typename T>
TVec3D<T> transformPoint(const TVec3D<T>& in, const Transform3D& trs)
{
	const Vector3D pointRot = rotateVector(Vector3D(in), trs.r) * trs.s;
	return trs.t + pointRot;
}

template <typename T>
TVec3D<T> transformPointInverse(const TVec3D<T>& in, const Transform3D& trs)
{
	Transform3D invTrs = inverse(trs);
	const Vector3D pointRot = rotateVector(Vector3D(in) + invTrs.t, invTrs.r) * invTrs.s;
	return pointRot;
}

template Vector3D rotateVector(const Vector3D& in, const Transform3D& trs);
template FVector3D rotateVector(const FVector3D& in, const Transform3D& trs);
template Vector3D transformPoint(const Vector3D& in, const Transform3D& trs);
template FVector3D transformPoint(const FVector3D& in, const Transform3D& trs);
template Vector3D transformPointInverse(const Vector3D& in, const Transform3D& trs);
template FVector3D transformPointInverse(const FVector3D& in, const Transform3D& trs);