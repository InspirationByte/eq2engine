#pragma once

class IMaterialSystem;
class IMaterial;

EQSCRIPT_BIND_TYPE_NO_PARENT(IMaterialSystem, "IMaterialSystem", esl::BY_REF | esl::ABSTRACT)
EQSCRIPT_BIND_TYPE_NO_PARENT(IMaterial, "IMaterial", esl::REF_PTR)

bool eslSysMaterialSystemInit(const esl::ScriptState& state);