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

#pragma warning(push)
#pragma warning(disable:4267)

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
struct PushGetImpl
{
	static void PushObject(lua_State* L, const T& obj, int flags)
	{
		using UT = StripTraitsT<T>;
		using BaseUType = BaseType<UT>;
		
		ASSERT_MSG(LuaTypeByVal<BaseUType>::value == EqScriptClass<BaseUType>::isByVal, "Incorrect object push type on PUSH");

		if constexpr (LuaTypeByVal<BaseUType>::value)
		{
			BaseUType* ud = static_cast<BaseUType*>(lua_newuserdata(L, sizeof(BaseUType)));
			new(ud) BaseUType(obj); // FIXME: use move?
		}
		else
		{
			BoxUD* ud = static_cast<BoxUD*>(lua_newuserdata(L, sizeof(BoxUD)));
			ud->objPtr = const_cast<void*>(reinterpret_cast<const void*>(&obj));
			ud->type = UD_TYPE_PTR;
			ud->flags = flags;
		}
		luaL_setmetatable(L, LuaBaseTypeAlias<BaseUType>::value);
	}

	static T* GetObject(lua_State* L, int index, bool toCpp)
	{
		using UT = StripTraitsT<T>;
		using BaseUType = BaseType<UT>;

		static_assert(std::is_trivial_v<BaseUType> == false, "GetObject for trivial type");

		ASSERT_MSG(LuaTypeByVal<BaseUType>::value == EqScriptClass<BaseUType>::isByVal, "Incorrect object push type on GET");

		if constexpr (LuaTypeByVal<BaseUType>::value)
		{
			BaseUType* objPtr = static_cast<UT*>(lua_touserdata(L, index));
			return objPtr;
		}
		else
		{
			BoxUD* userData = static_cast<BoxUD*>(lua_touserdata(L, index));
			if (!userData)
				return static_cast<BaseUType*>(nullptr);

			// drop ownership flag when ToCpp is specified
			if(toCpp)
				userData->flags &= ~UD_FLAG_OWNED;

			return static_cast<BaseUType*>(userData ? userData->objPtr : nullptr);
		}
	}

	static int ObjectDestructor(lua_State* L)
	{
		using UT = StripTraitsT<T>;
		using BaseUType = BaseType<UT>;

		// destructor is safe to use statically-compiled ByVal
		if constexpr (LuaTypeByVal<T>::value)
		{
			ESL_VERBOSE_LOG("destruct val %s", LuaBaseTypeAlias<BaseUType>::value);
			T* userData = static_cast<T*>(lua_touserdata(L, 1));
			userData->~T();
		}
		else
		{
			BoxUD* userData = static_cast<BoxUD*>(lua_touserdata(L, 1));
			if (userData->flags & UD_FLAG_OWNED)
			{
				ESL_VERBOSE_LOG("destruct owned ref %s", LuaBaseTypeAlias<BaseUType>::value);
				delete static_cast<T*>(userData->objPtr);
				userData->flags &= ~UD_FLAG_OWNED;
				userData->objPtr = nullptr;
			}
		}
		return 0;
	}
};

template<typename T, typename WT>
static void PushValue(lua_State* L, const T& value)
{
	if constexpr (std::is_same_v<T, bool>)
	{
		lua_pushboolean(L, value);
	}
	else if constexpr (
		   std::is_same_v<T, long>
		|| std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, int8>
		|| std::is_same_v<T, uint8>
		|| std::is_same_v<T, int16>
		|| std::is_same_v<T, uint16>
		|| std::is_same_v<T, int32>
		|| std::is_same_v<T, uint32>
		|| std::is_same_v<T, int64>
		|| std::is_same_v<T, uint64>
		|| std::is_enum_v<T>)
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
	else if constexpr(std::is_same_v<BasePtrType<T>, lua_CFunction>)
	{
		lua_pushcfunction(L, value);
	}
	else if constexpr (std::is_same_v<BasePtrType<T>, bindings::LuaCFunction>)
	{
		lua_pushlightuserdata(L, value.funcPtr);
		lua_pushcclosure(L, value.luaFuncImpl, 1);
	}
	else if constexpr (
		   std::is_same_v<T, LuaRawRef>
		|| std::is_same_v<T, LuaFunctionRef>
		|| std::is_same_v<T, LuaTableRef>
		|| std::is_same_v<T, LuaTable>)
	{
		if (value)
			value.Push();
		else
			lua_pushnil(L);
	}
	else if constexpr (IsUserObj<T>::value)
	{
		const int retTraitFlag = HasToLuaReturnTrait<WT>::value ? UD_FLAG_OWNED : 0;
		if constexpr (std::is_pointer_v<T>)
		{
			if (value != nullptr)
				PushGet<BaseType<T>>::Push(L, *value, (std::is_const_v<T> ? UD_FLAG_CONST : 0) | retTraitFlag);
			else
				lua_pushnil(L);
		}
		else
			PushGet<BaseType<T>>::Push(L, value, (std::is_const_v<T> ? UD_FLAG_CONST : 0) | retTraitFlag);
	}
	else if constexpr(std::is_same_v<T, std::nullptr_t>)
	{
		lua_pushnil(L);
	}
	else
	{
		using UT = StripTraitsT<T>;
		using BaseUType = BaseType<UT>;
		static_assert(HasToLuaReturnTrait<WT>::value == false, "ToLua trait makes no sense when type is pushed as BY_VALUE");

		BaseUType pushObj(value);
		PushGet<BaseUType>::Push(L, pushObj, UD_FLAG_OWNED);
	}
}

