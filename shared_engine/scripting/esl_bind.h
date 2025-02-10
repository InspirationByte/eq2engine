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
struct LuaCFunctionProto { };

template<typename...UpVals>
struct LuaCFunction : LuaCFunctionProto
{
	// Since this binder is a little bit fucked up and I tremendously suck at templates,
	// callable (which is engine's EqFunction) is not supported 
	// but there are upvalues built into C function binds.
	using TupleVal = std::tuple<UpVals...>;
	TupleVal		upValues;

	void*			funcPtr;
	lua_CFunction	luaFuncImpl;
};

struct BaseClassStorage
{
	using Info = runtime::BaseClassInfo;
	static Map<uint, Info>&	GetBaseClassNames();

	static Info				GetUpcastingBaseClassInfo(const char* className, const char* targetClassName);

	template<typename T>
	static void				Add();

	template<typename T>
	static Info				Get();
	static Info				Get(const char* className);
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

namespace esl
{
template<typename T>
TypeInfo ScriptClass<T>::GetTypeInfo()
{
	return {
		ScriptClass<T>::baseClassTypeInfoGetter,
		ScriptClass<T>::className,
		ScriptClass<T>::baseClassName,
		bindings::ClassBinder<T>::GetMembers(),
		runtime::PushGet<T>::GetThis,
		LuaTypeByVal<T>::value
	};
};

template<typename T, typename... Args>
Object<T> ScriptState::MakeObject(Args&&... args)
{
	runtime::New<T>(m_state, std::forward<Args>(args)...);
	return Object<T>(m_state, lua_gettop(m_state));
}

template<typename T>
void ScriptState::SetGlobal(const char* name, const T& value) const
{
	runtime::SetGlobal(m_state, name, value);
}

template<typename T>
decltype(auto) ScriptState::GetGlobal(const char* name) const
{
	return runtime::GetGlobal<T>(m_state, name);
}

template<typename T>
void ScriptState::PushValue(const T& value) const
{
	runtime::PushValue(m_state, value);
}

template<typename T>
decltype(auto) ScriptState::GetValue(int index) const
{
	return runtime::GetValue<T, true>(m_state, index);
}

template<typename T>
void ScriptState::RegisterClass() const
{
	runtime::RegisterType(m_state, ScriptClass<T>::GetTypeInfo());
}

template<typename T, typename K, typename V>
void ScriptState::RegisterClassStatic(const K& k, const V& v) const
{
	lua_getglobal(m_state, ScriptClass<T>::className);
	const int top = lua_gettop(m_state);
	LuaTable metaTable(m_state, top);
	metaTable.Set(k, v);
	lua_pop(m_state, 1); // getglobal
}

template<typename T>
LuaTable ScriptState::GetClassTable() const
{
	lua_getglobal(m_state, ScriptClass<T>::className);
	const int top = lua_gettop(m_state);
	LuaTable metaTable(m_state, top);
	lua_pop(m_state, 1); // getglobal
	return metaTable;
}

template<typename T, typename V, typename K>
decltype(auto) ScriptState::GetClassStatic(const K& k) const
{
	lua_getglobal(m_state, ScriptClass<T>::className);
	const int top = GetStackTop();
	LuaTable metaTable(m_state, top);
	lua_pop(m_state, 1); // getglobal
	return metaTable.Get<V>(k);
}

template<typename R, typename ... Args>
decltype(auto) ScriptState::CallFunction(const char* name, Args...args) const
{
	using FuncSignature = runtime::FunctionCall<R, Args...>;
	lua_getglobal(m_state, name);
	const int top = GetStackTop();
	return FuncSignature::Invoke(m_state, top, 1, std::forward<Args>(args)...);		
}
}

//---------------------------------------------------

/*

NOTE on weak pointers:
	Since Lua does not invoke userdata's __eq operator when comparing userdata to nil or other things,
	It is required (when necessary) for programmer to bind special funciton that could check weak pointer for it's validty:

		class TestWeakPtr : WeakRefObject<TestWeakPtr> { ... }

		static bool IsObjectDestroyed(const TestWeakPtr* weakObj) { return weakObj == nullptr; }
		state.SetGlobal("isweaknil", EQSCRIPT_CFUNC(IsObjectDestroyed));

	 Since ESL itself itself pushes NULL to the C++ code when object is destroyed, it goes with simple nullptr check;
*/

#include "esl_bind.hpp"

//------------------------------------------------------
// Event declaration helpers

#define _ESL_EVENT_NAME(Name)		m_eslEvent##Name

// Declares event for a bound class
#define ESL_DECLARE_EVENT(Name)		esl::LuaEvent _ESL_EVENT_NAME(Name)

// Declares event caller to be used with ESL_CALL_EVENT
#define ESL_DECLARE_EVENT_CALL(Obj, EventName, ...) \
	auto EventName##Caller = esl::LuaEventCaller<__VA_ARGS__>((Obj)->_ESL_EVENT_NAME(EventName));

// calls the event
#define ESL_CALL_EVENT(EventName, ...) \
	EventName##Caller.Invoke(__VA_ARGS__)

//------------------------------------------------------
// Type binder

#define ESL_ENUM(x) // it was LuaTypeAlias before but now left as placeholder macro for future use only

// usage: INHERIT_PARENT
#define _ESL_PUSH_INHERIT_PARENT(x, parentClass) \
	template<> struct LuaTypeByVal<x> : LuaTypeByVal<parentClass> {}; \
	template<> struct LuaTypeRefCountedObj<x> : LuaTypeRefCountedObj<parentClass> {}; \
	template<> struct LuaTypeWeakRefObj<x> : LuaTypeWeakRefObj<parentClass> {};

// usage: BY_REF
#define _ESL_PUSH_BY_REF(x, _)
	// no traits applied

// usage: BY_VALUE
#define _ESL_PUSH_BY_VALUE(x, _) \
	template<> struct LuaTypeByVal<x> : std::true_type {};

// usage: REF_PTR
#define _ESL_PUSH_REF_PTR(x, _)	\
	template<> struct LuaTypeRefCountedObj<x> : std::true_type {};

// usage: WEAK_REF
#define _ESL_PUSH_WEAK_REF(x, _) \
	template<> struct LuaTypeWeakRefObj<x> : std::true_type {};

// Basic type binder
#define _ESL_BIND_TYPE_BASICS(Class, ParentClass, name, pushType, ...) \
	namespace esl { \
	template<> struct BaseScriptClass<Class> : ScriptClass<ParentClass> {}; \
	template<> inline const char ScriptClass<Class>::className[] = name; \
	template<> inline uint ScriptClass<Class>::classId = StringIdConst24(name); \
	template<> inline const char* ScriptClass<Class>::baseClassName = BaseScriptClass<Class>::className; \
	template<> inline const char* LuaTypeAlias<Class, false>::value = ScriptClass<Class>::className; \
	_ESL_PUSH_##pushType(Class, __VA_ARGS__) \
	}

// Bind class without parent type that was bound
#define EQSCRIPT_BIND_TYPE_NO_PARENT(Class, name, pushType) \
	_ESL_BIND_TYPE_BASICS(\
		Class \
		, void \
		, name \
		, pushType \
	)

// Bind class that has bound parent class
#define EQSCRIPT_BIND_TYPE_WITH_PARENT(Class, ParentClass, name) \
	_ESL_BIND_TYPE_BASICS(\
		  Class \
		, ParentClass \
		, name \
		, INHERIT_PARENT, ParentClass \
	)

// Bind class that has bound parent class with specifying alternate push type
#define EQSCRIPT_BIND_TYPE_WITH_PARENT_EX(Class, ParentClass, name, pushType) \
	_ESL_BIND_TYPE_BASICS(\
		  Class \
		, ParentClass \
		, name \
		, pushType \
	)

#define _ESL_CLASS_MEMBER(Name) 		(&BindClass::Name)
#define _ESL_CLASS_OVERLOAD(R, ...) 	static_cast<R(BindClass::*) __VA_ARGS__>
#define _ESL_CFUNC_OVERLOAD(R, ...) 	static_cast<R(*)__VA_ARGS__>

#define ESL_APPLY_TRAITS(...)		, __VA_ARGS__

// Constructor([ ArgT1, ArgT2, ...ArgTN ])
#define EQSCRIPT_BIND_CONSTRUCTOR(...) \
	MakeConstructor<__VA_ARGS__>(),

#define EQSCRIPT_CLONE_FUNC() \
	MakeStaticFunction<ToLua<BindClass*>, const BindClass&>(+[](const BindClass& self) -> BindClass* { return PPNew BindClass(self); }, "Clone"),

#define EQSCRIPT_BIND_STATIC_FUNC(FuncName, Func, ...) \
	MakeStaticFunction<__VA_ARGS__>(Func, FuncName),

#define EQSCRIPT_BIND_STATIC_FUNC_OVERLOAD(FuncName, Func, R, Signature, ...) \
	MakeStaticFunction<__VA_ARGS__>(_ESL_CFUNC_OVERLOAD(R, Signature)(Func), FuncName),

// Func(Name, [ ESL_APPLY_TRAITS(rgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC(Name, ...) \
	MakeFunction<_ESL_CLASS_MEMBER(Name)__VA_ARGS__>(#Name),

// Func(Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_OVERLOAD(Name, R, Signature, ...) \
	MakeFunction<_ESL_CLASS_OVERLOAD(R, Signature) _ESL_CLASS_MEMBER(Name)__VA_ARGS__>(#Name),

// Func("StrName", Name, [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_NAMED(FuncName, Name, ...) \
	MakeFunction<_ESL_CLASS_MEMBER(Name)__VA_ARGS__>(FuncName),

// Func("StrName", Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD(FuncName, Name, R, Signature, ...) \
	MakeFunction<_ESL_CLASS_OVERLOAD(R, Signature) _ESL_CLASS_MEMBER(Name)__VA_ARGS__>(FuncName),

#define EQSCRIPT_BIND_OP(Name) \
	MakeOperator<binder::OP_##Name>("__" #Name),

#define EQSCRIPT_BIND_OP_CUSTOM(Func, Name) \
	MakeOperator(&Func<BindClass, binder::OP_##Name>, "__" #Name),

#define EQSCRIPT_BIND_OP_TOSTRING(Func) \
	MakeOperator(binder::ToStringOperator<BindClass, &Func>, "__tostring"),

#define EQSCRIPT_BIND_VAR(Name) \
	MakeVariable<_ESL_CLASS_MEMBER(Name)>(#Name),

#define EQSCRIPT_BIND_VAR_NAMED(Name, Var) \
	MakeVariable<_ESL_CLASS_MEMBER(Var)>(Name),

#define EQSCRIPT_BIND_VAR_EX_SET(Name, SetterName) \
	MakeVariableWithSetter<_ESL_CLASS_MEMBER(Name), _ESL_CLASS_MEMBER(SetterName)>(#Name),

#define EQSCRIPT_BIND_VAR_EX_SET_NAMED(Name, Ver, SetterName) \
	MakeVariableWithSetter<_ESL_CLASS_MEMBER(Ver), _ESL_CLASS_MEMBER(SetterName)>(Name),

#define EQSCRIPT_BIND_EVENT(Name) \
	MakeVariable<_ESL_CLASS_MEMBER(_ESL_EVENT_NAME(Name))>(#Name),

// Func(Name, [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_CFUNC(Name, ...) \
	esl::binder::BindCFunction<__VA_ARGS__>(Name)

// Func(Name, Ret, (ArgT1, ArgT2, ...ArgTN), [ ESL_APPLY_TRAITS(ArgT1, ArgT2, ...ArgTN) ])
#define EQSCRIPT_CFUNC_OVERLOAD(Name, R, Signature,...) \
	esl::binder::BindCFunction<__VA_ARGS__>(_ESL_CFUNC_OVERLOAD(R, Signature)(Name))

// Begin binding of members
#define EQSCRIPT_TYPE_BEGIN(Class) \
	namespace esl { \
	template struct ScriptClass<Class>; \
	template<> TypeInfoGetter ScriptClass<Class>::baseClassTypeInfoGetter = BaseScriptClass<Class>::GetTypeInfo; \
	namespace runtime { \
		template<> PushGet<Class>::PushFunc		PushGet<Class>::Push	= &PushGetImpl<Class>::PushObject; \
		template<> PushGet<Class>::GetFunc		PushGet<Class>::Get		= &PushGetImpl<Class>::GetObject; \
		template<> PushGet<Class>::GetThisFunc	PushGet<Class>::GetThis = &PushGetImpl<Class>::GetThis; \
	} \
	namespace bindings { \
		template<> ArrayCRef<Member> ClassBinder<Class>::GetMembers() { \
		BaseClassStorage::Add<BindClass>(); \
		static Member members[] = { MakeDestructor(), \

// End member binding
#define EQSCRIPT_TYPE_END	\
		}; \
		return members; \
	} \
	} /* bindings */ } // esl
