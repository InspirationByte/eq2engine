#pragma once

//
// keyvalues
//

struct KVPairValue;
struct KVSection;
class LuaKeyValues;

EQSCRIPT_BIND_TYPE_NO_PARENT(KVPairValue, "KVPairValue", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(KVSection, "KVSection", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(LuaKeyValues, "KeyValues", esl::BY_REF)

bool eslSysKeyValuesInit(const esl::ScriptState& state);