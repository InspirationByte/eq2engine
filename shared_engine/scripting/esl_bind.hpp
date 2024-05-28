#pragma once

namespace esl
{

enum EUserDataFlags : int
{
	UD_FLAG_CONST	= (1 << 0),	// a 'const' qualified object
	UD_FLAG_OWNED	= (1 << 1),	// was created by Lua and should be destroyed
};

// boxed userdata
struct BoxUD
{
	void*	objPtr{ nullptr };
	uint	flags{ 0 };
};
}

#pragma warning(push)
#pragma warning(disable:4267)

//-----------------------------------------------
// TYPE BINDER
namespace esl::binder
{
// Traits to detect which types are pullable from Lua
template<typename T> struct IsUserObj : std::false_type {};
template<typename T> struct IsUserObj<T*> : std::true_type {};
template<typename T> struct IsUserObj<T&> : std::true_type {};
template<typename T> struct IsUserObj<CRefPtr<T>> : std::true_type {};
template<typename T> struct IsUserObj<CWeakPtr<T>> : std::true_type {};

template<typename T> struct IsRefPtr : std::false_type {};
template<typename T> struct IsRefPtr<CRefPtr<T>> : std::true_type {};
template<typename T> struct IsRefPtr<CRefPtr<T>&> : std::true_type {};
template<typename T> struct IsRefPtr<const CRefPtr<T>&> : std::true_type {};

template<typename T> struct IsWeakPtr : std::false_type {};
template<typename T> struct IsWeakPtr<CWeakPtr<T>> : std::true_type {};
template<typename T> struct IsWeakPtr<CWeakPtr<T>&> : std::true_type {};
template<typename T> struct IsWeakPtr<const CWeakPtr<T>&> : std::true_type {};

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

} // end esl::binder

namespace esl::runtime
{
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
			if constexpr (std::is_same_v<First, EqScriptState>)
				return EqString(*restStr ? "," : "") + restStr;
			else
				return EqString(LuaBaseTypeAlias<First>::value) + (*restStr ? "," : "") + restStr;			
		}();
		return result;
	}
};

template<typename T, typename... Args>
T& New(lua_State* L, Args&&... args)
{
	if constexpr (LuaTypeByVal<T>::value)
	{
		T* ud = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
		new(ud) T{ std::forward<Args>(args)... };
		luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);

		return *ud;
	}
	else
	{
		T* newObj = PPNew T{ std::forward<Args>(args)... };

		BoxUD* ud = static_cast<BoxUD*>(lua_newuserdata(L, sizeof(BoxUD)));
		ud->objPtr = newObj;
		ud->flags = UD_FLAG_OWNED;

		if constexpr (LuaTypeRefCountedObj<T>::value)
			newObj->Ref_Grab();

		luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);
		return *newObj;
	}
}

// Push pull is essential when you want to send or get values from Lua
template<typename T>
struct PushGetImpl
{
	using UT = StripTraitsT<T>;
	using BaseUType = BaseType<UT>;

	// TODO: MoveObject()

	static void PushObject(lua_State* L, const T& obj, int flags)
	{
		static_assert(std::is_fundamental_v<BaseUType> == false, "PushObject used for fundamental type");

		if constexpr (LuaTypeByVal<BaseUType>::value)
		{
			BaseUType* ud = static_cast<BaseUType*>(lua_newuserdata(L, sizeof(BaseUType)));
			new(ud) BaseUType(obj); // FIXME: use move?
		}
		else
		{
			BoxUD* ud = static_cast<BoxUD*>(lua_newuserdata(L, sizeof(BoxUD)));
			ud->objPtr = const_cast<void*>(reinterpret_cast<const void*>(&obj));
			ud->flags = flags;

			if constexpr (LuaTypeRefCountedObj<BaseUType>::value)
				const_cast<BaseUType*>(&obj)->Ref_Grab();
		}
		luaL_setmetatable(L, LuaBaseTypeAlias<BaseUType>::value);
	}

