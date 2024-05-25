#pragma once

#include <lua.hpp>

//#define ESL_TRACE

#ifdef ESL_TRACE
#define ESL_VERBOSE_LOG(fmt, ...)	MsgWarning("[ESL] " fmt "\n", __VA_ARGS__)
#else
#define ESL_VERBOSE_LOG(fmt, ...)
#endif // ESL_TRACE

namespace esl
{
using StaticFunc = lua_CFunction;

template <typename T>
struct IsConstMemberFunc : std::false_type {};

template <typename R, typename C, typename... Args>
struct IsConstMemberFunc<R(C::*)(Args...) const> : std::true_type {};

template<typename T>
using BaseType = typename std::remove_cv<typename std::remove_pointer<typename std::remove_reference<T>::type>::type>::type;

template<typename T>
using BasePtrType = typename std::remove_cv<typename std::remove_pointer<T>::type>::type;

template<typename T>
using BaseRefType = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename T>
struct LuaTypeByVal : std::false_type {};

template <typename T>
struct LuaTypeRefCountedObj : std::false_type {};

//template <typename T>
//struct LuaTypeWeakRefObject : std::false_type {};

template <typename T>
struct LuaTypeAlias
{
	static const char* value;
};

// parameter trait 
template <typename T> struct ToCpp {};	// input value ownership is given to native/c++
template <typename T> struct ToLua {};	// input value ownership is given to Lua

template <typename T>
struct StripTraits 
{
	using type = T;
};

template <typename T>
struct StripTraits<ToCpp<T>>
{
	using type = T;
};

template <typename T>
struct StripTraits<ToLua<T>>
{
	using type = T;
};

template <typename T>
using StripTraitsT = typename StripTraits<T>::type;

template <typename T>
struct StripRefPtr
{
	using type = T;
};

template <typename T>
struct StripRefPtr<CRefPtr<T>>
{
	using type = T;
};

template <typename T>
using StripRefPtrT = typename StripRefPtr<T>::type;

template<typename T> struct HasToCppParamTrait : std::false_type {};
template<typename T> struct HasToCppParamTrait<ToCpp<T>> : std::true_type {};

template<typename T> struct HasToLuaReturnTrait : std::false_type {};
template<typename T> struct HasToLuaReturnTrait<ToLua<T>> : std::true_type {};

template <typename T>
struct LuaBaseTypeAlias : LuaTypeAlias<StripRefPtrT<BaseType<StripTraitsT<T>>>> {};


enum EMemberType : int
{
	MEMB_NULL = 0,

	MEMB_CONST,
	MEMB_DTOR,
	MEMB_CTOR,
	MEMB_FUNC,
	MEMB_C_FUNC,
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
	const char*		name{ nullptr };
	const char*		signature{ nullptr };
	void*			data{ nullptr };
	struct {
		union {
			BindFunc	func;
			StaticFunc	staticFunc;
		};
		BindFunc		getFunc;
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
	V&			operator*() { return value; }
	const V&	operator*() const { return value; }
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

void RegisterType(lua_State* L, esl::TypeInfo typeInfo);
}

namespace esl::runtime
{
void		SetLuaErrorFromTopOfStack(lua_State* L);
void		ResetErrorValue(lua_State* L);
const char*	GetLastError(lua_State* L);
int			StackTrace(lua_State* L);

template<typename T, typename WT = T>
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

/// script engine class registrator
template<typename T>
struct EqScriptClass
{
	static esl::TypeInfo GetTypeInfo();

	// NOTE: don't access these directly, use typeinfo
	static esl::TypeInfo	baseClassTypeInfo;
	static const char		className[];
	static const char*		baseClassName;
	static bool				isByVal;
};

class EqScriptState
{
public:
	EqScriptState(lua_State* state)
		: m_state(state)
	{
	}

	operator lua_State* () const { return m_state; }
	operator lua_State* () { return m_state; }

	void			ThrowError(const char* fmt, ...) const;

	bool			RunBuffer(IVirtualStream* virtStream, const char* name) const;
	bool			RunChunk(const EqString& chunk, const char* name = "userChunk") const;

	int				GetStackTop() const;
	int				GetStackType(int index) const;

	template<typename T>
	void			SetGlobal(const char* name, const T& value) const;

	template<typename T>
	decltype(auto)	GetGlobal(const char* name) const;

	esl::LuaTable	CreateTable() const;

	template<typename T>
	void			PushValue(const T& value) const;

	template<typename T>
	decltype(auto)	GetValue(int index) const;

	template<typename T>
	void			RegisterClass() const;

	template<typename T, typename K, typename V>
	void			RegisterClassStatic(const K& k, const V& v) const;

	template<typename T>
	esl::LuaTable	GetClassTable() const;

	template<typename T, typename V, typename K>
	decltype(auto)	GetClassStatic(const K& k) const;

	template<typename R, typename ... Args>
	decltype(auto)	CallFunction(const char* name, Args...);
protected:
	lua_State*	m_state{ nullptr };
};
