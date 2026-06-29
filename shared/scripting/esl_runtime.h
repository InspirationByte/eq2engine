#pragma once

#include "esl.h"

namespace esl::runtime
{
struct BaseClassInfo;

// Push pull is essential when you want to send or get values from Lua
template<typename T>
struct PushGet
{
	using PushFunc = void(*)(lua_State* L, const BaseType<T>& obj, int flags);
	using GetFunc = BaseType<T>* (*)(lua_State* L, int index, bool toCpp, bool& isConst, const runtime::BaseClassInfo& upcastBaseInfo);
	using GetThisFunc = ThisGetterFunc;

	static PushFunc Push;
	static GetFunc Get;
	static ThisGetterFunc GetThis;
};

bool	CheckUserdataCanBeUpcasted(lua_State* L, int index, const char* typeName);

}