	static T* GetObject(lua_State* L, int index, bool toCpp)
	{
		static_assert(std::is_fundamental_v<BaseUType> == false, "GetObject used for fundamental type");

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
			// so Lua can no longer delete object (C++ now has to)
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
			if constexpr (LuaTypeRefCountedObj<BaseUType>::value)
			{
				ESL_VERBOSE_LOG("deref obj %s", LuaBaseTypeAlias<BaseUType>::value);
				static_cast<T*>(userData->objPtr)->Ref_Drop();
			}
			else
			{
				if (userData->flags & UD_FLAG_OWNED)
				{
					ESL_VERBOSE_LOG("destruct owned obj %s", LuaBaseTypeAlias<BaseUType>::value);
					delete static_cast<T*>(userData->objPtr);
					userData->flags &= ~UD_FLAG_OWNED;
					userData->objPtr = nullptr;
				}
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
	else if constexpr (binder::IsString<T>::value)
	{
		lua_pushstring(L, value);
	}
	else if constexpr(std::is_same_v<BaseRefType<T>, lua_CFunction>)
	{
		lua_pushcfunction(L, value);
	}
	else if constexpr (std::is_same_v<BaseRefType<T>, bindings::LuaCFunction>)
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
	else if constexpr (binder::IsRefPtr<T>::value)
	{
		using UT = BaseType<StripRefPtrT<BaseType<T>>>;

		// really does not matter since deleter will still Ref_Drop() 
		// object but we'll keep it anyway
		const int retTraitFlag = HasToLuaReturnTrait<WT>::value ? UD_FLAG_OWNED : 0;
		if (value)
			PushGet<UT>::Push(L, value.Ref(), (std::is_const_v<UT> ? UD_FLAG_CONST : 0) | retTraitFlag);
		else
			lua_pushnil(L);
	}
	else if constexpr (binder::IsUserObj<T>::value)
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
		const int retTraitFlag = HasToLuaReturnTrait<WT>::value ? UD_FLAG_OWNED : 0;
		using BaseUType = StripRefPtrT<BaseType<StripTraitsT<T>>>;
		PushGet<BaseUType>::Push(L, value, (std::is_const_v<T> ? UD_FLAG_CONST : 0) | retTraitFlag);
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

	// TODO: Nullable trait instead of checks for table, function and ptr userdata
	const bool isArgNull = lua_type(L, index) == LUA_TNIL;

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
			return Result{ false, EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, lua_type(L, index)))};

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
			return Result{ false, EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, lua_type(L, index))) };

		return Result{ true, {}, static_cast<T>(lua_tointeger(L, index)) };
	}
	else if constexpr (
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>)
	{
		if (!checkType(L, index, LUA_TNUMBER))
			return Result{ false, EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, lua_type(L, index))) };

		return Result{ true, {}, static_cast<T>(lua_tonumber(L, index)) };
    } 
	else if constexpr (std::is_same_v<BaseType<T>, LuaRawRef>)
	{
		const int type = lua_type(L, index);
		return ResultWithValue<BaseType<T>>{ true, {}, LuaRawRef(L, index, type) };
	}
	else if constexpr (std::is_same_v<BaseType<T>, LuaFunctionRef>)
	{
		if (!isArgNull && !checkType(L, index, LUA_TFUNCTION))
			return ResultWithValue<BaseType<T>>{ false, {}, BaseType<T>(L) };

		return ResultWithValue<BaseType<T>>{ true, {}, BaseType<T>(L, index) };
	}
	else if constexpr (
		   std::is_same_v<BaseType<T>, LuaTableRef>
		|| std::is_same_v<BaseType<T>, LuaTable>)
	{
		if (!isArgNull && !checkType(L, index, LUA_TTABLE))
			return ResultWithValue<BaseType<T>>{ false, {}, BaseType<T>(L) };

		return ResultWithValue<BaseType<T>>{ true, {}, BaseType<T>(L, index) };

	}
	else if constexpr (binder::IsString<T>::value)
	{
		if (!checkType(L, index, LUA_TSTRING))
		{
			if constexpr (binder::IsEqString<T>::value)
				return ResultWithValue<EqString>{ false, {}, EqString() };
			else
				return Result{ false, EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, lua_type(L, index))), nullptr };
		}

		size_t len = 0;
		const char* value = lua_tolstring(L, index, &len);
		if constexpr (binder::IsEqString<T>::value)
		{
			static_assert(!std::is_pointer_v<T>, "passing EqString by pointer is not supported yet");
			return ResultWithValue<EqString>{ true, {}, EqString(value, len) };
		}
		else
			return Result{ true, {}, value };
	}
	else if constexpr (binder::IsUserObj<UT>::value)
	{
		const int type = lua_type(L, index);
		if constexpr (binder::IsRefPtr<UT>::value)
		{
			using RT = StripRefPtrT<BaseType<UT>>;
			using REFPTR = CRefPtr<RT>;

			if (type != LUA_TUSERDATA)
			{
				EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<UT>::value, lua_typename(L, type));
				if constexpr (!SilentTypeCheck)
				{
					if (!isArgNull)
						luaL_argerror(L, index, err);
				}

				return ResultWithValue<REFPTR>{ false, std::move(err), nullptr };
			}

			if (!CheckUserdataCanBeUpcasted(L, index, LuaBaseTypeAlias<RT>::value))
			{
				const int type = luaL_getmetafield(L, index, "__name");
				const char* className = lua_tostring(L, -1);
				lua_pop(L, 1);

				EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<RT>::value, className);
				if constexpr (!SilentTypeCheck)
					luaL_argerror(L, index, err);

				return ResultWithValue<REFPTR>{ false, std::move(err), nullptr };
			}

			static_assert(!HasToCppParamTrait<T>::value, "can't use ToCpp trait on CRefPtr");
			REFPTR objPtr(PushGet<RT>::Get(L, index, false));

			return ResultWithValue<REFPTR>{ true, {}, std::move(objPtr) };
		}
		else
		{
			if (type != LUA_TUSERDATA)
			{
				EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, type));
				if constexpr (!SilentTypeCheck)
				{
					if (isArgNull)
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

			const bool toCpp = HasToCppParamTrait<T>::value;
			BaseType<UT>* objPtr = static_cast<BaseType<UT>*>(PushGet<BaseType<UT>>::Get(L, index, toCpp));

			if constexpr (std::is_reference_v<UT>)
				return Result{ true, {}, reinterpret_cast<UT>(*objPtr) };
			else
				return Result{ true, {}, reinterpret_cast<UT>(objPtr) };
		}
	}
	else 
	{
		ASSERT_FAIL("Unsupported argument type %s", typeid(T).name());

		if constexpr (std::is_reference_v<T>)
			return Result{ false, "Unsupported argument type", reinterpret_cast<T>(*(BaseType<T>*)nullptr)};
		else
			return Result{ false, "Unsupported argument type", reinterpret_cast<T>(nullptr) };

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
			{
				return Result{ true };
			}
			else
			{
				auto value = runtime::GetValue<R, true>(L, -1);
				return Result{ value.success, value.success ? EqString() : EqString::Format("return value: %s", value.errorMessage.ToCString()), *value};
			}
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
template<typename R, typename ... Args>
struct FuncSignature
{
	using TR = R;
	using TArgs = std::tuple<Args...>;
};

using FuncSignatureDefault = FuncSignature<void>;

//---------------------------------------------------------------
// Constructor binder

template<typename T, typename... Args>
struct ConstructorBinder;

// Default constructor binder
template<typename T>
struct ConstructorBinder<T>
{
	using UT = StripTraitsT<T>;
	using BaseUType = BaseType<UT>;

	static int Func(lua_State* L)
	{
		ESL_VERBOSE_LOG("ctor(default) %s, byval %d", EqScriptClass<T>::className, LuaTypeByVal<T>::value);
		runtime::New<BaseUType>(L);
		return 1;
	}
};

// Binder specialization for custom constructors
template<typename T, typename... Args>
struct ConstructorBinder 
{
	using UT = StripTraitsT<T>;
	using BaseUType = BaseType<UT>;

	template<size_t... IDX>
	static void Invoke(lua_State* L, std::index_sequence<IDX...>)
	{
		ESL_VERBOSE_LOG("ctor(...) %s, byval %d", EqScriptClass<T>::className, LuaTypeByVal<T>::value);
		runtime::New<BaseUType>(L, *runtime::GetValue<Args, true>(L, IDX + 1)...);
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

template<typename ... Args>
struct CheckLuaStateArg : std::false_type {};

template<typename First, typename ... Rest>
struct CheckLuaStateArg<First, Rest...> : std::is_same<First, EqScriptState> {};

//---------------------------------------------------------------
// Member function binder

template<typename T, auto FuncPtr, typename Traits>
struct MemberFunctionBinder {};

template <typename T, auto FuncPtr, typename Traits, typename R, typename ... Args>
struct MemberFunction
{
	// first argument is Lua state?
	using HasLuaStateArg = CheckLuaStateArg<Args...>;

	template <size_t... IDX>
	static R Invoke(T* thisPtr, lua_State* L, std::index_sequence<IDX...>)
	{
		// NOTES: Member functions start with IDX = 2
		// this is now unsafe, CallMemberFunc must have been taken care for us
		if constexpr (HasLuaStateArg::value)
			return (thisPtr->*FuncPtr)(L, *runtime::GetValue<std::tuple_element_t<IDX+1, typename Traits::TArgs>, false>(L, IDX + 2)...);
		else
			return (thisPtr->*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, typename Traits::TArgs>, false>(L, IDX + 2)...);
	}

	static int FuncImpl(T* thisPtr, lua_State* L)
	{
		ESL_VERBOSE_LOG("call member %s:%x", EqScriptClass<T>::className, FuncPtr);
		if constexpr (std::is_void_v<R>)
		{
			Invoke(thisPtr, L, std::make_index_sequence<sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0)>{});
			return 0;
		}
		else
		{
			R ret = Invoke(thisPtr, L, std::make_index_sequence<sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0)>{});
			runtime::PushValue<R, typename Traits::TR>(L, ret);
			return 1;
		}
	}
};

// Non-const binder
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...), typename Traits>
struct MemberFunctionBinder<T, FuncPtr, Traits> : public esl::ScriptBind
{
	int Func(lua_State* L) { return MemberFunction<T, FuncPtr, Traits, R, Args...>::FuncImpl(static_cast<T*>(this->thisPtr), L); }
	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

// Const binder, only has const below
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const, typename Traits>
struct MemberFunctionBinder<T, FuncPtr, Traits> : public esl::ScriptBind
{
	int Func(lua_State* L) { return MemberFunction<T, FuncPtr, Traits, R, Args...>::FuncImpl(static_cast<T*>(this->thisPtr), L); }
	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

template<typename T, auto FuncPtr>
struct MemberFunctionBinderNoTraits;

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...)>
struct MemberFunctionBinderNoTraits<T, FuncPtr> : public MemberFunctionBinder<T, FuncPtr, FuncSignature<R, Args...>> {};

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const>
struct MemberFunctionBinderNoTraits<T, FuncPtr> : public MemberFunctionBinder<T, FuncPtr, FuncSignature<R, Args...>> {};

template<typename T, auto FuncPtr, typename Traits>
static auto BindMemberFunction() 
{ 
	using BindFunc = esl::ScriptBind::BindFunc;

	if constexpr (std::is_void_v<typename Traits::TR> && std::tuple_size_v<typename Traits::TArgs> == 0)
		return static_cast<BindFunc>(&MemberFunctionBinderNoTraits<T, FuncPtr>::Func);
	else
		return static_cast<BindFunc>(&MemberFunctionBinder<T, FuncPtr, Traits>::Func);
}

//---------------------------------------------------------------
// C (non-member) Function binder

template<typename Func, typename Traits>
struct FunctionBinder;

template<typename R, typename ... Args, typename Traits>
struct FunctionBinder<R(*)(Args...), Traits>
{
	// first argument is Lua state?
	using HasLuaStateArg = CheckLuaStateArg<Args...>;

