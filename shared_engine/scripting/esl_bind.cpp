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

ESL_BUILTIN_ALIAS_TYPE(esl::LuaFunctionRef, "function")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaTableRef, "table")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaTable, "table")
ESL_BUILTIN_ALIAS_TYPE(esl::LuaRawRef, "any")

ESL_BUILTIN_ALIAS_TYPE(char, "string")	// basically ok

namespace esl 
{
void RegisterType(lua_State* L, esl::TypeInfo typeInfo)
{
	{
		lua_getfield(L, LUA_REGISTRYINDEX, typeInfo.className);
		defer{
			lua_pop(L, 1);
		};
		if (!lua_isnil(L, -1))
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
		// upvalues:
		// 1: methods
		// 2: call className
		// 3: thisGetter
		// 4: numClasses
		// 5: ...N classNameHash[numClasses]

		// mt[__index] = function (...)
		lua_pushstring(L, name);

		lua_pushvalue(L, methods);
		lua_pushstring(L, typeInfo.className);
		lua_pushlightuserdata(L, reinterpret_cast<void*>(typeInfo.isByVal ? &runtime::ThisGetterVal : &runtime::ThisGetterPtr));
		
		int numTypeInfos = 0;
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
				const int classNameHash = StringToHash(typeInfoEnumerator.className);
				lua_pushinteger(L, classNameHash);

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

	// push especial eq operator that compares userdata
	if (!typeInfo.isByVal)
	{
		// methods[__eq] = function ()
		lua_pushstring(L, "__eq");
		lua_pushcclosure(L, &esl::runtime::CompareBoxedPointers, 0);
		lua_rawset(L, mt);
	}

	const int classNameHash = StringToHash(typeInfo.className);
	for (const esl::Member& mem : typeInfo.members)
	{
		{
			defer{
				lua_pop(L, 1);
			};
			lua_pushstring(L, mem.name);
			lua_rawget(L, methods);
			if (lua_type(L, -1) != LUA_TNIL)
			{
				ASSERT_FAIL("Class can't have same multiple functions with same name (%s:%s)", typeInfo.className, mem.name);
				continue;
			}
		}

		if (mem.type == esl::MEMB_C_FUNC)
		{
			// methods[name] = function [funcPtr] ()
			lua_pushstring(L, mem.name);
			lua_pushlightuserdata(L, mem.data);
			lua_pushcclosure(L, mem.staticFunc, 1);
			lua_rawset(L, methods);
		}
		else if (mem.type == esl::MEMB_FUNC)
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
		else if (mem.type == esl::MEMB_DTOR)
		{
			lua_pushstring(L, mem.name);
			lua_pushcclosure(L, mem.staticFunc, 0);
			lua_rawset(L, mt);
		}
		else if (mem.type == esl::MEMB_VAR)
		{
			lua_pushstring(L, mem.name);
			lua_pushlightuserdata(L, const_cast<Member*>(&mem));
			lua_rawset(L, methods);
		}
	}

	lua_pushvalue(L, methods);
	lua_setmetatable(L, methods); // set methods as it's own metatable
	lua_pop(L, 2);
}
}

Map<int, EqString>& esl::bindings::BaseClassStorage::GetBaseClassNames()
{
	static Map<int, EqString> baseClassNames{ PP_SL };
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