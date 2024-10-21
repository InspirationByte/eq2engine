#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_bind.h"
#include "esl_luaref.h"
#include "esl_runtime.h"

namespace esl::runtime 
{
using ThisGetterFunc = void* (*)(lua_State* L, bool& isConstRef);

static void PushErrorIdStr(lua_State* vm)
{
	char const lastErrStr[] = { "EqScriptLib_LastError" };
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

static lua_State* getthread(lua_State* vm, int* arg)
{
	if (lua_isthread(vm, 1))
	{
		*arg = 1;
		return lua_tothread(vm, 1);
	}
	else
	{
		*arg = 0;
		return vm;
	}
}

int StackTrace(lua_State* vm)
{
	static constexpr const int LEVELS1 = 10;
	static constexpr const int LEVELS2 = 20;

	int level;
	int firstpart = 1;  /* still before eventual `...' */
	int arg;
	lua_State *L1 = getthread(vm, &arg);
	lua_Debug ar;
	if (lua_isnumber(vm, arg+2)) {
		level = (int)lua_tointeger(vm, arg+2);//NOLINT
		lua_pop(vm, 1);
	}
	else
		level = (vm == L1) ? 1 : 0;  /* level 0 may be this own function */
	if (lua_gettop(vm) == arg)
		lua_pushliteral(vm, "");
	else if (!lua_isstring(vm, arg+1)) return 1;  /* message is not a string */
	else lua_pushliteral(vm, "\n");
	lua_pushliteral(vm, "stack traceback:");
	while (lua_getstack(L1, level++, &ar))
	{
		if (level > LEVELS1 && firstpart)
		{
			/* no more than `LEVELS2' more levels? */
			if (!lua_getstack(L1, level+LEVELS2, &ar))
				level--;  /* keep going */
			else
			{
				lua_pushliteral(vm, "\n\t...");  /* too many levels */
				while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
					level++;
			}
			firstpart = 0;
			continue;
		}
		lua_pushliteral(vm, "\n\t");
		lua_getinfo(L1, "Snl", &ar);
		lua_pushfstring(vm, "%s:", ar.short_src);
		if(ar.currentline > 0)
			lua_pushfstring(vm, "%d:", ar.currentline);
		if(*ar.namewhat != '\0')  /* is there a name? */
			lua_pushfstring(vm, " in function '%s'", ar.name);
		else
		{
			if(*ar.what == 'm')  /* main? */
				lua_pushfstring(vm, " in main chunk");
			else if (*ar.what == 'C' || *ar.what == 't')
				lua_pushliteral(vm, " ?");  /* C function or tail call */
			else
				lua_pushfstring(vm, " in function <%s:%d>",
								ar.short_src, ar.linedefined);
		}
		lua_concat(vm, lua_gettop(vm) - arg);
	}
	lua_concat(vm, lua_gettop(vm) - arg);
	return 1;
}

StackGuard::StackGuard(StackGuard&& other) noexcept
	: m_state(other.m_state)
	, m_pos(other.m_pos)
{
	other.m_state = nullptr;
}

StackGuard& StackGuard::operator=(StackGuard&& other) noexcept
{
	m_state = other.m_state;
	m_pos = other.m_pos;
	other.m_state = nullptr;
	return *this;
}

StackGuard::StackGuard(lua_State* L, int offset)
	: m_state(L)
{
	if (!L)
		return;
	m_pos = lua_gettop(L) + offset;
	//PPDCheck(L->stack.p);
}
StackGuard::~StackGuard()
{
	if (!m_state)
		return;

	//PPDCheck(m_state->stack.p);
	const int currentTop = lua_gettop(m_state);
	if (currentTop == m_pos)
		return;
	lua_settop(m_state, m_pos);
}

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
				typeInfoEnumerator = typeInfoEnumerator.baseGetter();
				++numTypeInfos;
			}
			lua_pushnumber(L, numTypeInfos);
		}

		{
			esl::TypeInfo typeInfoEnumerator = typeInfo;
			while (typeInfoEnumerator.className)
			{
				const int classNameHash = StringId24(typeInfoEnumerator.className);
				lua_pushinteger(L, classNameHash);

				typeInfoEnumerator = typeInfoEnumerator.baseGetter();
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

	const int classNameHash = StringId24(typeInfo.className);
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
		if (!CString::Compare(typeName, className))
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

		ESL_VERBOSE_LOG("construct %s with %s", className, mem->signature);
		return mem->staticFunc(L);
	}

	luaL_error(L, "Can't construct %s with [%s]", className, GetCallSignatureString(L).ToCString());
	lua_pushnil(L);
	return 1;
}

void* ThisGetterVal(lua_State* L, bool& isConstRef)
{
	isConstRef = false;
	void* userData = lua_touserdata(L, 1);
	return userData;
}

void* ThisGetterPtr(lua_State* L, bool& isConstRef)
{
	esl::BoxUD* userData = static_cast<esl::BoxUD*>(lua_touserdata(L, 1));
	isConstRef = userData ? (userData->flags & UD_FLAG_CONST) : 0;
	return userData ? userData->objPtr : nullptr;
}

