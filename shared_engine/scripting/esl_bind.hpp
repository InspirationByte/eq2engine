#pragma once

namespace esl
{
enum EUserDataType : int
{
	UD_TYPE_PTR		= 0,	// raw pointer
	UD_TYPE_REFPTR,			// CRefPtr<T>
	UD_TYPE_WEAKPTR,		// CWeakPtr<T>
};

enum EUserDataFlags : int
{
	UD_FLAG_CONST	= (1 << 0),	// a 'const' qualified object
	UD_FLAG_OWNED	= (1 << 1),	// was created by Lua and should be destroyed
};

// boxed userdata
struct BoxUD
{
	void*	objPtr{ nullptr };
	ushort	type{ UD_TYPE_PTR };
	ushort	flags{ 0 };
};
}

//-----------------------------------------------
// TYPE BINDER
namespace esl::runtime
{
template <bool Cond, typename Then, typename OrElse>
decltype(auto) constexpr_if(Then&& then, OrElse&& or_else)
{
	if constexpr (Cond)
		return std::forward<Then>(then);
	else
		return std::forward<OrElse>(or_else);
}

// Function arguments signature generator
// Used mainly for constructors
template <typename... Args>
struct ArgsSignature;

// Base specialization: When there are no types left.
template <>
struct ArgsSignature<> 
{
	static const char* Get() { return ""; }
};

// Recursive specialization: One type followed by rest of the types.
template <typename First, typename... Rest>
struct ArgsSignature<First, Rest...>
{
	static const char* Get()
	{
		static EqString result = []() {
			const char* restStr = ArgsSignature<Rest...>::Get();
			return EqString(LuaBaseTypeAlias<First>::value) + (*restStr ? "," : "") + restStr;
		}();
		return result;
	}
};

// Traits to detect which types are pullable from Lua
template<typename T> struct IsUserObj : std::false_type {};
template<typename T> struct IsUserObj<T*> : std::true_type {};
template<typename T> struct IsUserObj<T&> : std::true_type {};

template<> struct IsUserObj<EqString> : std::false_type {};
template<> struct IsUserObj<EqString*> : std::false_type {};
template<> struct IsUserObj<const EqString*> : std::false_type {};
template<> struct IsUserObj<LuaRawRef> : std::false_type {};
template<> struct IsUserObj<LuaFunctionRef> : std::false_type {};
template<> struct IsUserObj<LuaTableRef> : std::false_type {};

template<typename T> struct IsString : std::false_type {};
template<> struct IsString<char*> : std::true_type {};
template<> struct IsString<const char*> : std::true_type {};
template<> struct IsString<const char*&> : std::true_type {};
template<std::size_t N> struct IsString<const char[N]> : std::true_type {};
template<std::size_t N> struct IsString<char[N]> : std::true_type {};
template<> struct IsString<EqString> : std::true_type {};
template<> struct IsString<EqString&> : std::true_type {};
template<> struct IsString<const EqString&> : std::true_type {};

template<typename T> struct IsEqString : std::false_type {};
template<> struct IsEqString<EqString> : std::true_type {};
template<> struct IsEqString<EqString&> : std::true_type {};
template<> struct IsEqString<const EqString&> : std::true_type {};

// Push pull is essential when you want to send or get values from Lua
template<typename T>
struct PushGet
{
	static void PushObject(lua_State* L, const BaseType<T>& obj, bool ownedByLua, bool isConst)
	{
		// TODO: safe constexpr when file with binding is not included
		if (EqScriptClass<BaseType<T>>::isByVal) // if constexpr (LuaTypeByVal<BaseType<T>>::value)
		{
			BaseType<T>* ud = static_cast<BaseType<T>*>(lua_newuserdata(L, sizeof(BaseType<T>)));
			new(ud) BaseType<T>(obj); // FIXME: use move?
		}
		else
		{
			BoxUD* ud = static_cast<BoxUD*>(lua_newuserdata(L, sizeof(BoxUD)));
			ud->objPtr = const_cast<void*>(reinterpret_cast<const void*>(&obj));
			ud->type = UD_TYPE_PTR;
			ud->flags = (ownedByLua ? UD_FLAG_OWNED : 0) | (isConst ? UD_FLAG_CONST : 0);
		}
		luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);
	}