	template<size_t... IDX>
	static R Invoke(lua_State* L, std::index_sequence<IDX...>)
	{
		const auto FuncPtr = reinterpret_cast<R(*)(Args...)>(lua_touserdata(L, lua_upvalueindex(1)));
		if constexpr(HasLuaStateArg::value)
			return (*FuncPtr)(L, *runtime::GetValue<std::tuple_element_t<IDX+1, typename Traits::TArgs>, false>(L, IDX + 1)...);
		else
			return (*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, typename Traits::TArgs>, false>(L, IDX + 1)...);
	}

	static int Func(lua_State* L)
	{
		if constexpr (std::is_void_v<R>)
		{
			Invoke(L, std::make_index_sequence<sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0)>{});
			return 0;
		}
		else
		{
			R ret = Invoke(L, std::make_index_sequence<sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0)>{});
			runtime::PushValue<R, typename Traits::TR>(L, ret);
			return 1;
		}
	}

	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
};

template<typename Func>
struct FunctionBinderNoTraits;

template<typename R, typename ... Args>
struct FunctionBinderNoTraits<R(*)(Args...)> : public FunctionBinder<R(*)(Args...), FuncSignature<R, Args...>> {};

template<typename UR = void, typename ... UArgs, typename Func>
static bindings::LuaCFunction BindCFunction(Func f)
{
	bindings::LuaCFunction funcInfo;
	funcInfo.funcPtr = f;

	if constexpr (std::is_void_v<UR> && sizeof...(UArgs) == 0)
		funcInfo.luaFuncImpl = &FunctionBinderNoTraits<Func>::Func;
	else
		funcInfo.luaFuncImpl = &FunctionBinder<Func, FuncSignature<UR, UArgs...>>::Func;
	return funcInfo;
}

