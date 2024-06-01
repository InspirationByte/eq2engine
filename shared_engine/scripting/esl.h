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
template <typename T>
class Object;

template<int TYPE>
class LuaRef;

using LuaFunctionRef = LuaRef<LUA_TFUNCTION>;
using LuaTableRef = LuaRef<LUA_TTABLE>;
using LuaUserRef = LuaRef<LUA_TUSERDATA>;

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

template <typename T, bool isEnum>
struct LuaTypeAlias;

template <typename T>
struct LuaTypeAlias<T, false>
{
	static const char* value;
};

template <typename T>
struct LuaTypeAlias<T, true>
{
	static constexpr const char* value = "number";
};

// parameter trait 
template <typename T> struct ToCpp {};	// input value ownership is given to native/c++
template <typename T> struct ToLua {};	// input value ownership is given to Lua

template<typename T> struct HasToCppParamTrait : std::false_type {};
template<typename T> struct HasToCppParamTrait<ToCpp<T>> : std::true_type {};

template<typename T> struct HasToLuaReturnTrait : std::false_type {};
template<typename T> struct HasToLuaReturnTrait<ToLua<T>> : std::true_type {};

//------------------------

template <typename T>
struct StripTraits { using type = T; };

template <typename T>
struct StripTraits<ToCpp<T>> { using type = T; };

template <typename T>
struct StripTraits<ToLua<T>> { using type = T; };

template <typename T>
using StripTraitsT = typename StripTraits<T>::type;

//------------------------

template <typename T>
struct StripRefPtr { using type = T; };

template <typename T>
struct StripRefPtr<CRefPtr<T>> { using type = T; };

template <typename T>
using StripRefPtrT = typename StripRefPtr<T>::type;

//------------------------

template <typename T>
struct StripObject { using type = T; };

template <typename T>
struct StripObject<Object<T>> { using type = T; };

template <typename T>
using StripObjectT = typename StripObject<T>::type;

//------------------------

template <typename T>
struct LuaBaseTypeAlias : LuaTypeAlias<StripRefPtrT<BaseType<StripTraitsT<StripObjectT<T>>>>, std::is_enum_v<T>> {};

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

namespace binder {
	struct ObjectIndexGetter;
}
namespace bindings {}
namespace runtime {}

// declared in esl_luaref.h
class LuaRawRef;
class LuaTable;

/// script engine class registrator
template<typename T>
struct ScriptClass
{
	static esl::TypeInfo	GetTypeInfo();

	// NOTE: don't access these directly, use typeinfo
	static esl::TypeInfo	baseClassTypeInfo;
	static const char		className[];
	static const char*		baseClassName;
};

/// script state wrapper
class ScriptState
{
public:
	ScriptState(lua_State* state)
		: m_state(state)
	{
	}

	operator lua_State* () const { return m_state; }
	operator lua_State* () { return m_state; }

	void			ThrowError(const char* fmt, ...) const;

	bool			RunBuffer(IVirtualStream* virtStream, const char* name) const;
	bool			RunChunk(EqStringRef chunk, const char* name = "userChunk") const;

	int				GetStackTop() const;
	int				GetStackType(int index) const;

	template<typename T>
	void			SetGlobal(const char* name, const T& value) const;

	template<typename T>
	decltype(auto)	GetGlobal(const char* name) const;

	// creates table and
	esl::LuaTable	CreateTable() const;

	// creates new object and pushes it to the stack. Returns object that can be return by functions
	template<typename T, typename... Args>
	esl::Object<T>	MakeObject(Args&&... args);

	// pushes value to the stack
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

/// script object wrapper representing value on stack. Useful for BY_VALUE objects to not be non-copyable
template<typename T>
class Object
{
	friend struct binder::ObjectIndexGetter;
public:
	using TYPE = T;

	Object() = default;
	Object(lua_State* L, int index)
		: m_state(L)
		, m_index(index)
	{
	}
	Object(const Object& other)
		: m_state(other.m_state)
		, m_index(other.m_index)
	{
	}

	T&			Get() const;
	T*			GetPtr() const;

	LuaUserRef	ToRef() const;

	T&			operator*() const { return Get(); }
	T*			operator->() const { return GetPtr(); }

	operator bool() const { return m_state != nullptr; }

private:
	lua_State*	m_state{ nullptr };
	int			m_index{ 0 };
};

}

namespace esl::runtime
{
void					SetLuaErrorFromTopOfStack(lua_State* L);
void					ResetErrorValue(lua_State* L);
const char*				GetLastError(lua_State* L);
int						StackTrace(lua_State* L);

// Registers type in the specific lua state
void					RegisterType(lua_State* L, esl::TypeInfo typeInfo);

// Creates new user object and immediately pushes it to stack
template<typename T, typename... Args>
static T&				New(lua_State* L, Args&&... args);

// Pushes user object or fundamental value to stack
template<typename T, typename WT = T>
static void				PushValue(lua_State* L, const T& value);

// Returns a T value from stack by index. Allows to specify pointer/reference in T type
template<typename T, bool SilentTypeCheck>
static decltype(auto)	GetValue(lua_State* L, int index);

// Pushes user object or fundamental value to global table (_G) by name
template<typename T>
static void				SetGlobal(lua_State* L, const char* fieldName, const T& value);

// Returns a T value from global table (_G) by name. Allows to specify pointer/reference in T type
template<typename T>
static decltype(auto)	GetGlobal(lua_State* L, const char* fieldName);

template<typename R, typename ... Args>
struct FunctionCall;
}