// TODO: const member caller
int CallMemberFunc(lua_State* L)
{
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(1)));
	const char* className = lua_tostring(L, lua_upvalueindex(2));
	esl::Member* mem = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstRef = false;
	void* userData = thisGetter(L, isConstRef);
	if (!userData)
	{
		// TODO: ctx.ThrowError
		luaL_error(L, "Error calling %s::%s - self is nil", className, mem->name);
		return -1;
	}

	if (!mem->isConst && isConstRef)
	{
		luaL_error(L, "Error calling %s::%s - cannot call non-const method on const reference", className, mem->name);
		return -1;
	}

	esl::ScriptBind bindObj{ userData };
	return (bindObj.*(mem->func))(L);
}

int CompareBoxedPointers(lua_State* L)
{
	esl::BoxUD* lhs = static_cast<esl::BoxUD*>(lua_touserdata(L, 1));
	esl::BoxUD* rhs = static_cast<esl::BoxUD*>(lua_touserdata(L, 2));

	if (!lhs || !rhs)
	{
		// both maybe nil
		lua_pushboolean(L, lhs == rhs);
		return 1;
	}
	lua_pushboolean(L, lhs->objPtr == rhs->objPtr);
	return 1;
}

static int IndexImplBasic(lua_State* L, EqFunction<int(const Member*)> onVariableIndexed)
{
	// lookup in class metatable first
	{
		lua_pushvalue(L, 2);
		lua_rawget(L, lua_upvalueindex(1)); // methods[key]
		const int type = lua_type(L, -1);
		if (type == LUA_TFUNCTION)
		{
			return 1;
		}
		else if (type == LUA_TLIGHTUSERDATA)
		{
			const Member* memberVar = static_cast<const Member*>(lua_touserdata(L, -1));
			lua_pop(L, 1);
			return onVariableIndexed(memberVar);
		}
		lua_pop(L, 1);
	}

	const char* className = luaL_checkstring(L, lua_upvalueindex(2));

	// lookup in base classes
	const char* classNameLookup = bindings::BaseClassStorage::Get(className);
	while (classNameLookup)
	{
		lua_getglobal(L, classNameLookup); // _G[className]
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			break;
		}

		lua_pushvalue(L, 2);
		lua_rawget(L, -2); // parentMethods[key]

		const int type = lua_type(L, -1);
		if (type == LUA_TFUNCTION)
		{
			lua_replace(L, 3);
			return 1;
		}
		else if (type == LUA_TLIGHTUSERDATA)
		{
			const Member* memberVar = static_cast<const Member*>(lua_touserdata(L, -1));
			lua_pop(L, 2);
			return onVariableIndexed(memberVar);
		}
		lua_pop(L, 2);

		classNameLookup = bindings::BaseClassStorage::Get(classNameLookup);
	}

	return 0;
}

int IndexImpl(lua_State* L)
{
	// upvalues:
	// 1: methods
	// 2: call className
	// 3: thisGetter
	// 4: numClasses
	// 5: ...N classNameHash[numClasses]

	ESL_VERBOSE_LOG("__index %s.%s", lua_tostring(L, lua_upvalueindex(2)), luaL_checkstring(L, 2));

	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstRef = false;
	void* userData = thisGetter(L, isConstRef);

	auto onVariableIndexed = [&](const esl::Member* mem) {
		ASSERT(mem->type == esl::MEMB_VAR);
		if (!userData)
		{
			// TODO: ThrowError
			const char* className = luaL_checkstring(L, lua_upvalueindex(2));
			luaL_error(L, "self is nil while accessing property %s.%s", className, mem->name);
			return 0;
		}

		lua_settop(L, 1);
		esl::ScriptBind bindObj{ userData };
		return (bindObj.*(mem->getFunc))(L);
	};

	const int ret = IndexImplBasic(L, onVariableIndexed);
	if (ret > 0)
		return ret;

	if (userData)
	{
		const char* key = luaL_checkstring(L, 2);
		luaL_error(L, "cannot index variable '%s'", key);
	}

	lua_pushnil(L);
	return 1;
}

int NewIndexImpl(lua_State* L)
{
	// upvalues:
	// 1: methods
	// 2: call className
	// 3: thisGetter
	// 4: numClasses
	// 5: ...N classNameHash[numClasses]

	ESL_VERBOSE_LOG("__newindex %s.%s", lua_tostring(L, lua_upvalueindex(2)), luaL_checkstring(L, 2));

	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstRef = false;
	void* userData = thisGetter(L, isConstRef);

	auto onVariableIndexed = [&](const esl::Member* mem) {
		ASSERT(mem->type == esl::MEMB_VAR);
		if (!userData)
		{
			// TODO: ThrowError
			const char* className = luaL_checkstring(L, lua_upvalueindex(2));
			luaL_error(L, "self is nil while accessing property %s.%s", className, mem->name);
			return 0;
		}

		if (isConstRef)
		{
			const char* className = luaL_checkstring(L, lua_upvalueindex(2));
			luaL_error(L, "trying to set %s.%s on constant reference", className, mem->name);
			return 0;
		}

		// ensure that value is at index 2.
		lua_replace(L, 2);
		esl::ScriptBind bindObj{ userData };
		return (bindObj.*(mem->func))(L);
	};

	const int ret = IndexImplBasic(L, onVariableIndexed);
	if (ret > 0)
		return ret;

	lua_pushnil(L);
	return 1;
}
}