// Lua type getters
template<typename T, bool SilentTypeCheck>
static decltype(auto) GetValue(lua_State* L, int index)
{
	using UT = std::remove_const_t<StripTraitsT<T>>;
	using Result = ResultWithValue<UT>;

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
		{
			luaL_checktype(L, index, type);
		}
		return true;
	};

	if constexpr (std::is_same_v<T, bool>) 
	{
		if(!checkType(L, index, LUA_TBOOLEAN))
			return Result{ false, {}};

		return Result{ true, {}, lua_toboolean(L, index) != 0 };
    }
	else if constexpr (
		   std::is_same_v<T, long>
		|| std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, int8>
		|| std::is_same_v<T, uint8>
		|| std::is_same_v<T, int16>
		|| std::is_same_v<T, uint16>
		|| std::is_same_v<T, int32>
		|| std::is_same_v<T, uint32>
		|| std::is_same_v<T, int64>
		|| std::is_same_v<T, uint64>
		|| std::is_enum_v<T>)
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
	else if constexpr (std::is_same_v<BaseType<T>, LuaRawRef>)
	{
		const int type = lua_type(L, index);
		return ResultWithValue<BaseType<T>>{ true, {}, LuaRawRef(L, index, type) };
	}
	else if constexpr (std::is_same_v<BaseType<T>, LuaFunctionRef>)
	{
		if (!checkType(L, index, LUA_TFUNCTION))
			return ResultWithValue<BaseType<T>>{ false, {}, BaseType<T>(L) };
		return ResultWithValue<BaseType<T>>{ true, {}, BaseType<T>(L, index) };
	}
	else if constexpr (
		   std::is_same_v<BaseType<T>, LuaTableRef>
		|| std::is_same_v<BaseType<T>, LuaTable>)
	{
		if (!checkType(L, index, LUA_TTABLE))
			return ResultWithValue<BaseType<T>>{ false, {}, BaseType<T>(L) };
		return ResultWithValue<BaseType<T>>{ true, {}, BaseType<T>(L, index) };
	}
	else if constexpr (IsString<T>::value)
	{
		if (!checkType(L, index, LUA_TSTRING))
		{
			if constexpr (IsEqString<T>::value)
				return ResultWithValue<EqString>{ false, {}, EqString() };
			else
				return Result{ false, {}, nullptr };
		}

		size_t len = 0;
		const char* value = lua_tolstring(L, index, &len);
		if constexpr (IsEqString<T>::value)
		{
			static_assert(!std::is_pointer_v<T>, "passing EqString by pointer is not supported yet");
			return ResultWithValue<EqString>{ true, {}, EqString(value, len) };
		}
		else
			return Result{ true, {}, value };
	}
	else if constexpr (IsUserObj<UT>::value)
	{
		const bool toCpp = HasToCppParamTrait<T>::value;

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

		BaseType<UT>* objPtr = static_cast<BaseType<UT>*>(PushGet<BaseType<UT>>::Get(L, index, toCpp));

		if constexpr (std::is_reference_v<UT>)
			return Result{ true, {}, reinterpret_cast<UT>(*objPtr) };
		else 
			return Result{ true, {}, reinterpret_cast<UT>(objPtr) };
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
static decltype(auto) GetGlobal(lua_State* L, const char* fieldName)
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
			return Result{ false, "is not callable"};

		lua_State* L = func.GetState();

		lua_pushcfunction(L, StackTrace);
		const int errIdx = lua_gettop(L);

		func.Push();
		PushArguments(L, std::forward<Args>(args)...);

		return InvokeFunc(L, sizeof...(Args), errIdx);
	}

	static Result Invoke(lua_State* L, int funcIndex, Args... args)
	{
		if (lua_type(L, funcIndex) != LUA_TFUNCTION)
			return Result{ false, "is not callable"};

		lua_pushcfunction(L, StackTrace);
		const int errIdx = lua_gettop(L);

		lua_pushvalue(L, funcIndex);
		PushArguments(L, std::forward<Args>(args)...);

		return InvokeFunc(L, sizeof...(Args), errIdx);
	}