	static T* GetObject(lua_State* L, int index)
	{
		// TODO: safe constexpr when file with binding is not included
		if (EqScriptClass<BaseType<T>>::isByVal) //if constexpr (LuaTypeByVal<BaseType<T>>::value)
		{
			BaseType<T>* objPtr = static_cast<T*>(lua_touserdata(L, index));
			return objPtr;
		}
		else
		{
			BoxUD* userData = static_cast<BoxUD*>(lua_touserdata(L, index));
			BaseType<T>* objPtr = userData ? static_cast<BaseType<T>*>(userData->objPtr) : nullptr;
			return objPtr;
		}
	}

	static int ObjectDestructor(lua_State* L)
	{
		// destructor is safe to use statically-compiled ByVal
		if constexpr (LuaTypeByVal<T>::value)
		{
			T* userData = static_cast<T*>(lua_touserdata(L, 1));
			userData->~T();
		}
		else
		{
			BoxUD* userData = static_cast<BoxUD*>(lua_touserdata(L, 1));
			if (userData->flags & UD_FLAG_OWNED)
			{
				delete static_cast<T*>(userData->objPtr);
				userData->flags &= ~UD_FLAG_OWNED;
			}
		}
		return 0;
	}
};

template<typename T>
static void PushValue(lua_State* L, const T& value)
{
	if constexpr (std::is_same_v<T, bool>)
	{
		lua_pushboolean(L, value);
	}
	else if constexpr (
		   std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, int8>
		|| std::is_same_v<T, uint8>
		|| std::is_same_v<T, int16>
		|| std::is_same_v<T, uint16>
		|| std::is_same_v<T, int32>
		|| std::is_same_v<T, uint32>
		|| std::is_same_v<T, int64>
		|| std::is_same_v<T, uint64>)
	{
		lua_pushinteger(L, value);
	}
	else if constexpr (
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>)
	{
		lua_pushnumber(L, value);
	}
	else if constexpr (IsString<T>::value)
	{
		lua_pushstring(L, value);
	}
	else if constexpr(std::is_same_v<std::decay<T>::type, lua_CFunction>)
	{
		lua_pushcfunction(L, value);
	}
	else if constexpr (
		   std::is_same_v<T, LuaRawRef>
		|| std::is_same_v<T, LuaFunctionRef>
		|| std::is_same_v<T, LuaTableRef>
		|| std::is_same_v<T, LuaTable>)
	{
		value.Push();
	}
	else if constexpr (IsUserObj<T>::value)
	{
		if constexpr (std::is_pointer_v<T>)
			PushGet<T>::PushObject(L, *value, false, std::is_const_v<T>);
		else
			PushGet<T>::PushObject(L, value, false, std::is_const_v<T>);
	}
	else if constexpr(std::is_same_v<T, std::nullptr_t>)
	{
		lua_pushnil(L);
	}
	else
	{
		// make a copy of object
		PushGet<T>::PushObject(L, *(PPNew T(value)), true, false);
	}
}

