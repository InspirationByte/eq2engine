//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

// TODO
#pragma once

class IDebugOverlay;
class CSoundEmitterSystem;
class CSoundingObject;
struct EmitParams;

struct DDText3D;
struct DDBox;
struct DDOrientedBox;
struct DDSphere;
struct DDCylinder;
struct DDLine;
struct DDPoly;

EQSCRIPT_BIND_TYPE_NO_PARENT(IDebugOverlay, "IDebugOverlay", esl::BY_REF | esl::ABSTRACT)
EQSCRIPT_BIND_TYPE_NO_PARENT(EmitParams, "EmitParams", esl::BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundEmitterSystem, "CSoundEmitterSystem", esl::BY_REF | esl::ABSTRACT)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundingObject, "CSoundingObject", esl::BY_REF)

EQSCRIPT_BIND_TYPE_NO_PARENT(DDText3D, "DbgText3D", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDBox, "DbgBox", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDOrientedBox, "DDOrientedBox", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDSphere, "DbgSphere", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDCylinder, "DbgCylinder", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDLine, "DbgLine", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDPoly, "DbgPoly", esl::BY_REF)

bool eslSysHostInit(const esl::ScriptState& state);
bool eslSysStateManagerInit(const esl::ScriptState& state);
bool eslSysSoundEmitterSystemInit(const esl::ScriptState& state);
bool eslSysInputInit(const esl::ScriptState& state);
bool eslSysDebugDrawingInit(const esl::ScriptState& state);
void eslSysInputTerm();
