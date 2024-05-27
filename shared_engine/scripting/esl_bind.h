#pragma once

#include "esl_event.h"
#include "esl_runtime.h"

namespace esl::binder
{
enum EOpType : int
{
	OP_add,
	OP_sub,
	OP_mul,
	OP_div,
	OP_mod,
	OP_pow,
	OP_unm,
	OP_idiv,
	OP_band,
	OP_bor,
	OP_xor,
	OP_not,
	OP_shl,
	OP_shr,
	OP_concat,
	OP_len,
	OP_eq,
	OP_lt,
	OP_le,
};
}

// The binder itself.
namespace esl::bindings
{
struct LuaCFunction
{
	void*			funcPtr;
	lua_CFunction	luaFuncImpl;
};

struct BaseClassStorage
{
	static Map<int, EqString>& GetBaseClassNames();

	template<typename T>
	static void			Add();

	template<typename T>
	static const char*	Get();

	static const char*	Get(const char* className);
};

template <typename T>
struct ClassBinder
{
	using BindClass = T;
	static ArrayCRef<Member>	GetMembers();

	static Member	MakeDestructor();

	template<typename UR = void, typename ... UArgs, typename F>
	static Member	MakeStaticFunction(F func, const char* name);

	template<auto F, typename UR = void, typename ... UArgs>
	static Member	MakeFunction(const char* name);

	template<auto V>
	static Member	MakeVariable(const char* name);

	template<auto V, auto F>
	static Member	MakeVariableWithSetter(const char* name);

	template<typename ...Args>
	static Member	MakeConstructor();

	template<binder::EOpType OpType>
	static Member	MakeOperator(const char* name);

	template<typename F>
	static Member	MakeOperator(F func, const char* name);
};
}

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
}

template<typename T>
esl::TypeInfo EqScriptClass<T>::GetTypeInfo()
{
	return {
		&EqScriptClass<T>::baseClassTypeInfo,
		EqScriptClass<T>::className,
		EqScriptClass<T>::baseClassName,
		esl::bindings::ClassBinder<T>::GetMembers(),
		EqScriptClass<T>::isByVal
	};
};


template<typename T>
void EqScriptState::SetGlobal(const char* name, const T& value) const
{
	esl::runtime::SetGlobal(m_state, name, value);
}

template<typename T>
decltype(auto) EqScriptState::GetGlobal(const char* name) const
{
	return esl::runtime::GetGlobal<T>(m_state, name);
}

template<typename T>
void EqScriptState::PushValue(const T& value) const
{
	esl::runtime::PushValue(m_state, value);
}

template<typename T>
decltype(auto) EqScriptState::GetValue(int index) const
{
	return esl::runtime::GetValue<T, true>(m_state, index);
}

template<typename T>
void EqScriptState::RegisterClass() const
{
	esl::RegisterType(m_state, EqScriptClass<T>::GetTypeInfo());
}

template<typename T, typename K, typename V>
void EqScriptState::RegisterClassStatic(const K& k, const V& v) const
{
	lua_getglobal(m_state, EqScriptClass<T>::className);
	const int top = lua_gettop(m_state);
	esl::LuaTable metaTable(m_state, top);
	metaTable.Set(k, v);
	lua_pop(m_state, 1); // getglobal
}

template<typename T>
esl::LuaTable EqScriptState::GetClassTable() const
{
	lua_getglobal(m_state, EqScriptClass<T>::className);
	const int top = lua_gettop(m_state);
	esl::LuaTable metaTable(m_state, top);
	lua_pop(m_state, 1); // getglobal
	return metaTable;
}

template<typename T, typename V, typename K>
decltype(auto) EqScriptState::GetClassStatic(const K& k) const
{
	lua_getglobal(m_state, EqScriptClass<T>::className);
	const int top = GetStackTop();
	esl::LuaTable metaTable(m_state, top);
	return metaTable.Get<V>(k);
}

