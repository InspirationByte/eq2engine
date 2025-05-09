#pragma once

//
// keyvalues
//

struct KVPairValue;
struct KVSection;
class KeyValues;

EQSCRIPT_BIND_TYPE_NO_PARENT(KVPairValue, "KVPairValue", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(KVSection, "KVSection", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(KeyValues, "KeyValues", BY_REF)

bool eslSysKeyValuesInit(const esl::ScriptState& state);