//---------------------------------------------------------------
// Variable binder. Generates appropriate getters/setters

template<typename T, auto MemberVar>
struct VariableBinder;

template<typename T, typename V, V T::* MemberVar>
struct VariableBinder<T, MemberVar> : public esl::ScriptBind
{
	int GetterFunc(lua_State* L)
	{
		T* thisPtr = static_cast<T*>(this->thisPtr);
		runtime::PushValue<V>(L, thisPtr->*MemberVar);
		return 1;
	}

	int SetterFunc(lua_State* L)
	{
		T* thisPtr = static_cast<T*>(this->thisPtr);

		// enums and fundamental types should be by-value
		if constexpr (std::is_fundamental<V>::value || std::is_enum<V>::value)
			thisPtr->*MemberVar = *runtime::GetValue<V, true>(L, 2);
		else
			thisPtr->*MemberVar = *runtime::GetValue<V&, true>(L, 2);
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
			runtime::New<T>(L,-lhs);
		}
		else if constexpr (OpType == OP_not)
		{
			lua_pushboolean(!lhs);
		}
		else
		{
			const T& rhs = *runtime::GetValue<T&, false>(L, 2);
			if constexpr (OpType == OP_add)
				runtime::New<T>(L, lhs + rhs);
			else if constexpr (OpType == OP_sub)
				runtime::New<T>(L, lhs - rhs);
			else if constexpr (OpType == OP_mul)
				runtime::New<T>(L, lhs * rhs);
			else if constexpr (OpType == OP_div)
				runtime::New<T>(L, lhs / rhs);
			else if constexpr (OpType == OP_mod)
				runtime::New<T>(L, lhs % rhs);
			else if constexpr (OpType == OP_band)
				runtime::New<T>(L, lhs & rhs);
			else if constexpr (OpType == OP_bor)
				runtime::New<T>(L, lhs | rhs);
			else if constexpr (OpType == OP_xor)
				runtime::New<T>(L, lhs ^ rhs);
			else if constexpr (OpType == OP_shl)
				runtime::New<T>(L, lhs << rhs);
			else if constexpr (OpType == OP_shr)
				runtime::New<T>(L, lhs >> rhs);
			else if constexpr (OpType == OP_eq)
				lua_pushboolean(lhs == rhs);
			else if constexpr (OpType == OP_lt)
				lua_pushboolean(lhs < rhs);
			else if constexpr (OpType == OP_le)
				lua_pushboolean(lhs <= rhs);
			else
				static_assert(sizeof(T) > 0, "Unsupported operator type");
		}
		return 1;
	}
};

