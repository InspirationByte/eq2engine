#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_bind.h"
#include "esl_luaref.h"
#include "esl_event.h"
#include "esl_runtime.h"

constexpr EqStringRef s_luaTString = "string";
constexpr EqStringRef s_luaTBoolean = "boolean";
constexpr EqStringRef s_luaTNumber = "number";
constexpr EqStringRef s_luaTTable = "table";
constexpr EqStringRef s_luaTFunction = "function";
constexpr EqStringRef s_eslTAny = "any";

#define _BUILTIN_ALIAS_TYPE(x, n) \
	template<> const char* LuaTypeAlias<x, false>::value = n;

// default types. Required.
namespace esl
{
_BUILTIN_ALIAS_TYPE(char, s_luaTString)	// basically ok
_BUILTIN_ALIAS_TYPE(EqString, s_luaTString)
_BUILTIN_ALIAS_TYPE(EqStringRef, s_luaTString)
_BUILTIN_ALIAS_TYPE(bool, s_luaTBoolean)
_BUILTIN_ALIAS_TYPE(double, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(int, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(int8, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(uint8, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(int16, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(uint16, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(long, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(uint, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(int64, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(uint64, s_luaTNumber)
_BUILTIN_ALIAS_TYPE(float, s_luaTNumber)

_BUILTIN_ALIAS_TYPE(esl::LuaTable, s_luaTTable)
_BUILTIN_ALIAS_TYPE(esl::LuaTableRef, s_luaTTable)
_BUILTIN_ALIAS_TYPE(esl::LuaFunctionRef, s_luaTFunction)
_BUILTIN_ALIAS_TYPE(esl::LuaRawRef, s_eslTAny)
}

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

esl::TypeInfo esl::GetEmptyTypeInfo()
{
	return {};
}

// TODO: event registrator