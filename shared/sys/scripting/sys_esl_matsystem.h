#pragma once

class IMaterialSystem;
class IMaterial;

EQSCRIPT_BIND_TYPE_NO_PARENT(IMaterialSystem, "IMaterialSystem", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(IMaterial, "IMaterial", REF_PTR)

bool eslSysMaterialSystemInit(const esl::ScriptState& state);