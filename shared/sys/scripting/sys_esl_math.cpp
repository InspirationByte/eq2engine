//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "math/Utility.h"
#include "math/Random.h"

#include "scripting/esl.h"
#include "scripting/esl_luaref.h"
#include "scripting/esl_bind.h"

#include "sys_esl.h"
#include "sys_esl_math.h"

static void VecToStringImpl(const IVector2D& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "{ x = %d, y = %d }", v.x, v.y);
}

static void VecToStringImpl(const Vector2D& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "{ x = %g, y = %g }", v.x, v.y);
}

static void VecToStringImpl(const Vector3D& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "{x = %g, y = %g, z = %g}", v.x, v.y, v.z);
}

static void VecToStringImpl(const Vector4D& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "{x = %g, y = %g, z = %g, w = %g }", v.x, v.y, v.z, v.w);
}

static void VecToStringImpl(const Quaternion& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "{x = %g, y = %g, z = %g, w = %g }", v.x, v.y, v.z, v.w);
}

template<typename T, esl::binder::EOpType OpType>
int VectorOperatorsFunc(lua_State* L)
{
	constexpr int opIdxA = 1;
	constexpr int opIdxB = 2;

	if constexpr (OpType == esl::binder::OP_unm)
	{
		// NOTE: not performing additional type checking
		esl::runtime::New<T>(L, -*esl::runtime::PushGet<T>::Get(L, opIdxA, false, {}));
		return 1;
	}
	else if constexpr (OpType == esl::binder::OP_not)
	{
		// NOTE: not performing additional type checking
		lua_pushboolean(L, !*esl::runtime::PushGet<T>::Get(L, opIdxA, false, {}));
		return 1;
	}
	else
	{
		T tmp;
		const T* lhs = &tmp;
		const T* rhs = &tmp;
		if (lua_type(L, opIdxA) == LUA_TNUMBER)
		{
			tmp = lua_tonumber(L, opIdxA);
			rhs = *esl::runtime::GetValue<const T*, false, false>(L, opIdxB);
		}
		else if(lua_type(L, opIdxB) == LUA_TNUMBER)
		{
			lhs = *esl::runtime::GetValue<const T*, false, false>(L, opIdxA);
			tmp = lua_tonumber(L, opIdxB);
		}
		else
		{
			lhs = *esl::runtime::GetValue<const T*, false, false>(L, opIdxA);
			rhs = *esl::runtime::GetValue<const T*, false, false>(L, opIdxB);
		}

		if constexpr (OpType == esl::binder::OP_add)
			esl::runtime::New<T>(L, *lhs + *rhs);
		else if constexpr (OpType == esl::binder::OP_sub)
			esl::runtime::New<T>(L, *lhs - *rhs);
		else if constexpr (OpType == esl::binder::OP_mul)
			esl::runtime::New<T>(L, *lhs * *rhs);
		else if constexpr (OpType == esl::binder::OP_div)
			esl::runtime::New<T>(L, *lhs / *rhs);
		else if constexpr (OpType == esl::binder::OP_mod)
			esl::runtime::New<T>(L, *lhs % *rhs);
		else if constexpr (OpType == esl::binder::OP_band)
			esl::runtime::New<T>(L, *lhs & *rhs);
		else if constexpr (OpType == esl::binder::OP_bor)
			esl::runtime::New<T>(L, *lhs | *rhs);
		else if constexpr (OpType == esl::binder::OP_xor)
			esl::runtime::New<T>(L, *lhs ^ *rhs);
		else if constexpr (OpType == esl::binder::OP_shl)
			esl::runtime::New<T>(L, *lhs << *rhs);
		else if constexpr (OpType == esl::binder::OP_shr)
			esl::runtime::New<T>(L, *lhs >> *rhs);
		else if constexpr (OpType == esl::binder::OP_eq)
			lua_pushboolean(L, *lhs == *rhs);
		else if constexpr (OpType == esl::binder::OP_lt)
			lua_pushboolean(L, *lhs < *rhs);
		else if constexpr (OpType == esl::binder::OP_le)
			lua_pushboolean(L, *lhs <= *rhs);
		else
			static_assert(sizeof(T) > 0, "Unsupported operator type");
		return 1;
	}
}

