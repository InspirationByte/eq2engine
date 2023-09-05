#pragma once

namespace esl::runtime
{
using ThisGetterFunc = void* (*)(lua_State* L);

class StackGuard
{
public:
	StackGuard(lua_State* L);
	~StackGuard();
private:
	lua_State*	m_state;
	int			m_pos{ 0 };
};

void* ThisGetterVal(lua_State* L);
void* ThisGetterPtr(lua_State* L);

bool CheckUserdataCanBeUpcasted(lua_State* L, int index, const char* typeName);
int CallConstructor(lua_State* L);
int CallMemberFunc(lua_State* L);
int CompareBoxedPointers(lua_State* L);

int IndexImpl(lua_State* L);
int NewIndexImpl(lua_State* L);

}