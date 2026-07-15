#pragma once

struct Spline3dPoint;
class CSpline3d;

EQSCRIPT_BIND_TYPE_NO_PARENT(Spline3dPoint, "Spline3dPoint", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSpline3d, "Spline3d", esl::BY_REF)

bool eslSysMathUtilsInit(const esl::ScriptState& state);