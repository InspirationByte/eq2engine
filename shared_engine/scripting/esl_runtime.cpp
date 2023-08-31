#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_bind.h"
#include "esl_luaref.h"
#include "esl_runtime.h"

namespace esl::runtime 
{
static void PushErrorIdStr(lua_State* vm)
{
	char const lastErrStr[] = { "esl_last_error" };
	lua_pushlstring(vm, lastErrStr, (sizeof(lastErrStr) / sizeof(char)) - 1);
}

void SetLuaErrorFromTopOfStack(lua_State* L)
{
	const int errIdx = lua_gettop(L);
	PushErrorIdStr(L);
	lua_pushvalue(L, errIdx);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);
}

void ResetErrorValue(lua_State* L)
{
	PushErrorIdStr(L);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
}

const char* GetLastError(lua_State* L)
{
	PushErrorIdStr(L);
	lua_gettable(L, LUA_REGISTRYINDEX);
	static EqString errorStr;
	if (lua_type(L, -1) == LUA_TSTRING)
	{
		size_t len = 0;
		char const* str = lua_tolstring(L, -1, &len);
		errorStr = EqString(str, len);
	}
	lua_pop(L, 1);
	return errorStr;
}

StackGuard::StackGuard(lua_State* L)
{
	m_state = L;
	m_pos = lua_gettop(L);
}
StackGuard::~StackGuard()
{
	const int currentTop = lua_gettop(m_state);
	if (currentTop == m_pos)
		return;
	lua_pop(m_state, currentTop - m_pos);
	m_pos = lua_gettop( m_state );
}

bool CheckUserdataCanBeUpcasted(lua_State* L, int index, const char* typeName)
{
	const int type = luaL_getmetafield(L, index, "__name");
	defer{
		lua_pop(L, 1);
	};
	if (type != LUA_TSTRING)
		return false;

	bool found = false;
	const char* className = lua_tostring(L, -1);
	while (className)
	{
		if (!strcmp(typeName, className))
		{
			found = true;
			break;
		}
		className = bindings::BaseClassStorage::Get(className);
	}
	return found;
}

static EqString GetCallSignatureString(lua_State* L)
{
	const int numArgs = lua_gettop(L);
	EqString argSignature;
	for (int i = 1; i <= numArgs; ++i)
	{
		const int argType = lua_type(L, i);
		if (argType == LUA_TUSERDATA)
		{
			const int type = luaL_getmetafield(L, i, "__name");
			if (type == LUA_TSTRING)
				argSignature.Append(lua_tostring(L, -1));
			else
				argSignature.Append("<invalid_meta_table>");
			lua_pop(L, 1);
		}
		else
			argSignature.Append(lua_typename(L, argType));

		if (i < numArgs)
			argSignature.Append(',');
	}
	return argSignature;
}

static bool CheckCallSignature(lua_State* L, const char* checkSignature)
{
	const int numArgs = lua_gettop(L);

	const char* sigArgName = checkSignature;
	for (int i = 1; i <= numArgs; ++i)
	{
		const char* sigNext = strchr(sigArgName, ',');
		if (!sigNext)
			sigNext = sigArgName + strlen(sigArgName);
		++sigNext;

		const int argType = lua_type(L, i);
		if (argType == LUA_TUSERDATA)
		{
			char* sigArgNameTemp = (char*)stackalloc(sigNext - sigArgName);
			strncpy(sigArgNameTemp, sigArgName, sigNext - sigArgName-1);
			sigArgNameTemp[sigNext - sigArgName - 1] = 0;

			if (!CheckUserdataCanBeUpcasted(L, i, sigArgNameTemp))
				return false;
		}
		else
		{
			const char* typeName = lua_typename(L, argType);
			if (strncmp(sigArgName, typeName, sigNext - sigArgName - 1))
				return false;
		}

		sigArgName = sigNext;
	}
	return true;
}

static bool CheckCallSignatureMatching(lua_State* L, const esl::Member* member)
{
	const int numArgs = lua_gettop(L);
	if (member->numArgs != numArgs)
		return false;

	return CheckCallSignature(L, member->signature);
}