template<typename T, EOpType OpType>
static auto BindOperator()
{
	return &StandardOperatorBinder<T, OpType>::OpFunc;
}

template<typename T, void (*Func)(const T&, char*, const int)>
int ToStringOperator(lua_State* L)
{
	const int opType = lua_type(L, 1);
	if (opType == LUA_TNIL)
	{
		lua_pushstring(L, "(null)");
		return 1;
	}

	const T& val = *esl::runtime::GetValue<T&, false>(L, 1);

	// FIXME: consider dynamic allocation
	char tmpStr[256];
	Func(val, tmpStr, elementsOf(tmpStr));

	lua_pushstring(L, tmpStr);
	return 1;
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
	static const char* Get() { return LuaBaseTypeAlias<V>::value; }
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
	using Traits = binder::FuncSignature<UR, UArgs...>;

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
	using Traits = binder::FuncSignature<UR, UArgs...>;

	Member m;
	m.type = MEMB_FUNC;
	m.name = name;
	m.signature = binder::MemberFunctionBinder<T, F, Traits>::GetFuncArgsSignature();
	m.numArgs = binder::MemberFunctionBinder<T, F, Traits>::GetFuncArgsCount();
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
template<auto V, auto F>
Member ClassBinder<T>::MakeVariableWithSetter(const char* name)
{
	Member m;
	m.type = MEMB_VAR;
	m.name = name;
	m.signature = GetVariableTypeName<T, V>();
	m.func = binder::BindMemberFunction<T, F, binder::FuncSignatureDefault>();
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