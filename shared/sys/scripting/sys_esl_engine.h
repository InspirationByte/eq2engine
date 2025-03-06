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

struct DbgText3DBuilder;
struct DbgBoxBuilder;
struct DbgOriBoxBuilder;
struct DbgSphereBuilder;
struct DbgCylinderBuilder;
struct DbgLineBuilder;
struct DbgPolyBuilder;

EQSCRIPT_BIND_TYPE_NO_PARENT(IDebugOverlay, "IDebugOverlay", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(EmitParams, "EmitParams", BY_VALUE)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundEmitterSystem, "CSoundEmitterSystem", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CSoundingObject, "CSoundingObject", BY_REF)

EQSCRIPT_BIND_TYPE_NO_PARENT(DbgText3DBuilder, "DbgText3DBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgBoxBuilder, "DbgBoxBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgOriBoxBuilder, "DbgOriBoxBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgSphereBuilder, "DbgSphereBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgCylinderBuilder, "DbgCylinderBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgLineBuilder, "DbgLineBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(DbgPolyBuilder, "DbgPolyBuilder", BY_REF)

bool eslSysHostInit(const esl::ScriptState& state);
bool eslSysStateManagerInit(const esl::ScriptState& state);
bool eslSysSoundEmitterSystemInit(const esl::ScriptState& state);
bool eslSysInputInit(const esl::ScriptState& state);
bool eslSysDebugDrawingInit(const esl::ScriptState& state);
void eslSysInputTerm();