// Lua type getters
template<typename T, bool SilentTypeCheck>
static decltype(auto) GetValue(lua_State* L, int index)
{
	using Result = ResultWithValue<T>;

	const int top = lua_gettop(L);

	// TODO: ThrowError
	if(index > top)
	{
		if constexpr (!SilentTypeCheck)
			luaL_error(L, "insufficient number of arguments - %s expected", LuaBaseTypeAlias<T>::value);
	}

	auto checkType = [](lua_State* L, int index, int type) -> bool
	{
		if constexpr (SilentTypeCheck)
		{
			if (lua_type(L, index) != type)
				return false;
		}
		else
			luaL_checktype(L, index, type);
		return true;
	};

	if constexpr (std::is_same_v<T, bool>) 
	{
		if(!checkType(L, index, LUA_TBOOLEAN))
			return Result{ false, {} };

		return Result{ true, {}, lua_toboolean(L, index) != 0 };
    } 
	else if constexpr (
		   std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, int8>
		|| std::is_same_v<T, uint8>
		|| std::is_same_v<T, int16>
		|| std::is_same_v<T, uint16>
		|| std::is_same_v<T, int32>
		|| std::is_same_v<T, uint32>
		|| std::is_same_v<T, int64>
		|| std::is_same_v<T, uint64>)
	{
		if (!checkType(L, index, LUA_TNUMBER))
			return Result{ false, {} };

		return Result{ true, {}, static_cast<T>(lua_tointeger(L, index)) };
	}
	else if constexpr (
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>)
	{
		if (!checkType(L, index, LUA_TNUMBER))
			return Result{ false, {} };

		return Result{ true, {}, static_cast<T>(lua_tonumber(L, index)) };
    } 
	else if constexpr (std::is_same_v<T, LuaRawRef>)
	{
		const int type = lua_type(L, index);
		return Result{ true, {}, LuaRawRef(L, index, type) };
	}
	else if constexpr (std::is_same_v<T, LuaFunctionRef>)
	{
		if (!checkType(L, index, LUA_TFUNCTION))
			return Result{ false, {} };
		return Result{ true, {}, LuaFunctionRef(L, index) };
	}
	else if constexpr (
		   std::is_same_v<T, LuaTableRef>
		|| std::is_same_v<T, LuaTable>)
	{
		if (!checkType(L, index, LUA_TTABLE))
			return Result{ false, {} };
		return Result{ true, {}, T(L, index) };
	}
	else if constexpr (IsString<T>::value)
	{
		if (!checkType(L, index, LUA_TSTRING))
		{
			if constexpr (IsEqString<T>::value)
				return ResultWithValue<EqString>{ true, {}, EqString() };
			else
				return Result{ true, {}, nullptr };
		}

		const char* value = lua_tostring(L, index);
		if constexpr (IsEqString<T>::value)
		{
			static_assert(!std::is_pointer_v<T>, "passing EqString by pointer is not supported yet");
			return ResultWithValue<EqString>{ true, {}, EqString(value) };
		}
		else
			return Result{ true, {}, value };
	}
	else if constexpr (IsUserObj<T>::value)
	{
		const int type = lua_type(L, index);
		if (type != LUA_TUSERDATA)
		{
			EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, type));
			if constexpr(!SilentTypeCheck)
			{
				if (type == LUA_TNIL)
				{
					if constexpr (!std::is_pointer_v<T>)
						luaL_argerror(L, index, err);
				}
				else
					luaL_argerror(L, index, err);
			}

			if constexpr (std::is_reference_v<T>)
				return Result{ false, std::move(err), reinterpret_cast<T>(*(BaseType<T>*)nullptr) };
			else
				return Result{ false, std::move(err), nullptr };
		}
		
		// we still won't allow null value to be pushed if upcasting has failed
		if (!CheckUserdataCanBeUpcasted(L, index, LuaBaseTypeAlias<T>::value))
		{
			const int type = luaL_getmetafield(L, index, "__name");
			const char* className = lua_tostring(L, -1);
			lua_pop(L, 1);

			EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, className);
			if constexpr (!SilentTypeCheck)
				luaL_argerror(L, index, err);

			if constexpr (std::is_reference_v<T>)
				return Result{ false, std::move(err), reinterpret_cast<T>(*(BaseType<T>*)nullptr) };
			else
				return Result{ false, std::move(err), nullptr };
		}

		BaseType<T>* objPtr = static_cast<BaseType<T>*>(PushGet<BaseType<T>>::GetObject(L, index));

		if constexpr (std::is_reference_v<T>)
			return Result{ true, {}, reinterpret_cast<T>(*objPtr) };
		else 
			return Result{ true, {}, reinterpret_cast<T>(objPtr) };
	}
	else 
	{
		ASSERT_FAIL("Unsupported argument type %s", typeid(T).name());

		if constexpr (std::is_reference_v<T>)
			return Result{ false, {}, reinterpret_cast<T>(*(BaseType<T>*)nullptr) };
		else
			return Result{ false, {}, reinterpret_cast<T>(nullptr) };

        // NOTE: GCC struggles here, need to rethink this code
		// static_assert(false, "Unsupported argument type");
	}
}

