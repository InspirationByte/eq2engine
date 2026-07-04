//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CPseudoRandomGenerator;
class CUniformRandomStream;

EQSCRIPT_BIND_TYPE_NO_PARENT(IVector2D, "IVector2D", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector2D, "Vector2D", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector3D, "Vector3D", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Vector4D, "Vector4D", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(MColor, "MColor", esl::BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Quaternion, "Quaternion", esl::BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Matrix4x4, "Matrix4x4", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Matrix3x3, "Matrix3x3", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(Transform3D, "Transform3D", esl::BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(Plane, "Plane", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(AARectangle, "Rectangle", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(IAARectangle, "IRectangle", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(BoundingBox, "BoundingBox", esl::BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(CPseudoRandomGenerator, "PseudoRandomGenerator", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CUniformRandomStream, "UniformRandomGenerator", esl::BY_REF)

bool eslSysMathInit(const esl::ScriptState& state);