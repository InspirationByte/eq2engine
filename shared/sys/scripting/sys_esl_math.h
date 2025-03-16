//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "scripting/esl.h"
#include "scripting/esl_luaref.h"
#include "scripting/esl_bind.h"

class CPseudoRandomGenerator;
class CUniformRandomStream;

EQSCRIPT_BIND_TYPE_NO_PARENT(IVector2D, "IVector2D", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector2D, "Vector2D", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector3D, "Vector3D", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector4D, "Vector4D", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(MColor, "MColor", BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Quaternion, "Quaternion", BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Matrix4x4, "Matrix4x4", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Matrix3x3, "Matrix3x3", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Transform3D, "Transform3D", BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Plane, "Plane", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(AARectangle, "Rectangle", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(IAARectangle, "IRectangle", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(BoundingBox, "BoundingBox", BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(CPseudoRandomGenerator, "PseudoRandomGenerator", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CUniformRandomStream, "UniformRandomGenerator", BY_REF)

bool eslSysMathInit(const esl::ScriptState& state);