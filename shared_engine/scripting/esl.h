#pragma once

#include <lua.hpp>

namespace esl
{
using StaticFunc = lua_CFunction;

template<typename T>
using BaseType = typename std::remove_cv<typename std::remove_pointer<typename std::remove_reference<T>::type>::type>::type;

template <typename T>
struct LuaTypeByVal : std::false_type {};

template <typename T>
struct LuaTypeAlias
{
	static const char* value;
};

template <typename T>
struct LuaBaseTypeAlias : LuaTypeAlias<BaseType<T>> {};

enum EMemberType : int
{
	MEMB_NULL = 0,

	MEMB_CONST,
	MEMB_DTOR,
	MEMB_CTOR,
	MEMB_FUNC,
	MEMB_VAR,
	MEMB_OPERATOR,
};

struct Member;

// The base proxy class that binds script runtime
// and native code.
struct ScriptBind
{
	void* thisPtr{ nullptr }; // used for function invocation

	using BindFunc = int (ScriptBind::*)(lua_State*);	
};

// Member function registrator
struct Member
{
	using BindFunc = int (ScriptBind::*)(lua_State*);

	EMemberType		type{ MEMB_NULL };
	int				nameHash{ 0 };
	const char*		name{ nullptr };
	const char*		signature{ nullptr };
	struct {
		union {
			BindFunc	func;
			StaticFunc	staticFunc;
		};
		BindFunc	getFunc;
	};
	int				numArgs{ -1 };
	bool			isConst{ false };
};

// Type info which is could be used for debugging
struct TypeInfo
{
	TypeInfo*			base{ nullptr };

	const char*			className{ nullptr };
	const char*			baseClassName{ nullptr };

	ArrayCRef<Member>	members{ nullptr };
	bool				isByVal{ false };
};

template<typename V>
struct ResultWithValue
{
	operator bool() { return success; }
	V			operator*() { return value; }
	const V		operator*() const { return value; }
	bool		success{ false };
	EqString	errorMessage;
	V			value;
};

template<>
struct ResultWithValue<void>
{
	operator bool() { return success; }
	bool		success{ false };
	EqString	errorMessage;
};

namespace binder {}
namespace bindings {}
namespace runtime {}
}

namespace esl::runtime
{
template<typename T>
static void PushValue(lua_State* L, const T& value);

template<typename T, bool SilentTypeCheck>
static decltype(auto) GetValue(lua_State* L, int index);

template<typename T>
static ResultWithValue<T> GetGlobal(lua_State* L, const char* fieldName);

template<typename R, typename ... Args>
struct FunctionCall;
}

