#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_luaref.h"
#include "esl_bind.h"
#include "esl_event.h"
#include "esl_runtime.h"

constexpr EqStringRef s_luaTString = "string";
constexpr EqStringRef s_luaTBoolean = "boolean";
constexpr EqStringRef s_luaTNumber = "number";
constexpr EqStringRef s_luaTTable = "table";
constexpr EqStringRef s_luaTFunction = "function";
constexpr EqStringRef s_eslTAny = "any";
constexpr EqStringRef s_luaTLightUserdata = "lightuserdata";

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
_BUILTIN_ALIAS_TYPE(void, s_luaTLightUserdata)

_BUILTIN_ALIAS_TYPE(LuaTable, s_luaTTable)
_BUILTIN_ALIAS_TYPE(LuaTableRef, s_luaTTable)
_BUILTIN_ALIAS_TYPE(LuaFunctionRef, s_luaTFunction)
_BUILTIN_ALIAS_TYPE(LuaRawRef, s_eslTAny)

namespace bindings
{

Map<uint, runtime::BaseClassInfo>& BaseClassStorage::GetBaseClassNames()
{
	static Map<uint, runtime::BaseClassInfo> baseClassNames{ PP_SL };
	return baseClassNames;
}

runtime::BaseClassInfo BaseClassStorage::Get(const char* className)
{
	const uint nameHash = StringId24(className);
	auto it = GetBaseClassNames().find(nameHash);
	if (it.atEnd())
		return runtime::BaseClassInfo{};

	return *it;
}

runtime::BaseClassInfo BaseClassStorage::GetUpcastingBaseClassInfo(const char* className, const char* targetClassName)
{
	runtime::BaseClassInfo info;
	info.name = className;
	do
	{
		info = bindings::BaseClassStorage::Get(info.name);
		if (!info.name.IsValid())
			return {};

		if (!info.name.Compare(targetClassName))
			return info;

	} while (true);
	return {};
}

}
}