template<typename R, typename ... Args>
decltype(auto) EqScriptState::CallFunction(const char* name, Args...args)
{
	using FuncSignature = esl::runtime::FunctionCall<R, Args...>;
	lua_getglobal(m_state, name);
	const int top = GetStackTop();
	return FuncSignature::Invoke(m_state, top, std::forward<Args>(args)...);		
}

//---------------------------------------------------

#include "esl_bind.hpp"

#define ESL_PUSH_INHERIT_PARENT(x)
#define ESL_PUSH_BY_REF(x)			/* usage: BY_REF */
#define ESL_PUSH_BY_VALUE(x)		/* usage: BY_VALUE */ \
	template<> struct esl::LuaTypeByVal<x> : std::true_type {};

#define ESL_PUSH_REF_PTR(x)			/* usage: REF_PTR */ \
	template<> struct esl::LuaTypeRefCountedObj<x> : std::true_type {};

// TODO: WEAK_PTR
//#define ESL_PUSH_WEAK_PTR(x)		/* usage: WEAK_PTR */ \
//	template<> struct esl::LuaTypeWeakRefObject<x> : std::true_type {};

#define ESL_CLASS_MEMBER(Name) 		(&BindClass::Name)
#define ESL_CLASS_OVERLOAD(R, ...) 	static_cast<R(BindClass::*) __VA_ARGS__>
#define ESL_CFUNC_OVERLOAD(R, ...) 	static_cast<R(*)__VA_ARGS__>
#define ESL_APPLY_TRAITS(...)		, __VA_ARGS__

// type name definition
#define ESL_ALIAS_TYPE(x, n) \
	template<> inline const char* esl::LuaTypeAlias<x, false>::value = n;

#define ESL_ENUM(x) // it was LuaTypeAlias before but now left as placeholder macro for future use only

#define _ESL_EVENT_NAME(Name)		m_eslEvent##Name

// used for declaring event inside bound class
#define ESL_DECLARE_EVENT(Name)		esl::LuaEvent _ESL_EVENT_NAME(Name)

// declares event caller for use
#define ESL_DECLARE_EVENT_CALL(Obj, EventName, ...) \
	auto EventName##Caller = esl::LuaEventCaller<__VA_ARGS__>((Obj)->_ESL_EVENT_NAME(EventName));

// calls the event
#define ESL_CALL_EVENT(EventName, ...) \
	EventName##Caller.Invoke(__VA_ARGS__)

// Basic type binder
#define EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	ESL_ALIAS_TYPE(Class, name) \
	template<> inline const char EqScriptClass<Class>::className[] = name; \
	ESL_PUSH_##type(Class)

// Binder for class without parent type that was bound
#define EQSCRIPT_BIND_TYPE_NO_PARENT(Class, name, type) \
	EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	template<> inline bool EqScriptClass<Class>::isByVal = esl::LuaTypeByVal<Class>::value; \
	template<> inline const char* EqScriptClass<Class>::baseClassName = nullptr; \
	template<> inline esl::TypeInfo EqScriptClass<Class>::baseClassTypeInfo = {};

// Binder for class that has bound parent class
#define EQSCRIPT_BIND_TYPE_WITH_PARENT(Class, ParentClass, name) \
	EQSCRIPT_BIND_TYPE_BASICS(Class, name, INHERIT_PARENT) \
	template<> inline bool EqScriptClass<Class>::isByVal = esl::LuaTypeByVal<ParentClass>::value; \
	template<> inline const char* EqScriptClass<Class>::baseClassName = EqScriptClass<ParentClass>::className; \
	template<> inline esl::TypeInfo EqScriptClass<Class>::baseClassTypeInfo = EqScriptClass<ParentClass>::GetTypeInfo();

// Binder for class that has bound parent class
#define EQSCRIPT_BIND_TYPE_WITH_PARENT_EX(Class, ParentClass, name, type) \
	EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	template<> inline bool EqScriptClass<Class>::isByVal = esl::LuaTypeByVal<ParentClass>::value; \
	template<> inline const char* EqScriptClass<Class>::baseClassName = EqScriptClass<ParentClass>::className; \
	template<> inline esl::TypeInfo EqScriptClass<Class>::baseClassTypeInfo = EqScriptClass<ParentClass>::GetTypeInfo();