private:
	static void PushArguments(lua_State* L) {}

	template<typename First, typename... Rest>
	static void PushArguments(lua_State* L, First first, Rest... rest)
	{
		PushValue(L, first);
		PushArguments(L, rest...);
	}

	static Result InvokeFunc(lua_State* L, int numArgs, int errIdx)
	{
		const int res = lua_pcall(L, numArgs, std::is_void_v<R> ? LUA_MULTRET : 1, errIdx);
		if (res == 0)
		{
			if constexpr (std::is_void_v<R>)
				return Result{ true };
			else
				return Result{ true, {}, *runtime::GetValue<R, false>(L, -1) };
		}

		const char* errorMessage = nullptr;
		if (res != LUA_ERRMEM)
		{
			SetLuaErrorFromTopOfStack(L);
			errorMessage = GetLastError(L);
		}

		return Result{ false, errorMessage };
	}
};

}

//-----------------------------------------------
// FUNCTION BINDER
namespace esl::binder
{
// Traits signature wrapper to simplify templates
template<typename UR, typename ... UArgs>
struct FuncTraits
{
	using TR = UR;
	using TArgs = std::tuple<UArgs...>;
};

template <typename Func>
struct TransformSignature;

// For member functions
template <typename R, typename C, typename... Args>
struct TransformSignature<R(C::*)(Args...)> 
{
    using type = R(C::*)(StripTraitsT<Args>...);
};

// Add const qualifier variant if needed
template <typename R, typename C, typename... Args>
struct TransformSignature<R(C::*)(Args...) const> 
{
    using type = R(C::*)(StripTraitsT<Args>...) const;
};

template <typename Func>
using TransformSignatureT = typename TransformSignature<Func>::type;

//---------------------------------------------------------------
// Constructor binder

template<typename T, typename... Args>
struct ConstructorBinder;

// Default constructor binder
template<typename T>
struct ConstructorBinder<T>
{
	static int Func(lua_State* L)
	{
		T* newObj = PPNew T();
		runtime::PushGet<T>::Push(L, *newObj, UD_FLAG_OWNED);
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
		ESL_VERBOSE_LOG("create %s, byval %d", EqScriptClass<T>::className, EqScriptClass<T>::isByVal);

		// Extract arguments from Lua and forward them to the constructor
		using UT = StripTraitsT<T>;
		using BaseUType = BaseType<UT>;

		if constexpr (LuaTypeByVal<BaseUType>::value)
		{
			T newObjVal(*runtime::GetValue<Args, true>(L, IDX + 1)...);
			runtime::PushGet<T>::Push(L, newObjVal, UD_FLAG_OWNED);
		}
		else
		{
			T* newObj = PPNew T(*runtime::GetValue<Args, true>(L, IDX + 1)...);
			runtime::PushGet<T>::Push(L, *newObj, UD_FLAG_OWNED);
		}
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
	return &runtime::PushGetImpl<T>::ObjectDestructor;
}

//---------------------------------------------------------------
// Member function binder

template<typename T, auto FuncPtr, typename Traits>
struct MemberFunctionBinder;

// Non-const binder
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...), typename Traits>
struct MemberFunctionBinder<T, FuncPtr, Traits> : public esl::ScriptBind
{
	using UR = typename Traits::TR;
	using UArgs = typename Traits::TArgs;

	template <size_t... IDX>
	static R Invoke(T* thisPtr, lua_State* L, std::index_sequence<IDX...>)
	{
		// Member functions start with IDX = 2
		return (thisPtr->*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, UArgs>, false>(L, IDX + 2)...);
	}

