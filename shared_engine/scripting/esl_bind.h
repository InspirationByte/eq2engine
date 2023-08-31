#pragma once

#include "esl_luaref.h"
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
	OP_call
};
}

// The binder itself.
namespace esl::bindings
{
struct BaseClassStorage
{
	static Map<int, EqString> baseClassNames;

	template<typename T>
	static void Add();

	template<typename T>
	static const char* Get();

	static const char* Get(const char* className);
};

template <typename T>
struct ClassBinder
{
	using BindClass = T;
	static ArrayCRef<Member> GetMembers();

	static Member MakeDestructor();

	template<typename R, typename... Args>
	static Member MakeFunction(R(T::* func)(Args...), const char* name);

	template<auto V>
	static Member MakeVariable(const char* name);

	template<typename ...Args>
	static Member MakeConstructor();

	template<binder::EOpType OpType>
	static Member MakeOperator(const char* name);
};
}

//---------------------------------------------------

/// script engine class registrator
template<typename T>
struct EqScriptClass
{
	static esl::TypeInfo GetTypeInfo();

	static void Register(lua_State* L);

	// NOTE: don't access these directly, use typeinfo

	static esl::TypeInfo	baseClassTypeInfo;
	static const char		className[];
	static const char*		baseClassName;
	static bool				isByVal;
};

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

namespace esl
{
void RegisterType(lua_State* L, esl::TypeInfo typeInfo);
}

#include "esl_bind.hpp"

#define BY_REF(x)
#define BY_VALUE(x) template<> struct esl::LuaTypeByVal<x> : std::true_type {};

// type name definition
#define EQSCRIPT_ALIAS_TYPE(x, n) \
	template<> const char* esl::LuaTypeAlias<x>::value = n;

// Basic type binder
#define EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	EQSCRIPT_ALIAS_TYPE(Class, name) \
	template<> const char EqScriptClass<Class>::className[] = name; \
	type(Class)

// Binder for class without parent type that was bound
#define EQSCRIPT_BIND_TYPE_NO_PARENT(Class, name, type) \
	EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	template<> bool EqScriptClass<Class>::isByVal = esl::LuaTypeByVal<Class>::value; \
	template<> const char* EqScriptClass<Class>::baseClassName = nullptr; \
	template<> esl::TypeInfo EqScriptClass<Class>::baseClassTypeInfo = {}; \

// Binder for class that has bound parent class
#define EQSCRIPT_BIND_TYPE_WITH_PARENT(Class, ParentClass, name, type) \
	EQSCRIPT_BIND_TYPE_BASICS(Class, name, type) \
	template<> bool EqScriptClass<Class>::isByVal = esl::LuaTypeByVal<ParentClass>::value; \
	template<> const char* EqScriptClass<Class>::baseClassName = EqScriptClass<ParentClass>::className; \
	template<> esl::TypeInfo EqScriptClass<Class>::baseClassTypeInfo = EqScriptClass<ParentClass>::GetTypeInfo(); \

#define EQSCRIPT_BIND_CONSTRUCTOR(...)			MakeConstructor<__VA_ARGS__>(),
#define EQSCRIPT_BIND_FUNC(Name)				MakeFunction(&BindClass::Name, #Name),
#define EQSCRIPT_BIND_FUNC_OVERLOAD(Name, ...)	MakeFunction<__VA_ARGS__>(&BindClass::Name, #Name),
#define EQSCRIPT_BIND_OP(Name)					MakeOperator<binder::OP_##Name>("__" #Name),

#define EQSCRIPT_BIND_FUNC_NAMED(FuncName, Name)				MakeFunction(&BindClass::Name, FuncName),
#define EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD(FuncName, Name, ...)	MakeFunction<__VA_ARGS__>(&BindClass::Name, FuncName),

#define EQSCRIPT_BIND_VAR(Name)			MakeVariable<&BindClass::Name>(#Name),

// Begin binding of members
#define EQSCRIPT_BEGIN_BIND(Class) \
	template<> ArrayCRef<esl::Member> esl::bindings::ClassBinder<Class>::GetMembers() { \
		esl::bindings::BaseClassStorage::Add<BindClass>();\
		static Member members[] = { \
			MakeDestructor(),

// End member binding
#define EQSCRIPT_END_BIND()	\
			{} /* default/end element */ \
		}; \
		return ArrayCRef<Member>(members, elementsOf(members) - 1); \
	}