template<typename T>
static ResultWithValue<T> GetGlobal(lua_State* L, const char* fieldName)
{
	lua_getglobal(L, fieldName);
	defer{
		lua_pop(L, 1);
	};
	return runtime::GetValue<T, true>(L, -1);
}

template<typename T>
static void SetGlobal(lua_State* L, const char* fieldName, const T& value)
{
	runtime::PushValue(L, value);
	lua_setglobal(L, fieldName);
}

template<typename R, typename ... Args>
struct FunctionCall
{
	using Result = ResultWithValue<R>;

	static Result Invoke(const esl::LuaFunctionRef& func, Args... args)
	{
		if (!func.IsValid())
		{
			return Result{ false, "is not callable"};
		}

		lua_State* L = func.GetState();

		// lua_pushcfunction(L, EqLua::LuaBinding_StackTrace);
		// const int errIdx = lua_gettop(L);

		func.Push();
		PushArguments(L, args...);

		const int res = lua_pcall(L, sizeof...(Args), std::is_void_v<R> ? 0 : 1, 0);
		if (res == 0)
		{
			if constexpr (std::is_void_v<R>)
				return Result{ true };
			else
				return Result{ true, {}, *runtime::GetValue<R, false>(L, -1)};
		}

		const char* errorMessage = nullptr;
		if (res != LUA_ERRMEM)
		{
			const int errorIdx = lua_gettop(L);
			errorMessage = lua_tostring(L, errorIdx);
			lua_pop(L, 1);
		}

		return Result{ false, errorMessage };
	}

private:
	static void PushArguments(lua_State* L) {}

	template<typename First, typename... Rest>
	static void PushArguments(lua_State* L, First first, Rest... rest)
	{
		runtime::PushValue(L, first);
		PushArguments(L, rest...);
	}	
};

}

//-----------------------------------------------
// FUNCTION BINDER
namespace esl::binder
{
template<typename T, typename... Args>
struct ConstructorBinder;

// Default constructor binder
template<typename T>
struct ConstructorBinder<T>
{
	static int Func(lua_State* L)
	{
		T* newObj = PPNew T();
		runtime::PushGet<T>::PushObject(L, *newObj, true, false);
		return 1;
	}
};

// Binder specialization for custom constructors
template<typename T, typename... Args>
struct ConstructorBinder 
{
	template<size_t... IDX>
	static void Invoke(lua_State* L, std::index_sequence<IDX...>)
	{
		// Extract arguments from Lua and forward them to the constructor
		T* newObj = PPNew T(*runtime::GetValue<Args, true>(L, IDX + 1)...);
		runtime::PushGet<T>::PushObject(L, *newObj, true, false);
	}

	static int Func(lua_State* L) 
	{
		Invoke(L, std::index_sequence_for<Args...>{});
		return 1;
	}
};

template<typename T>
static auto BindDestructor()
{
	return &runtime::PushGet<T>::ObjectDestructor;
}

template<typename T, typename R, typename ... Args>
struct MemberFunctionBinder
{
	using BindFunc = esl::ScriptBind::BindFunc;

	template <typename F, size_t... IDX>
	static R Invoke(F f, T* thisPtr, lua_State* L, std::index_sequence<IDX...>)
	{
		// Member functions start with IDX = 2
		return (thisPtr->*f)(*runtime::GetValue<Args, false>(L, IDX + 2)...);
	}