	int Func(lua_State* L)
	{
		ESL_VERBOSE_LOG("call member %s:%x", EqScriptClass<T>::className, FuncPtr);

		T* thisPtr = static_cast<T*>(this->thisPtr); // NOTE: unsafe, CallMemberFunc must check types first
		if constexpr (std::is_void_v<R>)
		{
			Invoke(thisPtr, L, std::index_sequence_for<Args...>{});
			return 0;
		}
		else
		{
			R ret = Invoke(thisPtr, L, std::index_sequence_for<Args...>{});
			runtime::PushValue<R, UR>(L, ret);
			return 1;
		}
	}

	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

// Const binder, only has const below
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const, typename Traits>
struct MemberFunctionBinder<T, FuncPtr, Traits> : public esl::ScriptBind
{
	using UR = typename Traits::TR;
	using UArgs = typename Traits::TArgs;

	template <size_t... IDX>
	static R Invoke(T* thisPtr, lua_State* L, std::index_sequence<IDX...>)
	{
		// Member functions start with IDX = 2
		return (thisPtr->*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, UArgs>, false>(L, IDX + 2)...);
	}

	int Func(lua_State* L) const
	{
		ESL_VERBOSE_LOG("call member %s:%x const", EqScriptClass<T>::className, FuncPtr);

		T* thisPtr = static_cast<T*>(this->thisPtr); // NOTE: unsafe, CallMemberFunc must check types first
		if constexpr (std::is_void_v<R>)
		{
			Invoke(thisPtr, L, std::index_sequence_for<Args...>{});
			return 0;
		}
		else
		{
			R ret = Invoke(thisPtr, L, std::index_sequence_for<Args...>{});
			runtime::PushValue<R, UR>(L, ret);
			return 1;
		}
	}

	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

template<typename T, auto FuncPtr>
struct MemberFunctionBinderNoTraits;

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...)>
struct MemberFunctionBinderNoTraits<T, FuncPtr> : public MemberFunctionBinder<T, FuncPtr, FuncTraits<R, Args...>> {};

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const>
struct MemberFunctionBinderNoTraits<T, FuncPtr> : public MemberFunctionBinder<T, FuncPtr, FuncTraits<R, Args...>> {};

template<typename T, auto FuncPtr, typename Traits>
static auto BindMemberFunction() 
{ 
	using BindFuncConst = esl::ScriptBind::BindFuncConst;
	using BindFunc = esl::ScriptBind::BindFunc;
	using UR = typename Traits::TR;
	using UArgs = typename Traits::TArgs;

	if constexpr (std::is_void_v<UR> && std::tuple_size_v<UArgs> == 0)
	{
		if constexpr (IsConstMemberFunc<decltype(FuncPtr)>::value)
			return static_cast<BindFuncConst>(&MemberFunctionBinderNoTraits<T, FuncPtr>::Func);
		else
			return static_cast<BindFunc>(&MemberFunctionBinderNoTraits<T, FuncPtr>::Func);
	}
	else
	{
		if constexpr (IsConstMemberFunc<decltype(FuncPtr)>::value)
			return static_cast<BindFuncConst>(&MemberFunctionBinder<T, FuncPtr, Traits>::Func);
		else
			return static_cast<BindFunc>(&MemberFunctionBinder<T, FuncPtr, Traits>::Func);
	}
}

//---------------------------------------------------------------
// C (non-member) Function binder

template<typename Func, typename Traits>
struct FunctionBinder;

template<typename R, typename ... Args, typename Traits>
struct FunctionBinder<R(*)(Args...), Traits>
{
	using FuncType = R(*)(Args...);
	using UR = typename Traits::TR;
	using UArgs = typename Traits::TArgs;

	template<size_t... IDX>
	static R Invoke(lua_State* L, std::index_sequence<IDX...>)
	{
		const FuncType FuncPtr = reinterpret_cast<FuncType>(lua_touserdata(L, lua_upvalueindex(1)));
		return (*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, UArgs>, false>(L, IDX + 1)...);
	}