// Constructor([ ArgT1, ArgT2, ...ArgTN ])
#define EQSCRIPT_BIND_CONSTRUCTOR(...) \
	MakeConstructor<__VA_ARGS__>(),

#define EQSCRIPT_BIND_STATIC_FUNC(FuncName, Func, ...) \
	MakeStaticFunction<__VA_ARGS__>(Func, FuncName),

// Func(Name, [ ESL_APPLY_TRAITS(rgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC(Name, ...) \
	MakeFunction<ESL_CLASS_MEMBER(Name)__VA_ARGS__>(#Name),

// Func(Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_OVERLOAD(Name, R, Signature, ...) \
	MakeFunction<ESL_CLASS_OVERLOAD(R, Signature) ESL_CLASS_MEMBER(Name)__VA_ARGS__>(#Name),

// Func("StrName", Name, [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_NAMED(FuncName, Name, ...) \
	MakeFunction<ESL_CLASS_MEMBER(Name)__VA_ARGS__>(FuncName),

// Func("StrName", Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD(FuncName, Name, R, Signature, ...) \
	MakeFunction<ESL_CLASS_OVERLOAD(R, Signature) ESL_CLASS_MEMBER(Name)__VA_ARGS__>(FuncName),

#define EQSCRIPT_BIND_OP(Name) \
	MakeOperator<binder::OP_##Name>("__" #Name),

#define EQSCRIPT_BIND_OP_CUSTOM(Func, Name) \
	MakeOperator(&Func<BindClass, binder::OP_##Name>, "__" #Name),

#define EQSCRIPT_BIND_OP_TOSTRING(Func) \
	MakeOperator(binder::ToStringOperator<BindClass, &Func>, "__tostring"),

#define EQSCRIPT_BIND_VAR(Name) \
	MakeVariable<ESL_CLASS_MEMBER(Name)>(#Name),

#define EQSCRIPT_BIND_VAR_NAMED(Name, Var) \
	MakeVariable<ESL_CLASS_MEMBER(Var)>(Name),

#define EQSCRIPT_BIND_VAR_EX_SET(Name, SetterName) \
	MakeVariableWithSetter<ESL_CLASS_MEMBER(Name), ESL_CLASS_MEMBER(SetterName)>(#Name),

#define EQSCRIPT_BIND_VAR_EX_SET_NAMED(Name, Ver, SetterName) \
	MakeVariableWithSetter<ESL_CLASS_MEMBER(Ver), ESL_CLASS_MEMBER(SetterName)>(Name),

#define EQSCRIPT_BIND_EVENT(Name) \
	MakeVariable<ESL_CLASS_MEMBER(_ESL_EVENT_NAME(Name))>(#Name),

// Func(Name, [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_CFUNC(Name, ...) \
	esl::binder::BindCFunction<__VA_ARGS__>(Name)

// Func(Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_CFUNC_OVERLOAD(Name, R, Signature,...) \
	esl::binder::BindCFunction<__VA_ARGS__>(ESL_CFUNC_OVERLOAD(R, Signature)(Name))

// Begin binding of members
#define EQSCRIPT_TYPE_BEGIN(Class) \
	namespace esl::runtime { \
	template<> PushGet<Class>::PushFunc PushGet<Class>::Push = &PushGetImpl<Class>::PushObject; \
	template<> PushGet<Class>::GetFunc PushGet<Class>::Get = &PushGetImpl<Class>::GetObject; \
	} \
	template<> ArrayCRef<esl::Member> esl::bindings::ClassBinder<Class>::GetMembers() { \
		esl::bindings::BaseClassStorage::Add<BindClass>();\
		static Member members[] = { \
			MakeDestructor(),

// End member binding
#define EQSCRIPT_TYPE_END	\
			{} /* default/end element */ \
		}; \
		return ArrayCRef<Member>(members, elementsOf(members) - 1); \
	}
