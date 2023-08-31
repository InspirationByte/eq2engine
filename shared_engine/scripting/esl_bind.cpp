#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_bind.h"
#include "esl_luaref.h"
#include "esl_runtime.h"

// default types. Required.
EQSCRIPT_ALIAS_TYPE(bool, "boolean")
EQSCRIPT_ALIAS_TYPE(int8, "number")
EQSCRIPT_ALIAS_TYPE(uint8, "number")
EQSCRIPT_ALIAS_TYPE(int16, "number")
EQSCRIPT_ALIAS_TYPE(uint16, "number")
EQSCRIPT_ALIAS_TYPE(int, "number")
EQSCRIPT_ALIAS_TYPE(uint, "number")
EQSCRIPT_ALIAS_TYPE(int64, "number")
EQSCRIPT_ALIAS_TYPE(uint64, "number")
EQSCRIPT_ALIAS_TYPE(float, "number")
EQSCRIPT_ALIAS_TYPE(double, "number")
EQSCRIPT_ALIAS_TYPE(EqString, "string")

EQSCRIPT_ALIAS_TYPE(esl::LuaFunctionRef, "function")
EQSCRIPT_ALIAS_TYPE(esl::LuaTableRef, "table")
EQSCRIPT_ALIAS_TYPE(esl::LuaTable, "table")
EQSCRIPT_ALIAS_TYPE(esl::LuaRawRef, "any")

EQSCRIPT_ALIAS_TYPE(char, "string")	// basically ok

namespace esl 
{
void RegisterType(lua_State* L, esl::TypeInfo typeInfo)
{
	{
		lua_getfield(L, LUA_REGISTRYINDEX, typeInfo.className);
		defer{
			lua_pop(L, 1);
		};
		if (lua_isnil(L, -1) == 0)
		{
			ASSERT_FAIL("Type %s already registered", typeInfo.className);
			return;
		}
	}

	lua_newtable(L);
	const int methods = lua_gettop(L); // methods

	luaL_newmetatable(L, typeInfo.className);
	const int mt = lua_gettop(L); // mt

	// store method table in globals so that scripts can add functions written in Lua.
	lua_pushvalue(L, methods);
	lua_setglobal(L, typeInfo.className); // _G[className] = methods

	// __index for property features
	auto setIndexFunction = [&](const char* name, lua_CFunction func) {
		// mt[__index] = function [thisGetter, methods, className, numTypeInfos, typeInfoMembers...] (...)
		lua_pushstring(L, name);
		int numTypeInfos = 0;

		void* funcPtr = reinterpret_cast<void*>(typeInfo.isByVal ? &runtime::ThisGetterVal : &runtime::ThisGetterPtr);
		lua_pushlightuserdata(L, funcPtr);
		lua_pushvalue(L, methods);
		lua_pushstring(L, typeInfo.className);
		{
			esl::TypeInfo typeInfoEnumerator = typeInfo;
			while (typeInfoEnumerator.className)
			{
				typeInfoEnumerator = *typeInfoEnumerator.base;
				++numTypeInfos;
			}
			lua_pushnumber(L, numTypeInfos);
		}

		{
			esl::TypeInfo typeInfoEnumerator = typeInfo;
			while (typeInfoEnumerator.className)
			{
				lua_pushlightuserdata(L, const_cast<esl::Member*>(typeInfoEnumerator.members.ptr()));
				typeInfoEnumerator = *typeInfoEnumerator.base;
			}
		}

		// mt[name] = func
		lua_pushcclosure(L, func, numTypeInfos + 4);
		lua_rawset(L, mt);
	};

	setIndexFunction("__index", runtime::IndexImpl);
	setIndexFunction("__newindex", runtime::NewIndexImpl);
	
	// constructors function
	// if null - abstract
	bool hasConstructors = false;
	for (const esl::Member& mem : typeInfo.members)
	{
		if(mem.type == MEMB_CTOR)
		{
			hasConstructors = true;
			break;
		}
	}
	if(hasConstructors)
	{
		// methods["new"] = function [className, typeInfoMembers] (...)

		lua_pushstring(L, "new");
		lua_pushstring(L, typeInfo.className);
		lua_pushlightuserdata(L, const_cast<esl::Member*>(typeInfo.members.ptr()));

		lua_pushcclosure(L, &esl::runtime::CallConstructor, 2);
		lua_rawset(L, methods);
	}

	auto registerMembers = [&](const esl::TypeInfo& typeInfo, bool onlyMembers) {
		for (const esl::Member& mem : typeInfo.members)
		{
			// follow function hiding rule of C++
			{
				// if methods[name] != nil
				defer{
					lua_pop(L, 1);
				};
				lua_pushstring(L, mem.name);
				lua_rawget(L, methods);
				const int top = lua_gettop(L);
				if (lua_type(L, top) != LUA_TNIL)
					continue;
			}

			if (mem.type == esl::MEMB_FUNC)
			{
				// methods[name] = function [thisGetter, className, typeInfoMember] ()
				lua_pushstring(L, mem.name);
				void* funcPtr = reinterpret_cast<void*>(typeInfo.isByVal ? &runtime::ThisGetterVal : &runtime::ThisGetterPtr);
				lua_pushlightuserdata(L, funcPtr);
				lua_pushstring(L, typeInfo.className);
				lua_pushlightuserdata(L, const_cast<esl::Member*>(&mem));
				lua_pushcclosure(L, &esl::runtime::CallMemberFunc, 3);
				lua_rawset(L, methods);
			}
			else if (mem.type == esl::MEMB_OPERATOR)
			{
				// methods[name] = function ()
				lua_pushstring(L, mem.name);
				lua_pushcclosure(L, mem.staticFunc, 0);
				lua_rawset(L, mt);
			}

			if (onlyMembers)
				continue;

			if (mem.type == esl::MEMB_DTOR)
			{
				lua_pushstring(L, mem.name);
				lua_pushcclosure(L, mem.staticFunc, 0);
				lua_rawset(L, mt);
			}
		}
	};
	registerMembers(typeInfo, false);

	// register base class
	{
		esl::TypeInfo typeInfoEnumerator = typeInfo;
		while (typeInfoEnumerator.className)
		{
			// also ignore constructors and destructors of parent types
			registerMembers(typeInfoEnumerator, true);
			typeInfoEnumerator = *typeInfoEnumerator.base;
		}
	}

	lua_pushvalue(L, methods);
	lua_setmetatable(L, methods); // set methods as it's own metatable
	lua_pop(L, 2);
}
}

template<typename T>
void EqScriptClass<T>::Register(lua_State* L)
{
	RegisterType(L, GetTypeInfo());
}

Map<int, EqString> esl::bindings::BaseClassStorage::baseClassNames = { PP_SL };
const char* esl::bindings::BaseClassStorage::Get(const char* className)
{
	const int nameHash = StringToHash(className);
	auto it = baseClassNames.find(nameHash);
	if (it.atEnd())
		return nullptr;

	return *it;
}