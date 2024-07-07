#pragma once

namespace esl::runtime
{
// Push pull is essential when you want to send or get values from Lua
template<typename T>
struct PushGet
{
	using PushFunc = void(*)(lua_State* L, const BaseType<T>& obj, int flags);
	using GetFunc = BaseType<T>* (*)(lua_State* L, int index, bool toCpp);

	static PushFunc Push;
	static GetFunc Get;
};

void* ThisGetterVal(lua_State* L, bool& isConstRef);
void* ThisGetterPtr(lua_State* L, bool& isConstRef);

bool CheckUserdataCanBeUpcasted(lua_State* L, int index, const char* typeName);
int CallConstructor(lua_State* L);
int CallMemberFunc(lua_State* L);
int CompareBoxedPointers(lua_State* L);

int IndexImpl(lua_State* L);
int NewIndexImpl(lua_State* L);

}