	template<typename F>
	static BindFunc Get(F f)
	{
		static F func{ f };
		struct Impl : public esl::ScriptBind
		{
			int Func(lua_State* L)
			{
				T* thisPtr = static_cast<T*>(this->thisPtr);	// NOTE: unsafe, CallMemberFunc must check types first

				if constexpr (std::is_void<R>::value)
				{
					Invoke(func, thisPtr, L, std::index_sequence_for<Args...>{});
					return 0;
				}
				else
				{
					R ret = Invoke(func, thisPtr, L, std::index_sequence_for<Args...>{});
					runtime::PushValue(L, ret);
					return 1;
				}
			}
		};

		return static_cast<BindFunc>(&Impl::Func);
	}
};

template<typename T, typename R, typename... Args>
static auto BindMemberFunction(R(T::* func)(Args...))
{
	return MemberFunctionBinder<T, R, Args...>::Get(func);
}

template<auto FuncPtr>
struct FunctionBinder;

template<typename R, typename ... Args, R FuncPtr(Args...)>
struct FunctionBinder<FuncPtr>
{
	template<size_t... IDX>
	static R Invoke(lua_State* L, std::index_sequence<IDX...>)
	{
		return (*FuncPtr)(*runtime::GetValue<Args, false>(L, IDX + 1)...);
	}

	static int Func(lua_State* L)
	{
		if constexpr (std::is_void<R>::value)
		{
			Invoke(L, std::index_sequence_for<Args...>{});
			return 0;
		}
		else
		{
			R ret = Invoke(L, std::index_sequence_for<Args...>{});
			runtime::PushValue(L, ret);
			return 1;
		}
	}
};

template<auto FuncPtr>
static lua_CFunction BindFunction()
{
	return &FunctionBinder<FuncPtr>::Func;
}

// Variable binder. Generates appropriate getters/setters
template<typename T, auto MemberVar>
struct VariableBinder;

template<typename T, typename V, V T::* MemberVar>
struct VariableBinder<T, MemberVar> : public esl::ScriptBind
{
	int GetterFunc(lua_State* L)
	{
		T* thisPtr = static_cast<T*>(this->thisPtr);	// NOTE: unsafe, CallMemberFunc must check types first
		runtime::PushValue<V>(L, thisPtr->*MemberVar);
		return 1;
	}

	int SetterFunc(lua_State* L)
	{
		T* thisPtr = static_cast<T*>(this->thisPtr);	// NOTE: unsafe, CallMemberFunc must check types first

		// all non-trivial types should be treated as userdata
		if constexpr (std::is_trivial<V>::value)
			thisPtr->*MemberVar = *runtime::GetValue<V, true>(L, 1);
		else
			thisPtr->*MemberVar = *runtime::GetValue<V&, true>(L, 1);
		return 0;
	}
};

template<typename T, auto V>
static auto BindVariableGetter()
{
	return static_cast<typename esl::ScriptBind::BindFunc>(&VariableBinder<T, V>::GetterFunc);
}

template<typename T, auto V>
static auto BindVariableSetter()
{
	return static_cast<typename esl::ScriptBind::BindFunc>(&VariableBinder<T, V>::SetterFunc);
}

template<typename T, EOpType OpType>
struct StandardOperatorBinder
{
	static int OpFunc(lua_State* L)
	{
		T result;
		const T& lhs = *runtime::GetValue<T&, false>(L, 1);
		if constexpr (OpType == OP_unm)
		{
			result = -lhs;
			return 1;
		}
		else if constexpr (OpType == OP_not)
		{
			result = !lhs;
			return 1;
		}
		else
		{
			const T& rhs = *runtime::GetValue<T&, false>(L, 2);
			if constexpr (OpType == OP_add)
				result = lhs + rhs;
			else if constexpr (OpType == OP_sub)
				result = lhs - rhs;
			else if constexpr (OpType == OP_mul)
				result = lhs * rhs;
			else if constexpr (OpType == OP_div)
				result = lhs / rhs;
			else if constexpr (OpType == OP_mod)
				result = lhs % rhs;
			else if constexpr (OpType == OP_band)
				result = lhs & rhs;
			else if constexpr (OpType == OP_bor)
				result = lhs | rhs;
			else if constexpr (OpType == OP_xor)
				result = lhs ^ rhs;
			else if constexpr (OpType == OP_shl)
				result = lhs << rhs;
			else if constexpr (OpType == OP_shr)
				result = lhs >> rhs;
			else if constexpr (OpType == OP_eq)
				result = lhs == rhs;
			else if constexpr (OpType == OP_lt)
				result = lhs < rhs;
			else if constexpr (OpType == OP_le)
				result = lhs <= rhs;
			else
				static_assert(sizeof(T) > 0, "Unsupported operator type");
		}

		if constexpr (LuaTypeByVal<T>::value)
			runtime::PushGet<T>::PushObject(L, result, true, false);
		else
			runtime::PushGet<T>::PushObject(L, *(PPNew T(result)), true, false);
		return 1;
	}
};

template<typename T, EOpType OpType>
static auto BindOperator()
{
	return &StandardOperatorBinder<T, OpType>::OpFunc;
}

}

