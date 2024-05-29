#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_bind.h"
#include "esl_luaref.h"
#include "esl_event.h"
#include "esl_runtime.h"

#define ESL_BUILTIN_ALIAS_TYPE(x, n) \
	template<> const char* esl::LuaTypeAlias<x, false>::value = n;

// default types. Required.
ESL_BUILTIN_ALIAS_TYPE(bool, "boolean")
ESL_BUILTIN_ALIAS_TYPE(int8, "number")
ESL_BUILTIN_ALIAS_TYPE(uint8, "number")
ESL_BUILTIN_ALIAS_TYPE(int16, "number")
ESL_BUILTIN_ALIAS_TYPE(uint16, "number")
ESL_BUILTIN_ALIAS_TYPE(int, "number")
ESL_BUILTIN_ALIAS_TYPE(long, "number")
ESL_BUILTIN_ALIAS_TYPE(uint, "number")
ESL_BUILTIN_ALIAS_TYPE(int64, "number")
ESL_BUILTIN_ALIAS_TYPE(uint64, "number")
ESL_BUILTIN_ALIAS_TYPE(float, "number")
ESL_BUILTIN_ALIAS_TYPE(double, "number")
ESL_BUILTIN_ALIAS_TYPE(EqString, "string")
ESL_BUILTIN_ALIAS_TYPE(EqStringRef, "string")

ESL_BUILTIN_ALIAS_TYPE(esl::LuaFunctionRef, "function")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaTableRef, "table")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaTable, "table")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaRawRef, "any")

ESL_BUILTIN_ALIAS_TYPE(char, "string")	// basically ok

Map<int, EqStringRef>& esl::bindings::BaseClassStorage::GetBaseClassNames()
{
	static Map<int, EqStringRef> baseClassNames{ PP_SL };
	return baseClassNames;
}

const char* esl::bindings::BaseClassStorage::Get(const char* className)
{
	const int nameHash = StringToHash(className);
	auto it = GetBaseClassNames().find(nameHash);
	if (it.atEnd())
		return nullptr;

	return *it;
}

// TODO: event registrator