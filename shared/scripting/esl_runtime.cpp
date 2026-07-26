#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_luaref.h"
#include "esl_bind.h"
#include "esl_runtime.h"

namespace esl::runtime 
{
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
		errorStr = EqString(str, static_cast<int>(len));
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

static int StackTracePrintHandler(lua_State* vm)
{
	static constexpr const int LEVELS1 = 10;
	static constexpr const int LEVELS2 = 20;

	int level;
	int firstpart = 1;  /* still before eventual `...' */
	int arg;
	lua_State* L1 = getthread(vm, &arg);
	lua_Debug ar;
	if (lua_isnumber(vm, arg + 2)) {
		level = (int)lua_tointeger(vm, arg + 2);//NOLINT
		lua_pop(vm, 1);
	}
	else
		level = (vm == L1) ? 1 : 0;  /* level 0 may be this own function */
	if (lua_gettop(vm) == arg)
		lua_pushliteral(vm, "");
	else if (!lua_isstring(vm, arg + 1)) return 1;  /* message is not a string */
	else lua_pushliteral(vm, "\n");
	lua_pushliteral(vm, "stack traceback:");
	while (lua_getstack(L1, level++, &ar))
	{
		if (level > LEVELS1 && firstpart)
		{
			/* no more than `LEVELS2' more levels? */
			if (!lua_getstack(L1, level + LEVELS2, &ar))
				level--;  /* keep going */
			else
			{
				lua_pushliteral(vm, "\n\t...");  /* too many levels */
				while (lua_getstack(L1, level + LEVELS2, &ar))  /* find last levels */
					level++;
			}
			firstpart = 0;
			continue;
		}
		lua_pushliteral(vm, "\n\t");
		lua_getinfo(L1, "Snl", &ar);
		lua_pushfstring(vm, "%s:", ar.short_src);
		if (ar.currentline > 0)
			lua_pushfstring(vm, "%d:", ar.currentline);
		if (*ar.namewhat != '\0')  /* is there a name? */
			lua_pushfstring(vm, " in function '%s'", ar.name);
		else
		{
			if (*ar.what == 'm')  /* main? */
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

static lua_CFunction RuntimeErrorHandlerFunc = StackTracePrintHandler;

lua_CFunction SetErrorHandler(lua_CFunction handler)
{
	lua_CFunction oldFunc = RuntimeErrorHandlerFunc;
	RuntimeErrorHandlerFunc = handler;
	return oldFunc;
}

int HandleRuntimeError(lua_State* L)
{
	return RuntimeErrorHandlerFunc(L);
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

bool CheckUserdataCanBeUpcasted(lua_State* L, int index, const char* targetClassName)
{
	const int type = luaL_getmetafield(L, index, "__name");
	defer{
		lua_pop(L, 1);
	};
	if (type != LUA_TSTRING)
		return false;

	const char* className = lua_tostring(L, -1);

	// check if no upcasting required
	if (!CString::Compare(targetClassName, className))
		return true;

	bindings::BaseClassStorage::Info baseInfo = bindings::BaseClassStorage::GetUpcastingBaseClassInfo(className, targetClassName);
	return baseInfo.IsValid();
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

static int UserTypeCallConstructor(lua_State* L)
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

static int UserTypeCallMemberFunc(lua_State* L)
{
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(1)));
	const char* className = lua_tostring(L, lua_upvalueindex(2));
	esl::Member* mem = static_cast<esl::Member*>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstThis = false;
	void* thisPtr = thisGetter(L, isConstThis);
	if (!thisPtr)
	{
		luaL_error(L, "Error calling %s::%s - self is nil", className, mem->name);
		return -1;
	}

	if (!mem->isConst && isConstThis)
	{
		luaL_error(L, "Error calling %s::%s - cannot call non-const method on const reference", className, mem->name);
		return -1;
	}

	const char* thisClassName = nullptr;
	{
		const int type = luaL_getmetafield(L, 1, "__name");
		thisClassName = lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	// apply offset for base class
	runtime::BaseClassInfo baseInfo = bindings::BaseClassStorage::GetUpcastingBaseClassInfo(thisClassName, className);
	if (baseInfo.IsValid())
		thisPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(thisPtr) + baseInfo.offset);

	esl::ScriptBind bindObj{ thisPtr };
	return (bindObj.*(mem->func))(L);
}

static int UserTypeCompareBoxedPointers(lua_State* L)
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

static int UserTypeIndexImplBasic(lua_State* L, void* thisPtr, const EqFunction<int(void* thisPtr, const Member*)>& onUserDataIndex)
{
	// lookup in class metatable first
	{
		lua_pushvalue(L, 2);
		lua_rawget(L, lua_upvalueindex(1)); // fields[key]
		const int type = lua_type(L, -1);
		if (type == LUA_TFUNCTION)
		{
			return 1;
		}
		else if (type == LUA_TLIGHTUSERDATA)
		{
			const Member* memberVar = static_cast<const Member*>(lua_touserdata(L, -1));
			lua_pop(L, 1);
			return onUserDataIndex(thisPtr, memberVar);
		}
		lua_pop(L, 1);
	}

	const char* className = luaL_checkstring(L, lua_upvalueindex(2));

	// lookup in base classes
	runtime::BaseClassInfo baseInfo = bindings::BaseClassStorage::Get(className);
	while (baseInfo.IsValid())
	{
		lua_getglobal(L, baseInfo.name); // _G[className]
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			break;
		}

		lua_pushvalue(L, 2);
		lua_rawget(L, -2); // parentFields[key]

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

			// apply offset for base class
			thisPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(thisPtr) + baseInfo.offset);

			return onUserDataIndex(thisPtr, memberVar);
		}
		lua_pop(L, 2);

		baseInfo = bindings::BaseClassStorage::Get(baseInfo.name);
	}

	return 0;
}

static int UserTypeIndexImpl(lua_State* L)
{
	// upvalues:
	// 1: fields
	// 2: call className
	// 3: thisGetter

	ESL_VERBOSE_LOG("__index %s.%s", lua_tostring(L, lua_upvalueindex(2)), luaL_checkstring(L, 2));
	const char* className = luaL_checkstring(L, lua_upvalueindex(2));
	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstRef = false;
	void* thisPtr = thisGetter(L, isConstRef);

	auto onUserDataIndex = [&](void* thisPtr, const esl::Member* mem) {
		ASSERT(mem->type == esl::MEMB_VAR);
		if (!thisPtr)
		{
			luaL_error(L, "self is nil while accessing %s.%s", className, mem->name);
			return 0;
		}

		lua_settop(L, 1);
		esl::ScriptBind bindObj{ thisPtr };
		return (bindObj.*(mem->getFunc))(L);
	};

	const int ret = UserTypeIndexImplBasic(L, thisPtr, onUserDataIndex);
	if (ret > 0)
		return ret;

	if (thisPtr)
	{
		const char* key = luaL_checkstring(L, 2);
		luaL_error(L, "cannot index '%s'", key);
	}

	lua_pushnil(L);
	return 1;
}

static int UserTypeNewIndexImpl(lua_State* L)
{
	// upvalues:
	// 1: fields
	// 2: call className
	// 3: thisGetter

	ESL_VERBOSE_LOG("__newindex %s.%s", lua_tostring(L, lua_upvalueindex(2)), luaL_checkstring(L, 2));
	const char* className = luaL_checkstring(L, lua_upvalueindex(2));

	ThisGetterFunc thisGetter = reinterpret_cast<ThisGetterFunc>(lua_touserdata(L, lua_upvalueindex(3)));

	bool isConstRef = false;
	void* thisPtr = thisGetter(L, isConstRef);

	auto onUserDataIndex = [&](void* thisPtr, const esl::Member* mem) {
		ASSERT(mem->type == esl::MEMB_VAR);
		if (!thisPtr)
		{
			luaL_error(L, "self is nil while accessing property %s.%s", className, mem->name);
			return 0;
		}

		if (isConstRef)
		{
			luaL_error(L, "trying to set %s.%s on constant reference", className, mem->name);
			return 0;
		}

		// ensure that value is at index 2.
		lua_replace(L, 2);
		esl::ScriptBind bindObj{ thisPtr };
		return (bindObj.*(mem->func))(L);
	};

	const int ret = UserTypeIndexImplBasic(L, thisPtr, onUserDataIndex);
	if (ret > 0)
		return ret;

	lua_pushnil(L);
	return 1;
}

// iterator for type info
struct UserTypeIterator
{
	runtime::BaseClassInfo baseInfo;
	bool iterateOverBases{ false };
	bool restart{ false };
};

static int UserTypeNext(lua_State* L)
{
	UserTypeIterator* iterator = reinterpret_cast<UserTypeIterator*>(lua_touserdata(L, lua_upvalueindex(1)));

	// iterate class metatable first
	if (!iterator->iterateOverBases)
	{
		// iterate over fields
		if (lua_next(L, lua_upvalueindex(2)))
		{
			lua_pop(L, 1);
			lua_pushvalue(L, 1);
			lua_pushvalue(L, -2);
			lua_gettable(L, -2);
			lua_replace(L, -2);
			return 2;
		}

		iterator->iterateOverBases = true;
		iterator->restart = true;

		// init base class info
		const char* className = luaL_checkstring(L, lua_upvalueindex(3));
		iterator->baseInfo = bindings::BaseClassStorage::Get(className);
	}

	// lookup in base classes
	while (iterator->baseInfo.IsValid())
	{
		if (iterator->restart)
		{
			lua_pushnil(L);
			iterator->restart = false;
		}

		lua_getglobal(L, iterator->baseInfo.name); // _G[className]
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			break;
		}

		lua_replace(L, -1);

		// iterate over fields
		if (lua_next(L, 0))
		{
			lua_pop(L, 1);
			lua_pushvalue(L, 1);
			lua_pushvalue(L, -2);
			lua_gettable(L, -2);
			lua_replace(L, -2);

			return 2;
		}
		lua_pop(L, 1);

		iterator->baseInfo = bindings::BaseClassStorage::Get(iterator->baseInfo.name);
		iterator->restart = true;
	}

