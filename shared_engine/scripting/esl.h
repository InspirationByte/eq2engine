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

// declared in esl_luaref.h
class LuaRawRef;
class LuaTable;
}

namespace esl::runtime
{
void		SetLuaErrorFromTopOfStack(lua_State* L);
void		ResetErrorValue(lua_State* L);
const char*	GetLastError(lua_State* L);

template<typename T>
static void PushValue(lua_State* L, const T& value);

template<typename T, bool SilentTypeCheck>
static decltype(auto) GetValue(lua_State* L, int index);

template<typename T>
static decltype(auto) GetGlobal(lua_State* L, const char* fieldName);

template<typename T>
static void SetGlobal(lua_State* L, const char* fieldName, const T& value);

template<typename R, typename ... Args>
struct FunctionCall;
}

class EqScriptState
{
public:
	EqScriptState(lua_State* state)
		: m_state(state)
	{
	}

	operator lua_State* () const { return m_state; }
	operator lua_State* () { return m_state; }

	void ThrowError(const char* fmt, ...) const;

	bool RunBuffer(IVirtualStream* virtStream, const char* name) const;
	bool RunChunk(const EqString& chunk) const;

	template<typename T>
	void SetGlobal(const char* name, const T& value) const
	{
		esl::runtime::SetGlobal(m_state, name, value);
	}

	template<typename T>
	decltype(auto) GetGlobal(const char* name) const
	{
		return esl::runtime::GetGlobal<T>(m_state, name);
	}

	esl::LuaTable CreateTable() const;

	template<typename T>
	void PushValue(const T& value) const
	{
		esl::runtime::PushValue(m_state, value);
	}

	template<typename T>
	decltype(auto) GetValue(int index) const
	{
		return esl::runtime::GetValue<T>(m_state, value);
	}

	template<typename T>
	void RegisterClass() const
	{
		esl::RegisterType(m_state, EqScriptClass<T>::GetTypeInfo());
	}

protected:
	lua_State*	m_state{ nullptr };
};