	static int Func(lua_State* L)
	{
		if constexpr (std::is_void_v<R>)
		{
			Invoke(L, std::index_sequence_for<Args...>{});
			return 0;
		}
		else
		{
			R ret = Invoke(L, std::index_sequence_for<Args...>{});
			runtime::PushValue<R, UR>(L, ret);
			return 1;
		}
	}

	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

template<typename Func>
struct FunctionBinderNoTraits;

template<typename R, typename ... Args>
struct FunctionBinderNoTraits<R(*)(Args...)> : public FunctionBinder<R(*)(Args...), FuncTraits<R, Args...>> {};

template<typename UR = void, typename ... UArgs, typename Func>
static bindings::LuaCFunction BindCFunction(Func f)
{
	bindings::LuaCFunction funcInfo;
	funcInfo.funcPtr = reinterpret_cast<void*>(f);

	if constexpr (std::is_void_v<UR> && sizeof...(UArgs) == 0)
		funcInfo.luaFuncImpl = &FunctionBinderNoTraits<Func>::Func;
	else
		funcInfo.luaFuncImpl = &FunctionBinder<Func, FuncTraits<UR, UArgs...>>::Func;
	return funcInfo;
}

//---------------------------------------------------------------
// Variable binder. Generates appropriate getters/setters

template<typename T, auto MemberVar>
struct VariableBinder;

template<typename T, typename V, V T::* MemberVar>
struct VariableBinder<T, MemberVar> : public esl::ScriptBind
{
	int GetterFunc(lua_State* L) const
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
	return static_cast<typename esl::ScriptBind::BindFuncConst>(&VariableBinder<T, V>::GetterFunc);
}

template<typename T, auto V>
static auto BindVariableSetter()
{
	return static_cast<typename esl::ScriptBind::BindFunc>(&VariableBinder<T, V>::SetterFunc);
}

//---------------------------------------------------------------
// Default operator binder

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
		}
		else if constexpr (OpType == OP_not)
		{
			result = !lhs;
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
			runtime::PushGet<T>::Push(L, result, UD_FLAG_OWNED);
		else
			runtime::PushGet<T>::Push(L, *(PPNew T(result)), UD_FLAG_OWNED);
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
template<typename T>
Member ClassBinder<T>::MakeDestructor()
{
	Member m;
	m.type = MEMB_DTOR;
	m.name = "__gc";
	m.staticFunc = binder::BindDestructor<T>();
	return m;
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
template<typename UR, typename ... UArgs, typename F>
Member ClassBinder<T>::MakeStaticFunction(F func, const char* name)
{
	using Traits = binder::FuncTraits<UR, UArgs...>;

	LuaCFunction funcInfo = binder::BindCFunction<UR, UArgs...>(func);

	Member m;
	m.type = MEMB_C_FUNC;
	m.name = name;
	m.data = &func;
	m.signature = binder::FunctionBinder<F, Traits>::GetFuncArgsSignature();
	m.numArgs = binder::FunctionBinder<F, Traits>::GetFuncArgsCount();
	m.staticFunc = funcInfo.luaFuncImpl;
	m.data = funcInfo.funcPtr;
	m.isConst = false;
	return m;
}

template<typename T>
template<auto F, typename UR, typename ... UArgs>
Member ClassBinder<T>::MakeFunction(const char* name)
{
	using Traits = binder::FuncTraits<UR, UArgs...>;

	Member m;
	m.type = MEMB_FUNC;
	m.name = name;
	m.signature = binder::MemberFunctionBinder<T, F, Traits>::GetFuncArgsSignature();
	m.numArgs = binder::MemberFunctionBinder<T, F, Traits>::GetFuncArgsCount();
	if constexpr (IsConstMemberFunc<decltype(F)>::value)
		m.constFunc = binder::BindMemberFunction<T, F, Traits>();
	else
		m.func = binder::BindMemberFunction<T, F, Traits>();
	m.isConst = false;
	return m;
}

template<typename T>
template<auto V>
Member ClassBinder<T>::MakeVariable(const char* name)
{
	Member m;
	m.type = MEMB_VAR;
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
	m.name = name;
	m.staticFunc = binder::BindOperator<T, OpType>();
	return m;
}

template<typename T>
template<typename F>
Member ClassBinder<T>::MakeOperator(F f, const char* name)
{
	Member m;
	m.type = MEMB_OPERATOR;
	m.name = name;
	m.staticFunc = f;
	return m;
}

template<typename T>
void BaseClassStorage::Add()
{
	const int nameHash = StringToHash(EqScriptClass<T>::className);
	if (!EqScriptClass<T>::baseClassName)
		return;
	GetBaseClassNames().insert(nameHash, EqScriptClass<T>::baseClassName);
}

template<typename T>
const char* BaseClassStorage::Get()
{
	return Get(EqScriptClass<T>::className);
}
}

#pragma warning(pop)