#define BIND_VECTOR_OPERATORS() \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, eq)  \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, lt)  \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, le)  \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, unm) \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, add) \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, sub) \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, mul) \
	EQSCRIPT_BIND_OP_CUSTOM(VectorOperatorsFunc, div)
	

//
// Vector2D
//
EQSCRIPT_TYPE_BEGIN( IVector2D )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(int)
	EQSCRIPT_BIND_CONSTRUCTOR(int, int)
	EQSCRIPT_BIND_CONSTRUCTOR(const IVector2D&)

	BIND_VECTOR_OPERATORS()
	EQSCRIPT_BIND_OP_TOSTRING(VecToStringImpl)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("sign", sign, IVector2D, (const IVector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("clamp", clamp, IVector2D, (const IVector2D&, const IVector2D&, const IVector2D&))
EQSCRIPT_TYPE_END

//
// Vector2D
//

EQSCRIPT_TYPE_BEGIN( Vector2D )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const IVector2D&)

	BIND_VECTOR_OPERATORS()
	EQSCRIPT_BIND_OP_TOSTRING(VecToStringImpl)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lerp", lerp, Vector2D, (const Vector2D&, const Vector2D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("cerp", cerp, Vector2D, (const Vector2D&, const Vector2D&, const Vector2D&, const Vector2D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("sign", sign, Vector2D, (const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("clamp",clamp, Vector2D, (const Vector2D&, const Vector2D&, const Vector2D&))

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distance", distance, float, (const Vector2D&, const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distanceSqr", distance, float, (const Vector2D&, const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("length", length, float, (const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lengthSqr", lengthSqr, float, (const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("dot", dot, float, (const Vector2D&, const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("normalize", normalize, Vector2D, (const Vector2D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lineProjection", lineProjection, float, (const Vector2D&, const Vector2D&, const Vector2D&))
EQSCRIPT_TYPE_END

//
// Vector3D
//
EQSCRIPT_TYPE_BEGIN( Vector3D )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, const Vector2D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&)

	BIND_VECTOR_OPERATORS()
	EQSCRIPT_BIND_OP_TOSTRING(VecToStringImpl)

	EQSCRIPT_BIND_FUNC(xy)
	EQSCRIPT_BIND_FUNC(yz)
	EQSCRIPT_BIND_FUNC(xz)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)
	EQSCRIPT_BIND_VAR(z)

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lerp", lerp, Vector3D, (const Vector3D&, const Vector3D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("cerp", cerp, Vector3D, (const Vector3D&, const Vector3D&, const Vector3D&, const Vector3D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("sign", sign, Vector3D, (const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("clamp", clamp, Vector3D, (const Vector3D&, const Vector3D&, const Vector3D&))

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distance", distance, float, (const Vector3D&, const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distanceSqr", distanceSqr, float, (const Vector3D&, const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("length", length, float, (const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lengthSqr", lengthSqr, float, (const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("dot", dot, float, (const Vector3D&, const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("normalize", normalize, Vector3D, (const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lineProjection", lineProjection, float, (const Vector3D&, const Vector3D&, const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("cross", cross, Vector3D, (const Vector3D&, const Vector3D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("reflect", reflect, Vector3D, (const Vector3D&, const Vector3D&))
EQSCRIPT_TYPE_END

//
// Vector4D
//
EQSCRIPT_TYPE_BEGIN( Vector4D )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, const Vector2D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, const Vector2D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&, const Vector2D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, const Vector3D&)

	BIND_VECTOR_OPERATORS()
	EQSCRIPT_BIND_OP_TOSTRING(VecToStringImpl)

	EQSCRIPT_BIND_FUNC(xy)
	EQSCRIPT_BIND_FUNC(xz)
	EQSCRIPT_BIND_FUNC(xw)
	EQSCRIPT_BIND_FUNC(yz)
	EQSCRIPT_BIND_FUNC(yw)
	EQSCRIPT_BIND_FUNC(zw)

	EQSCRIPT_BIND_FUNC(xyz)
	EQSCRIPT_BIND_FUNC(yzw)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)
	EQSCRIPT_BIND_VAR(z)
	EQSCRIPT_BIND_VAR(w)

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lerp", lerp, Vector4D, (const Vector4D&, const Vector4D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("cerp", cerp, Vector4D, (const Vector4D&, const Vector4D&, const Vector4D&, const Vector4D&, float))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("sign", sign, Vector4D, (const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("clamp", clamp, Vector4D, (const Vector4D&, const Vector4D&, const Vector4D&))

	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distance", distance, float, (const Vector4D&, const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("distanceSqr", distanceSqr, float, (const Vector4D&, const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("length", length, float, (const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("lengthSqr", lengthSqr, float, (const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("dot", dot, float, (const Vector4D&, const Vector4D&))
	EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD("normalize", normalize, Vector4D, (const Vector4D&))
EQSCRIPT_TYPE_END

//
// MColor
//
EQSCRIPT_TYPE_BEGIN( MColor )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(uint)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector4D&)

	EQSCRIPT_BIND_VAR(r)
	EQSCRIPT_BIND_VAR(g)
	EQSCRIPT_BIND_VAR(b)
	EQSCRIPT_BIND_VAR(a)
	EQSCRIPT_BIND_VAR(v)
EQSCRIPT_TYPE_END

//
// Quaternion
//
ESL_ENUM(EQuatRotationSequence);

EQSCRIPT_TYPE_BEGIN(Quaternion)
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector4D&)
	EQSCRIPT_BIND_CONSTRUCTOR(float, const Vector3D&)
	
	EQSCRIPT_BIND_OP(unm)
	EQSCRIPT_BIND_OP(add)
	EQSCRIPT_BIND_OP(sub)
	EQSCRIPT_BIND_OP(mul)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)
	EQSCRIPT_BIND_VAR(z)
	EQSCRIPT_BIND_VAR(w)

	EQSCRIPT_BIND_OP_TOSTRING(VecToStringImpl)

	EQSCRIPT_BIND_FUNC(asVector4D)

	EQSCRIPT_BIND_FUNC(normalize)
	EQSCRIPT_BIND_FUNC(fastNormalize)

	EQSCRIPT_BIND_FUNC(isNan)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(Matrix3x3)
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(
		float, float, float,
		float, float, float,
		float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, const Vector3D&, const Vector3D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Quaternion&)

	// matrix - matrix ops
	EQSCRIPT_BIND_OP(add)
	EQSCRIPT_BIND_OP(sub)
	EQSCRIPT_BIND_OP(mul)

	// negate
	EQSCRIPT_BIND_OP(unm)

	// inverse
	EQSCRIPT_BIND_OP(not)

	// members - row access
	EQSCRIPT_BIND_VAR(r1)
	EQSCRIPT_BIND_VAR(r2)
	EQSCRIPT_BIND_VAR(r3)

	// members - component access
	EQSCRIPT_BIND_VAR(m11)
	EQSCRIPT_BIND_VAR(m12)
	EQSCRIPT_BIND_VAR(m13)
	EQSCRIPT_BIND_VAR(m21)
	EQSCRIPT_BIND_VAR(m22)
	EQSCRIPT_BIND_VAR(m23)
	EQSCRIPT_BIND_VAR(m31)
	EQSCRIPT_BIND_VAR(m32)
	EQSCRIPT_BIND_VAR(m33)

	// operations
	EQSCRIPT_BIND_STATIC_FUNC("transformVec", +[](const Matrix3x3& self, const Vector3D& vec) { return rotateVector(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transformVecInv", +[](const Matrix3x3& self, const Vector3D& vec) { return rotateVector(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transposed", +[](const Matrix3x3& self) { return transpose(self); })
	EQSCRIPT_BIND_STATIC_FUNC("eulersXYZ", EulerMatrixXYZ)
	EQSCRIPT_BIND_STATIC_FUNC("eulersZXY", EulerMatrixZXY)

	// common matrix generators
	EQSCRIPT_BIND_STATIC_FUNC("identity", +[]() {return identity3; })
	EQSCRIPT_BIND_STATIC_FUNC("rotationX", rotateX3<float>)
	EQSCRIPT_BIND_STATIC_FUNC("rotationY", rotateY3<float>)
	EQSCRIPT_BIND_STATIC_FUNC("rotationZ", rotateZ3<float>)
	EQSCRIPT_BIND_STATIC_FUNC("rotationXYZ", +[](const Vector3D& val) {return rotateXYZ3(val.x, val.y, val.z); })
	EQSCRIPT_BIND_STATIC_FUNC("rotationZXY", +[](const Vector3D& val) {return rotateZXY3(val.x, val.y, val.z); })
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(Matrix4x4)
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(
		float, float, float, float,
		float, float, float, float,
		float, float, float, float,
		float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector4D&, const Vector4D&, const Vector4D&, const Vector4D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Quaternion&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Matrix3x3&)

	// matrix - matrix ops
	EQSCRIPT_BIND_OP(add)
	EQSCRIPT_BIND_OP(sub)
	EQSCRIPT_BIND_OP(mul)

	// negate
	EQSCRIPT_BIND_OP(unm)

	// inverse
	EQSCRIPT_BIND_OP(not)

	// members - row access
	EQSCRIPT_BIND_VAR(r1)
	EQSCRIPT_BIND_VAR(r2)
	EQSCRIPT_BIND_VAR(r3)
	EQSCRIPT_BIND_VAR(r4)

	// members - component access
	EQSCRIPT_BIND_VAR(m11)
	EQSCRIPT_BIND_VAR(m12)
	EQSCRIPT_BIND_VAR(m13)
	EQSCRIPT_BIND_VAR(m14)
	EQSCRIPT_BIND_VAR(m21)
	EQSCRIPT_BIND_VAR(m22)
	EQSCRIPT_BIND_VAR(m23)
	EQSCRIPT_BIND_VAR(m24)
	EQSCRIPT_BIND_VAR(m31)
	EQSCRIPT_BIND_VAR(m32)
	EQSCRIPT_BIND_VAR(m33)
	EQSCRIPT_BIND_VAR(m34)
	EQSCRIPT_BIND_VAR(m41)
	EQSCRIPT_BIND_VAR(m42)
	EQSCRIPT_BIND_VAR(m43)
	EQSCRIPT_BIND_VAR(m44)

	// getters
	EQSCRIPT_BIND_FUNC(getTranslationComponent)
	EQSCRIPT_BIND_FUNC(getRotationComponent)
	EQSCRIPT_BIND_FUNC(getTranslationComponentTransposed)
	EQSCRIPT_BIND_FUNC(getRotationComponentTransposed)
	EQSCRIPT_BIND_FUNC(setTranslation)
	EQSCRIPT_BIND_FUNC(setTranslationTransposed)

	// operations
	EQSCRIPT_BIND_STATIC_FUNC("transformVec", +[](const Matrix4x4& self, const Vector3D& vec) { return transformPoint(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transformVecInv", +[](const Matrix4x4& self, const Vector3D& vec) { return transformPoint(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transposed", +[](const Matrix4x4& self) { return transpose(self); })
	EQSCRIPT_BIND_STATIC_FUNC("eulersXYZ", +[](const Matrix4x4& self) { return EulerMatrixXYZ(self.getRotationComponent()); })
	EQSCRIPT_BIND_STATIC_FUNC("eulersZXY", +[](const Matrix4x4& self) { return EulerMatrixZXY(self.getRotationComponent()); })

	// common matrix generators
	EQSCRIPT_BIND_STATIC_FUNC("identity", +[]() {return identity4; })
	EQSCRIPT_BIND_STATIC_FUNC("rotationX", rotateX4<float>)
	EQSCRIPT_BIND_STATIC_FUNC("rotationY", rotateY4<float>)
	EQSCRIPT_BIND_STATIC_FUNC("rotationZ", rotateZ4<float>)
	EQSCRIPT_BIND_STATIC_FUNC("translate", +[](const Vector3D& val) { return translate(val); })
	EQSCRIPT_BIND_STATIC_FUNC("rotationXYZ", +[](const Vector3D& val) { return rotateXYZ4(val.x, val.y, val.z); })
	EQSCRIPT_BIND_STATIC_FUNC("rotationZXY", +[](const Vector3D& val) { return rotateZXY4(val.x, val.y, val.z); })
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(Transform3D)
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, const Quaternion&, const Vector3D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, const Vector3D&, const Vector3D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Matrix4x4&)

	EQSCRIPT_BIND_VAR(t)
	EQSCRIPT_BIND_VAR(r)
	EQSCRIPT_BIND_VAR(s)

	EQSCRIPT_BIND_FUNC(forward)
	EQSCRIPT_BIND_FUNC(right)
	EQSCRIPT_BIND_FUNC(up)

	EQSCRIPT_BIND_FUNC(toMatrix)

	EQSCRIPT_BIND_STATIC_FUNC("inverse", +[](const Transform3D& self) { return inverse(self); })
	EQSCRIPT_BIND_STATIC_FUNC("rotateVector", +[](const Transform3D& self, const Vector3D& vec) { return rotateVector(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transformPoint", +[](const Transform3D& self, const Vector3D& vec) { return transformPoint(vec, self); })
	EQSCRIPT_BIND_STATIC_FUNC("transformPointInverse", +[](const Transform3D& self, const Vector3D& vec) { return transformPointInverse(vec, self); })
EQSCRIPT_TYPE_END

//
// Plane
//
EQSCRIPT_TYPE_BEGIN( Plane )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, float )
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, const Vector3D&, const Vector3D&)
	
	EQSCRIPT_BIND_FUNC( Distance )
	EQSCRIPT_BIND_STATIC_FUNC("GetIntersectionWithRay", +[](const esl::ScriptState& state, const Plane& self, const Vector3D& rayStart, const Vector3D& rayDir) -> esl::Any<3> {
		Vector3D isectPos;
		float dist;
		const bool result = self.GetIntersectionWithRay(rayStart, rayDir, isectPos, dist);
		state.PushValue(result);
		state.PushValue(isectPos);
		state.PushValue(dist);
		return {};
	})
	EQSCRIPT_BIND_STATIC_FUNC("GetIntersectionLine", +[](const esl::ScriptState& state, const Plane& self, const Vector3D& lineBegin, const Vector3D& lineEnd) -> esl::Any<3> {
		Vector3D isectPos;
		float fract;
		const bool result = self.GetIntersectionWithLine(lineBegin, lineEnd, isectPos, fract);
		state.PushValue(result);
		state.PushValue(isectPos);
		state.PushValue(fract);
		return {};
	})
		
	EQSCRIPT_BIND_VAR( normal )
	EQSCRIPT_BIND_VAR( offset )
EQSCRIPT_TYPE_END

//
// BoundingBox
//
EQSCRIPT_TYPE_BEGIN( BoundingBox )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_VAR(minPoint)
	EQSCRIPT_BIND_VAR(maxPoint)

	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&, const Vector3D&)

	EQSCRIPT_BIND_FUNC(Reset)

	EQSCRIPT_BIND_FUNC(AddVertex)

	EQSCRIPT_BIND_FUNC(Fix)
	EQSCRIPT_BIND_FUNC(Merge)
	EQSCRIPT_BIND_FUNC_OVERLOAD(Expand, void, (const Vector3D&))

	EQSCRIPT_BIND_FUNC(IsValid)
	EQSCRIPT_BIND_FUNC(Contains)

	EQSCRIPT_BIND_FUNC(FullyInside)
	EQSCRIPT_BIND_FUNC(Intersects)
	EQSCRIPT_BIND_STATIC_FUNC("IntersectsRay", +[](const esl::ScriptState& state, const BoundingBox& self, const Vector3D& rayStart, const Vector3D& rayDir) -> esl::Any<3> {
		float tnear;
		float tfar;
		const bool result = self.IntersectsRay(rayStart, rayDir, tnear, tfar);
		state.PushValue(result);
		state.PushValue(tnear);
		state.PushValue(tfar);
		return {};
	})
	EQSCRIPT_BIND_FUNC(IntersectsSphere)

	EQSCRIPT_BIND_FUNC(GetCenter)
	EQSCRIPT_BIND_FUNC(GetSize)
	EQSCRIPT_BIND_FUNC(ClampPoint)

	EQSCRIPT_BIND_FUNC(GetVertex)
EQSCRIPT_TYPE_END


//
// Rectangle
//
EQSCRIPT_TYPE_BEGIN( AARectangle )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_VAR(leftTop)
	EQSCRIPT_BIND_VAR(rightBottom)

	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(const AARectangle&)
	EQSCRIPT_BIND_CONSTRUCTOR(const IAARectangle&)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&, const Vector2D&)

	EQSCRIPT_BIND_FUNC(AddVertex)

	EQSCRIPT_BIND_FUNC(IsValid)
	EQSCRIPT_BIND_FUNC(Reset)
	EQSCRIPT_BIND_FUNC(Fix)
	EQSCRIPT_BIND_FUNC_OVERLOAD(Expand, void, (const Vector2D&))

	EQSCRIPT_BIND_FUNC(GetLeftTop)
	EQSCRIPT_BIND_FUNC(GetRightBottom)
	EQSCRIPT_BIND_FUNC(GetLeftBottom)
	EQSCRIPT_BIND_FUNC(GetRightTop)

	EQSCRIPT_BIND_FUNC(GetCenter)
	EQSCRIPT_BIND_FUNC(GetSize)

	EQSCRIPT_BIND_FUNC(GetVertex)

	EQSCRIPT_BIND_FUNC(GetTopVertical)
	EQSCRIPT_BIND_FUNC(GetBottomVertical)
	EQSCRIPT_BIND_FUNC(GetLeftHorizontal)
	EQSCRIPT_BIND_FUNC(GetRightHorizontal)

	EQSCRIPT_BIND_FUNC(ClampPointInRectangle)
	EQSCRIPT_BIND_FUNC(GetRectangleIntersectionDiff)

	EQSCRIPT_BIND_FUNC(Contains)
	EQSCRIPT_BIND_FUNC(FullyInside)
	EQSCRIPT_BIND_FUNC(Intersects)
	EQSCRIPT_BIND_STATIC_FUNC("IntersectsRay", +[](const esl::ScriptState& state, const AARectangle& self, const Vector2D& rayStart, const Vector2D& rayDir) -> esl::Any<3> {
		float tnear;
		float tfar;
		const bool result = self.IntersectsRay(rayStart, rayDir, tnear, tfar);
		state.PushValue(result);
		state.PushValue(tnear);
		state.PushValue(tfar);
		return {};
	})
	EQSCRIPT_BIND_FUNC(IntersectsSphere)

	EQSCRIPT_BIND_FUNC(FlipX)
	EQSCRIPT_BIND_FUNC(FlipY)
EQSCRIPT_TYPE_END

//
// IRectangle
//
EQSCRIPT_TYPE_BEGIN( IAARectangle )
	EQSCRIPT_CLONE_FUNC()
	EQSCRIPT_BIND_VAR(leftTop)
	EQSCRIPT_BIND_VAR(rightBottom)

	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(const IAARectangle&)
	EQSCRIPT_BIND_CONSTRUCTOR(const AARectangle&)
	EQSCRIPT_BIND_CONSTRUCTOR(int, int, int, int)
	EQSCRIPT_BIND_CONSTRUCTOR(const IVector2D&, const IVector2D&)

	EQSCRIPT_BIND_FUNC(AddVertex)

	EQSCRIPT_BIND_FUNC(IsValid)
	EQSCRIPT_BIND_FUNC(Reset)
	EQSCRIPT_BIND_FUNC(Fix)
	EQSCRIPT_BIND_FUNC_OVERLOAD(Expand, void, (const IVector2D&))

	EQSCRIPT_BIND_FUNC(GetLeftTop)
	EQSCRIPT_BIND_FUNC(GetRightBottom)
	EQSCRIPT_BIND_FUNC(GetLeftBottom)
	EQSCRIPT_BIND_FUNC(GetRightTop)

	EQSCRIPT_BIND_FUNC(GetCenter)
	EQSCRIPT_BIND_FUNC(GetSize)

	EQSCRIPT_BIND_FUNC(GetVertex)

	EQSCRIPT_BIND_FUNC(GetTopVertical)
	EQSCRIPT_BIND_FUNC(GetBottomVertical)
	EQSCRIPT_BIND_FUNC(GetLeftHorizontal)
	EQSCRIPT_BIND_FUNC(GetRightHorizontal)

	EQSCRIPT_BIND_FUNC(ClampPointInRectangle)
	EQSCRIPT_BIND_FUNC(GetRectangleIntersectionDiff)

	EQSCRIPT_BIND_FUNC(Contains)
	EQSCRIPT_BIND_FUNC(FullyInside)
	EQSCRIPT_BIND_FUNC(Intersects)

	EQSCRIPT_BIND_FUNC(FlipX)
	EQSCRIPT_BIND_FUNC(FlipY)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(CPseudoRandomGenerator)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_FUNC(Regenerate)

	EQSCRIPT_BIND_FUNC(SetSeed)
	EQSCRIPT_BIND_FUNC(RandomInt)
	EQSCRIPT_BIND_FUNC(RandomFloat)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(CUniformRandomStream)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_FUNC(SetSeed)
	EQSCRIPT_BIND_FUNC(RandomInt)
	EQSCRIPT_BIND_FUNC(RandomFloat)
EQSCRIPT_TYPE_END

//---------------------------------------------------------------------------------------
// Vector math
//---------------------------------------------------------------------------------------

static Quaternion qconjugate(const Quaternion& quat) 
{
	return !quat;
}

static Vector4D qaxisAngle(const Quaternion& q)
{
	Vector3D axis;
	float angle;
	axisAngle(q, axis, angle);
	return Vector4D(axis, angle);
}

static Quaternion quatIdentity()
{
	return qidentity;
}

static esl::Any<2> L_LineIntersectsLine2D(const esl::ScriptState& state, const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE)
{
	Vector2D isectPoint;
	const bool result = LineIntersectsLine2D(lAB, lAE, lBB, lBE, isectPoint);
	state.PushValue(result);
	state.PushValue(isectPoint);
	return {};
}

static esl::Any<2> L_LineSegIntersectsLineSeg2D(const esl::ScriptState& state, const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE)
{
	Vector2D isectPoint;
	const bool result = LineSegIntersectsLineSeg2D(lAB, lAE, lBB, lBE, isectPoint);
	state.PushValue(result);
	state.PushValue(isectPoint);
	return {};
}

static esl::Any<2> L_LineSegIntersectsCircle2D(const esl::ScriptState& state, const Vector2D& lB, const Vector2D& lE, const Vector2D& center, float radius)
{
	FixedArray<Vector2D, 2> isectPoints;
	const bool result = LineSegIntersectsCircle2D(lB, lE, center, radius, isectPoints);
	state.PushValue(result);
	if (result)
	{
		esl::LuaTable isectPointsTbl = state.CreateTable();
		for (int i = 0; i < isectPoints.numElem(); ++i)
			isectPointsTbl.Set(i + 1, isectPoints[i]);

		state.PushValue(isectPointsTbl);
	}
	else
		state.PushValue(nullptr);
	return {};
}

static esl::Any<2> L_AngleVectors(const esl::ScriptState& state, const Vector3D& v)
{
	Vector3D forward, right, up;
	AngleVectors(v, &right, &up);

	state.PushValue(forward);
	state.PushValue(right);
	state.PushValue(up);
	return {};
};

bool eslSysMathInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_CONST(state, QuatRot_zyx);
	LUA_SET_GLOBAL_CONST(state, QuatRot_zxy);
	LUA_SET_GLOBAL_CONST(state, QuatRot_yxz);
	LUA_SET_GLOBAL_CONST(state, QuatRot_yzx);
	LUA_SET_GLOBAL_CONST(state, QuatRot_xyz);
	LUA_SET_GLOBAL_CONST(state, QuatRot_xzy);

	state.RegisterClass<Vector2D>();
	state.RegisterClass<IVector2D>();
	state.RegisterClass<Vector3D>();
	state.RegisterClass<Vector4D>();
	state.RegisterClass<MColor>();
	state.RegisterClass<Matrix3x3>();
	state.RegisterClass<Matrix4x4>();
	state.RegisterClass<Transform3D>();
	state.RegisterClass<Plane>();
	state.RegisterClass<Quaternion>();
	state.RegisterClass<BoundingBox>();
	state.RegisterClass<AARectangle>();
	state.RegisterClass<IAARectangle>();
	state.RegisterClass<CPseudoRandomGenerator>();
	state.RegisterClass<CUniformRandomStream>();

	// FLOAT
	state.SetGlobal("f_fract", EQSCRIPT_CFUNC(fract));
	state.SetGlobal("f_lerp", EQSCRIPT_CFUNC_OVERLOAD(lerp, float, (float, float, float)));
	state.SetGlobal("f_cerp", EQSCRIPT_CFUNC_OVERLOAD(cerp, float, (float, float, float, float, float)));
	state.SetGlobal("f_sign", EQSCRIPT_CFUNC_OVERLOAD(sign, float, (float)));

	// Quaternion
	state.SetGlobal("qinverse", EQSCRIPT_CFUNC_OVERLOAD(inverse, Quaternion, (const Quaternion&)));
	state.SetGlobal("qconjugate", EQSCRIPT_CFUNC(qconjugate));
	state.SetGlobal("qslerp", EQSCRIPT_CFUNC_OVERLOAD(slerp, Quaternion, (const Quaternion&, const Quaternion&, float)));
	state.SetGlobal("qscerp", EQSCRIPT_CFUNC(scerp));
	state.SetGlobal("qlength", EQSCRIPT_CFUNC_OVERLOAD(length, float, (const Quaternion&)));
	state.SetGlobal("qeulersSel", EQSCRIPT_CFUNC(quaternionToEulers));
	state.SetGlobal("qeulers", EQSCRIPT_CFUNC(eulersXYZ));
	state.SetGlobal("qrenormalize", EQSCRIPT_CFUNC(renormalize));
	state.SetGlobal("qaxisAngle", EQSCRIPT_CFUNC(qaxisAngle));
	state.SetGlobal("qcompare", EQSCRIPT_CFUNC_OVERLOAD(quaternionSimilar, bool, (const Quaternion&, const Quaternion&, const float)));
	state.SetGlobal("qrotateVector", EQSCRIPT_CFUNC_OVERLOAD(rotateVector, Vector3D, (const Vector3D&, const Quaternion&)));
	state.SetGlobal("qidentity", EQSCRIPT_CFUNC(quatIdentity));
	state.SetGlobal("qrotateX", EQSCRIPT_CFUNC_OVERLOAD(rotateX, Quaternion, (float)));
	state.SetGlobal("qrotateY", EQSCRIPT_CFUNC_OVERLOAD(rotateY, Quaternion, (float)));
	state.SetGlobal("qrotateZ", EQSCRIPT_CFUNC_OVERLOAD(rotateZ, Quaternion, (float)));
	state.SetGlobal("qrotateXY", EQSCRIPT_CFUNC_OVERLOAD(rotateXY, Quaternion, (float, float)));
	state.SetGlobal("qrotateXYZ", EQSCRIPT_CFUNC_OVERLOAD(rotateXYZ, Quaternion, (float, float, float)));
	state.SetGlobal("qrotateZXY", EQSCRIPT_CFUNC_OVERLOAD(rotateZXY, Quaternion, (float, float, float)));
	state.SetGlobal("qlookAt", EQSCRIPT_CFUNC(lookAt));

	state.SetGlobal("AngleDiff", EQSCRIPT_CFUNC(AngleDiff));
	state.SetGlobal("AnglesDiff", EQSCRIPT_CFUNC(AnglesDiff));

	state.SetGlobal("AngleVectors", EQSCRIPT_CFUNC(L_AngleVectors));
	state.SetGlobal("VectorAngles", EQSCRIPT_CFUNC(VectorAngles));

	state.SetGlobal("LineIntersectsLine2D", EQSCRIPT_CFUNC(L_LineIntersectsLine2D));
	state.SetGlobal("LineSegIntersectsLineSeg2D", EQSCRIPT_CFUNC(L_LineSegIntersectsLineSeg2D));
	state.SetGlobal("LineSegIntersectsCircle2D", EQSCRIPT_CFUNC(L_LineSegIntersectsCircle2D));
	
	return true;
}