	return 0; // End of iteration
}

static int UserTypePairsImpl(lua_State* L)
{
	// upvalues:
	// 1: fields
	// 2: className

	// Create and initialize the iterator userdata
	UserTypeIterator* iterator = reinterpret_cast<UserTypeIterator*>(lua_newuserdata(L, sizeof(UserTypeIterator)));
	*iterator = {};

	// Set up iterator function with upvalues
	lua_pushvalue(L, lua_upvalueindex(1)); // fields table
	lua_pushvalue(L, lua_upvalueindex(2)); // className

	lua_pushcclosure(L, UserTypeNext, 2 + 1); // +1 for iterator userdata
	lua_pushvalue(L, 1); // Push the userdata to be iterated
	lua_pushnil(L); // Initial key for iteration

	return 3; // Return iterator function, userdata, and initial key
}

//----------------------------------------------------------------

static void SetIndexFunction(lua_State* L, const esl::TypeInfo& typeInfo, int fields, int mt, const char* name, lua_CFunction func)
{
	// mt[__index] = function (...)
	lua_pushstring(L, name);

	// upvalues:
	// 1: fields
	// 2: className
	// 3: thisGetter (optional)
	lua_pushvalue(L, fields);
	lua_pushstring(L, typeInfo.className);
	if (typeInfo.thisGetter)
		lua_pushlightuserdata(L, reinterpret_cast<void*>(typeInfo.thisGetter));

	// mt[name] = func
	lua_pushcclosure(L, func, typeInfo.thisGetter ? 3 : 2);
	lua_rawset(L, mt);
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
	const int fields = lua_gettop(L); // fields

	// store method table in globals so that scripts can add functions written in Lua.
	lua_pushvalue(L, fields);
	lua_setglobal(L, typeInfo.className); // _G[className] = fields

	luaL_newmetatable(L, typeInfo.className);
	const int mt = lua_gettop(L); // mt

	// __index for property features
	SetIndexFunction(L, typeInfo, fields, mt, "__index", UserTypeIndexImpl);
	SetIndexFunction(L, typeInfo, fields, mt, "__newindex", UserTypeNewIndexImpl);

	// __pairs to view variables and function names
	{
		lua_pushstring(L, "__pairs");

		// upvalues:
		// 1: fields
		// 2: className
		lua_pushvalue(L, fields);
		lua_pushstring(L, typeInfo.className);
	
		// mt[__pairs] = function (...)
		lua_pushcclosure(L, UserTypePairsImpl, 2);
		lua_rawset(L, mt);
	}
	
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
		// fields["new"] = function [className, typeInfoMembers] (...)
		lua_pushstring(L, "new");
		lua_pushstring(L, typeInfo.className);
		lua_pushlightuserdata(L, const_cast<esl::Member*>(typeInfo.members.ptr()));

		lua_pushcclosure(L, &esl::runtime::UserTypeCallConstructor, 2);
		lua_rawset(L, fields);
	}

	// push especial eq operator that compares userdata
	if (typeInfo.pushType != esl::BY_VALUE)
	{
		// mt[__eq] = function ()
		lua_pushstring(L, "__eq");
		lua_pushcclosure(L, &esl::runtime::UserTypeCompareBoxedPointers, 0);
		lua_rawset(L, mt);
	}

	for (const esl::Member& mem : typeInfo.members)
	{
		{
			defer{
				lua_pop(L, 1);
			};
			lua_pushstring(L, mem.name);
			lua_rawget(L, fields);
			if (lua_type(L, -1) != LUA_TNIL)
			{
				ASSERT_FAIL("Class can't have same multiple functions with same name (%s:%s)", typeInfo.className, mem.name);
				continue;
			}
		}

		if (mem.type == esl::MEMB_C_FUNC)
		{
			// fields[name] = function [funcPtr] ()
			lua_pushstring(L, mem.name);
			lua_pushlightuserdata(L, mem.data);
			lua_pushcclosure(L, mem.staticFunc, 1);
			lua_rawset(L, fields);
		}
		else if (mem.type == esl::MEMB_FUNC)
		{
			// fields[name] = function [thisGetter, className, typeInfoMember] ()
			lua_pushstring(L, mem.name);
			lua_pushlightuserdata(L, reinterpret_cast<void*>(typeInfo.thisGetter));
			lua_pushstring(L, typeInfo.className);
			lua_pushlightuserdata(L, const_cast<esl::Member*>(&mem));
			lua_pushcclosure(L, &esl::runtime::UserTypeCallMemberFunc, 3);
			lua_rawset(L, fields);
		}
		else if (mem.type == esl::MEMB_OPERATOR)
		{
			// fields[name] = function ()
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
			lua_rawset(L, fields);
		}
	}

	lua_pushvalue(L, fields);
	lua_setmetatable(L, fields); // set fields as it's own metatable
	lua_pop(L, 2);
}

}