// Bindings generator
namespace esl::bindings
{
template <typename V>
struct IsConst : std::false_type {};

template <typename R, typename C, typename... Args>
struct IsConst<R(C::*)(Args...) const> : std::true_type {};

template<typename T>
Member ClassBinder<T>::MakeDestructor()
{
	Member m;
	m.type = MEMB_DTOR;
	m.name = "__gc";
	m.staticFunc = binder::BindDestructor<T>();
	return m;
}

template<typename T, typename R, typename... Args>
static auto GetFuncArgsSignature(R(T::* func)(Args...))
{
	return runtime::ArgsSignature<Args...>::Get();
}

template<typename T, typename R, typename... Args>
static int GetFuncArgsCount(R(T::* func)(Args...))
{
	return sizeof...(Args);
}

template<typename T, auto MemberVar>
struct MemberVarTypeName;

template<typename T, typename V, V T::* MemberVar>
struct MemberVarTypeName<T, MemberVar>
{
	static const char* Get() { return LuaBaseTypeAlias<T>::value; }
};

template<typename T, auto V>
static const char* GetVariableTypeName()
{
	return MemberVarTypeName<T, V>::Get();
}

template<typename T>
template<typename R, typename... Args>
Member ClassBinder<T>::MakeFunction(R(T::* func)(Args...), const char* name)
{
	Member m;
	m.type = MEMB_FUNC;
	m.nameHash = StringToHash(name);
	m.name = name;
	m.signature = GetFuncArgsSignature(func);
	m.numArgs = GetFuncArgsCount(func);
	m.func = binder::BindMemberFunction(func);
	m.isConst = IsConst<decltype(func)>::value;
	return m;
}

template<typename T>
template<auto V>
Member ClassBinder<T>::MakeVariable(const char* name)
{
	Member m;
	m.type = MEMB_VAR;
	m.nameHash = StringToHash(name);
	m.name = name;
	m.signature = GetVariableTypeName<T, V>();
	m.func = binder::BindVariableSetter<T, V>();
	m.getFunc = binder::BindVariableGetter<T, V>();
	return m;
}

template<typename T>
template<typename ...Args>
Member ClassBinder<T>::MakeConstructor()
{
	Member m;
	m.type = MEMB_CTOR;
	m.name = "constructor";
	m.signature = runtime::ArgsSignature<Args...>::Get();
	m.numArgs = sizeof...(Args);
	m.staticFunc = &binder::ConstructorBinder<T, Args...>::Func;
	return m;
}

template<typename T>
template<binder::EOpType OpType>
Member ClassBinder<T>::MakeOperator(const char* name)
{
	Member m;
	m.type = MEMB_OPERATOR;
	m.nameHash = StringToHash(name);
	m.name = name;
	m.staticFunc = binder::BindOperator<T, OpType>();
	return m;
}

template<typename T>
void BaseClassStorage::Add()
{
	const int nameHash = StringToHash(EqScriptClass<T>::className);
	if (!EqScriptClass<T>::baseClassName)
		return;
	baseClassNames.insert(nameHash, EqScriptClass<T>::baseClassName);
}

template<typename T>
const char* BaseClassStorage::Get()
{
	const int nameHash = StringToHash(EqScriptClass<T>::className);
	auto it = baseClassNames.find(nameHash);
	if (it.atEnd())
		return nullptr;

	return *it;
}
}