int CallConstructor(lua_State* L)
{
	const int numArgs = lua_gettop(L);

	const char* className = lua_tostring(L, lua_upvalueindex(1));
	const esl::Member* members = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(2)));

	// TODO: map lookup
	for (const esl::Member* mem = members; mem && mem->type != esl::MEMB_NULL; ++mem)
	{
		if (mem->type != esl::MEMB_CTOR)
			continue;

		if (!CheckCallSignatureMatching(L, mem))
			continue;

		return mem->staticFunc(L);
	}

	luaL_error(L, "Can't construct %s with [%s]", className, GetCallSignatureString(L).ToCString());
	lua_pushnil(L);
	return 1;
}

void* ThisGetterVal(lua_State* L)
{
	void* userData = lua_touserdata(L, 1);
	return userData;
}

void* ThisGetterPtr(lua_State* L)
{
	esl::BoxUD* userData = static_cast<esl::BoxUD*>(lua_touserdata(L, 1));
	return userData ? userData->objPtr : nullptr;
}

// TODO: const member caller
int CallMemberFunc(lua_State* L)
{
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(1)));
	const char* className = lua_tostring(L, lua_upvalueindex(2));
	esl::Member* reg = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(3)));

	void* userData = thisGetter(L);
	if (!userData)
	{
		// TODO: ctx.ThrowError
		luaL_error(L, "Error calling %s::%s - self is nil", className, reg->name);
		return -1;
	}

	esl::ScriptBind bindObj{ userData };
	return (bindObj.*(reg->func))(L);
}

int IndexImpl(lua_State* L)
{
	// lookup in table first
	{
		lua_pushvalue(L, 2);
		lua_rawget(L, lua_upvalueindex(2)); // methods[key]
		const int top = lua_gettop(L);
		if (!lua_isnil(L, top))
		{
			lua_pushvalue(L, top);
			return 1;
		}
		lua_pop(L, 1);
	}

	// lookup a property for setter

	// self, key
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(1)));
	void* userData = thisGetter(L);
	const char* key = luaL_checkstring(L, 2);

	// Access the members array from upvalues
	const int numClasses = luaL_checknumber(L, lua_upvalueindex(4));

	// TODO: property map lookup
	for (int i = 0; i < numClasses; ++i)
	{
		const esl::Member* members = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(5 + i)));

		for (const esl::Member* mem = members; mem && mem->type != esl::MEMB_NULL; ++mem)
		{
			if (mem->type != esl::MEMB_VAR)
				continue;

			if (strcmp(key, mem->name))
				continue;

			if (!userData)
			{
				// TODO: ThrowError
				const char* className = luaL_checkstring(L, lua_upvalueindex(3));
				luaL_error(L, "self is nil while accessing property %s.%s", className, mem->name);
				return 0;
			}

			lua_settop(L, 1);
			esl::ScriptBind bindObj{ userData };
			return (bindObj.*(mem->getFunc))(L);
		}
	}
	if(userData)
		luaL_error(L, "cannot index variable '%s'", key);

	lua_pushnil(L);
	return 1;
}

int NewIndexImpl(lua_State* L)
{
	// lookup in table first
	{
		lua_pushvalue(L, 2);
		lua_rawget(L, lua_upvalueindex(2)); // methods[key]
		const int top = lua_gettop(L);
		if (!lua_isnil(L, top))
		{
			lua_pushvalue(L, top);
			return 1;
		}
		lua_pop(L, 1);
	}

	// lookup a property for setter

	// self, key, value
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(1)));
	void* userData = thisGetter(L);
	const char* key = luaL_checkstring(L, 2);

	// Access the members array from upvalues
	const int numClasses = luaL_checknumber(L, lua_upvalueindex(4));

	// TODO: map lookup
	for (int i = 0; i < numClasses; ++i)
	{
		const esl::Member* members = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(5 + i)));

		for (const esl::Member* mem = members; mem && mem->type != esl::MEMB_NULL; ++mem)
		{
			if (mem->type != esl::MEMB_VAR)
				continue;

			if (strcmp(key, mem->name))
				continue;

			if (!userData)
			{
				// TODO: ThrowError
				const char* className = luaL_checkstring(L, lua_upvalueindex(3));
				luaL_error(L, "self is nil while accessing property %s.%s", className, mem->name);
				return 0;
			}

			// ensure that value is at index 1.
			lua_replace(L, 1);
			//lua_settop(L, 1);

			esl::ScriptBind bindObj{ userData };
			return (bindObj.*(mem->func))(L);
		}
	}
	lua_pushnil(L);
	return 1;
}
}