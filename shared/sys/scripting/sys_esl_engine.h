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

EQSCRIPT_BIND_TYPE_NO_PARENT(IDebugOverlay, "IDebugOverlay", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(EmitParams, "EmitParams", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundEmitterSystem, "CSoundEmitterSystem", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundingObject, "CSoundingObject", BY_REF)

EQSCRIPT_BIND_TYPE_NO_PARENT(DDText3D, "DbgText3D", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDBox, "DbgBox", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDOrientedBox, "DDOrientedBox", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDSphere, "DbgSphere", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDCylinder, "DbgCylinder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDLine, "DbgLine", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DDPoly, "DbgPoly", BY_REF)

bool eslSysHostInit(const esl::ScriptState& state);
bool eslSysStateManagerInit(const esl::ScriptState& state);
bool eslSysSoundEmitterSystemInit(const esl::ScriptState& state);
bool eslSysInputInit(const esl::ScriptState& state);
bool eslSysDebugDrawingInit(const esl::ScriptState& state);
void eslSysInputTerm();
