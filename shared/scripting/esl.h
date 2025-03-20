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

/// Any - special function return value that allows to send multiple return values to Lua (of any type) and also recieve multiple return values when using runtime::FunctionCall
/// 
/// Example use in C and Member function:
///		esl::Any<3> FunctionReturnsThreeValues(const esl::ScriptState& state)
///		{
///			state.PushValue(125); 
///			state.PushValue(true); 
///			state.PushValue("string") 
///		}
/// 
/// Example use in FunctionCall
/// 	using Call = esl::runtime::FunctionCall<esl::Any<2>, int>;
///		auto result = Call::Invoke(myFunctionRef, 555);
///		if (result) {
///		   retValue1 = *state.GetValue<bool>(-2);
///		   retValue2 = *state.GetValue<bool>(-1);
///		}
/// 
template<int NUM_VALUES = -1>
struct Any {
	static constexpr int COUNT = NUM_VALUES;
};

using StaticFunc = lua_CFunction;

template <typename T>
struct IsAny : std::false_type {};

template <int NUM_VALUES>
struct IsAny<Any<NUM_VALUES>> : std::true_type {};

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
	inline static const char* value = "number";
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
// CRefPtr<T> -> T

template <typename T>
struct StripRefPtr { using type = T; };

template <typename T>
struct StripRefPtr<CRefPtr<T>> { using type = T; };

template <typename T>
using StripRefPtrT = typename StripRefPtr<T>::type;

//------------------------
// CWeakPtr<T> -> T

template <typename T>
struct StripWeakPtr { using type = T; };

template <typename T>
struct StripWeakPtr<CWeakPtr<T>> { using type = T; };

template <typename T>
using StripWeakPtrT = typename StripWeakPtr<T>::type;

//------------------------

template <typename T>
struct StripObject { using type = T; };

template <typename T>
struct StripObject<Object<T>> { using type = T; };

template <typename T>
using StripObjectT = typename StripObject<T>::type;

//------------------------

template <typename T>
struct LuaBaseTypeAlias : LuaTypeAlias<StripWeakPtrT<StripRefPtrT<BaseType<StripTraitsT<StripObjectT<T>>>>>, std::is_enum_v<T>> {};

enum EMemberType : int
{
	MEMB_NULL = 0,

	MEMB_DTOR,
	MEMB_CTOR,
	MEMB_FUNC,
	MEMB_C_FUNC,
	MEMB_VAR,
	MEMB_OPERATOR,
};

enum EPushType
{
	BY_REF = 0,
	BY_VALUE,
	REF_PTR,
	WEAK_REF
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

struct TypeInfo;
using TypeInfoGetter = TypeInfo(*)();
using ThisGetterFunc = void* (*)(lua_State* L, bool& isConstRef);

// Type info which is could be used for debugging
struct TypeInfo
{
	TypeInfoGetter		baseGetter;

	const char*			className{ nullptr };
	const char*			baseClassName{ nullptr };

	ArrayCRef<Member>	members{ nullptr };
	ThisGetterFunc		thisGetter{ nullptr };
	EPushType			pushType{ BY_REF };
};

namespace binder {
	struct ObjectIndexGetter;
}

namespace bindings {}

namespace runtime {
class StackGuard
{
public:
	StackGuard() = default;
	StackGuard(StackGuard&& other) noexcept;
	StackGuard(lua_State* L, int offset = 0);
	~StackGuard();

	StackGuard&	operator=(StackGuard&& other) noexcept;
	int			Pos() const { return m_pos; }

private:
	lua_State*	m_state{ nullptr };
	int			m_pos{ 0 };
};
}

template<typename V>
struct ResultWithValue
{
	operator	bool() { return success; }
	V&			operator*() { return value; }
	const V&	operator*() const { return value; }

	runtime::StackGuard		guard;
	bool					success{ false };
	EqString				errorMessage;
	V						value;
};

template<>
struct ResultWithValue<void>
{
	operator bool() { return success; }
	runtime::StackGuard		guard;
	bool					success{ false };
	EqString				errorMessage;
};

// declared in esl_luaref.h
class LuaRawRef;
class LuaTable;

/// script engine class registrator
template<typename T>
struct ScriptClass
{
	using BindType = T;

	static TypeInfo			GetTypeInfo();

	// NOTE: don't access these directly, use typeinfo
	static const char		className[];

	static TypeInfoGetter		baseClassTypeInfoGetter;
	static const char*			baseClassName;
	static uint					classId;
};

template<typename T>
struct PushType;

template<typename T>
struct BaseScriptClass; // Type

template<> inline TypeInfo ScriptClass<void>::GetTypeInfo() { return {}; }
template<> inline const char ScriptClass<void>::className[] = "";
template<> inline TypeInfoGetter ScriptClass<void>::baseClassTypeInfoGetter = nullptr;
template<> inline const char* ScriptClass<void>::baseClassName = nullptr;
template<> inline uint ScriptClass<void>::classId = 0;

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

	// stops the garbage collector. 
	void			GCStop() const;

	// restarts the garbage collector.
	void			GCRestart() const;

	// performs an incremental step of garbage collection.
	void			GCStep(int stepSize) const;

	// perform full garbage collection cycle
	void			GCCollect() const;

	bool			RunChunk(EqStringRef chunk, const char* name = "userChunk") const;

	bool			RunFileBuffer(IVirtualStream* virtStream, const char* name, const char* mode = nullptr) const;
	int				LoadFileBuffer(IVirtualStream* virtStream, const char* name, const char* mode = nullptr) const;

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
	decltype(auto)	CallFunction(const char* name, Args...) const;

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

	bool		IsValid() const { return m_state != nullptr; }
	bool		IsNull() const { return m_state == nullptr || lua_type(m_state, m_index) == LUA_TNIL; }
	int			GetLuaType() const { return lua_type(m_state, m_index); }

	T&			operator*() const { return Get(); }
	T*			operator->() const { return GetPtr(); }

	operator bool() const { return !IsNull(); }

private:
	lua_State*	m_state{ nullptr };
	int			m_index{ 0 };
};

}

namespace esl::runtime
{
struct BaseClassInfo
{
	EqStringRef name;
	intptr_t	offset{ 0 };	// offset bytes for upcasting

	bool		IsValid() const { return name.IsValid() && name.Length() > 0; }
};

void					SetLuaErrorFromTopOfStack(lua_State* L);
void					ResetErrorValue(lua_State* L);
const char*				GetLastError(lua_State* L);
int						HandleRuntimeError(lua_State* L);

lua_CFunction			SetErrorHandler(lua_CFunction handler);

// Registers type in the specific lua state
void					RegisterType(lua_State* L, esl::TypeInfo typeInfo);

// Creates new user object and immediately pushes it to stack
template<typename T, typename... Args>
static T&				New(lua_State* L, Args&&... args);

// Pushes user object or fundamental value to stack
template<typename T, typename WT = T>
static void				PushValue(lua_State* L, const T& value);

// Returns a T value from stack by index. Allows to specify pointer/reference in T type
template<typename T, bool SilentTypeCheck, bool AllowUpcasting = true>
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

