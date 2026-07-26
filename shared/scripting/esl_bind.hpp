#pragma once

namespace esl
{

template<typename T>
LuaUserRef Object<T>::ToRef() const
{
	return LuaUserRef(m_state, m_index);
}

template<typename T>
T& Object<T>::Get() const
{
	return *esl::runtime::GetValue<T&, true>(m_state, m_index);
}

template<typename T>
T* Object<T>::GetPtr() const
{
	return *esl::runtime::GetValue<T*, true>(m_state, m_index);
}

}

#pragma warning(push)
#pragma warning(disable:4267)

//-----------------------------------------------
// TYPE BINDER
namespace esl::binder
{
// Traits to detect raw object on stack
template<typename T> struct IsObject : std::false_type {};
template<typename T> struct IsObject<Object<T>> : std::true_type {};
template<typename T> struct IsObject<Object<T>&> : std::true_type {};
template<typename T> struct IsObject<const Object<T>&> : std::true_type {};

template<typename T> struct IsRefPtr : std::false_type {};
template<typename T> struct IsRefPtr<CRefPtr<T>> : std::true_type {};
template<typename T> struct IsRefPtr<CRefPtr<T>&> : std::true_type {};
template<typename T> struct IsRefPtr<const CRefPtr<T>&> : std::true_type {};

template<typename T> struct IsWeakPtr : std::false_type {};
template<typename T> struct IsWeakPtr<CWeakPtr<T>> : std::true_type {};
template<typename T> struct IsWeakPtr<CWeakPtr<T>&> : std::true_type {};
template<typename T> struct IsWeakPtr<const CWeakPtr<T>&> : std::true_type {};

template<typename T> struct IsString : std::false_type {};
template<> struct IsString<char*> : std::true_type {};
template<> struct IsString<const char*> : std::true_type {};
template<> struct IsString<const char*&> : std::true_type {};
template<std::size_t N> struct IsString<const char[N]> : std::true_type {};
template<std::size_t N> struct IsString<char[N]> : std::true_type {};
template<> struct IsString<EqString> : std::true_type {};
template<> struct IsString<EqString&> : std::true_type {};
template<> struct IsString<const EqString&> : std::true_type {};
template<> struct IsString<EqStringRef> : std::true_type {};
template<> struct IsString<EqStringRef&> : std::true_type {};
template<> struct IsString<const EqStringRef&> : std::true_type {};

template<typename T> struct IsEqString : std::false_type {};
template<> struct IsEqString<EqString> : std::true_type {};
template<> struct IsEqString<EqString&> : std::true_type {};
template<> struct IsEqString<const EqString&> : std::true_type {};
template<> struct IsEqString<EqStringRef> : std::true_type {};
template<> struct IsEqString<EqStringRef&> : std::true_type {};
template<> struct IsEqString<const EqStringRef&> : std::true_type {};

struct ObjectIndexGetter
{
	template<typename T>
	static int Get(const Object<T>& obj) { return obj.m_index; }
};

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
	using IsScriptState = std::is_same<BaseRefType<First>, ScriptState>;

	static const char* Get()
	{
		static EqString result = []() {
			const char* restStr = ArgsSignature<Rest...>::Get();
			if constexpr (IsScriptState::value)
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
	if constexpr (PushType<T>::value == BY_VALUE)
	{
		T* ud = static_cast<T*>(lua_newuserdatauv(L, sizeof(T), 0));
		new(ud) T{ std::forward<Args>(args)... };
		luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);

		return *ud;
	}
	else
	{
		T* newObj = PPNew T{ std::forward<Args>(args)... };

		//BoxUD* ud = new(lua_newuserdatauv(L, sizeof(BoxUD), 0)) BoxUD();
		//ud->objPtr = newObj;
		//ud->flags = BOX_UD_FLAG_OWNED;
		//luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);

		bool justCreated;
		BoxUD* ud = GetBoxUD(L, newObj, BOX_UD_FLAG_OWNED, LuaBaseTypeAlias<T>::value, justCreated);
		ASSERT(justCreated);

		if constexpr (PushType<T>::value == REF_PTR)
			newObj->Ref_Grab();

		return *newObj;
	}
}

template<typename T>
static int DestroyImpl(lua_State* L)
{
	using UT = StripTraitsT<T>;
	using BaseUType = BaseType<UT>;

	// destructor is safe to use statically-compiled ByVal
	if constexpr (PushType<T>::value == BY_VALUE)
	{
		ESL_VERBOSE_LOG("destroy val %s", LuaBaseTypeAlias<T>::value);
		T* ud = static_cast<T*>(lua_touserdata(L, 1));
		ud->~T();
	}
	else
	{
		BoxUD* ud = static_cast<BoxUD*>(lua_touserdata(L, 1));
		ASSERT(ud);

		if constexpr (PushType<BaseUType>::value == WEAK_REF)
		{
			using WeakHandle = typename WeakRefObject<BaseUType>::WeakHandle;

			WeakHandle* weakHandle = reinterpret_cast<WeakHandle*>(ud->weakRefHandle);
			if (weakHandle)
				weakHandle->Ref_Drop();
		}

		if constexpr (PushType<BaseUType>::value == REF_PTR)
		{
			ESL_VERBOSE_LOG("deref obj %s", LuaBaseTypeAlias<T>::value);
			static_cast<T*>(ud->objPtr)->Ref_Drop();
		}
		else
		{
			if (ud->flags & BOX_UD_FLAG_OWNED)
			{
				ESL_VERBOSE_LOG("destroy owned obj %s", LuaBaseTypeAlias<T>::value);
				delete static_cast<T*>(ud->objPtr);
				ud->flags &= ~BOX_UD_FLAG_OWNED;
				ud->objPtr = nullptr;
			}
		}

		// remove from cache
		RemoveBoxUD(L, ud);
	}
	return 0;
}

// Push pull is essential when you want to send or get values from Lua
template<typename T>
struct PushGetImpl
{
	using UT = StripTraitsT<T>;
	using BaseUType = BaseType<UT>;

	static void PushObject(lua_State* L, const T& obj, int flags)
	{
		ASSERT_MSG(&obj, "NULL object passed as Ref or Box, use pushnil");

		static_assert(std::is_fundamental_v<BaseUType> == false, "PushObject used for fundamental type");

		if constexpr (PushType<BaseUType>::value == BY_VALUE)
		{
			BaseUType* ud = static_cast<BaseUType*>(lua_newuserdatauv(L, sizeof(BaseUType), 0));
			new(ud) BaseUType(obj);
			luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);
		}
		else
		{
			//BoxUD* ud = new(lua_newuserdatauv(L, sizeof(BoxUD), 0)) BoxUD();
			//ud->objPtr = const_cast<void*>(reinterpret_cast<const void*>(&obj));
			//ud->flags = flags;
			//luaL_setmetatable(L, LuaBaseTypeAlias<T>::value);

			bool justCreated;
			BoxUD* ud = GetBoxUD(L, const_cast<void*>(reinterpret_cast<const void*>(&obj)), flags, LuaBaseTypeAlias<T>::value, justCreated);

			if(justCreated)
			{
				if constexpr (PushType<BaseUType>::value == REF_PTR)
					const_cast<BaseUType*>(&obj)->Ref_Grab();

				if constexpr (PushType<BaseUType>::value == WEAK_REF)
				{
					auto* weakHandle = obj.GetWeakHandle();
					weakHandle->Ref_Grab();

					ud->weakRefHandle = weakHandle;
				}
			}
		}
	}

	static T* GetObject(lua_State* L, int index, bool toCpp, bool& isConst, const bindings::BaseClassStorage::Info& upcastBaseInfo)
	{
		isConst = false;

		static_assert(std::is_fundamental_v<BaseUType> == false, "GetObject used for fundamental type");
		ASSERT(upcastBaseInfo.offset == 0);

		void* objPtr = lua_touserdata(L, index);

		if constexpr (PushType<BaseUType>::value == BY_VALUE)
		{
			return reinterpret_cast<UT*>(reinterpret_cast<uintptr_t>(objPtr) + upcastBaseInfo.offset);
		}
		else
		{
			BoxUD* ud = static_cast<BoxUD*>(objPtr);
			if (!ud)
				return static_cast<BaseUType*>(nullptr);

			isConst = (ud->flags & BOX_UD_FLAG_CONST);

			if constexpr (PushType<BaseUType>::value == WEAK_REF)
			{
				WeakRefObject<void>::WeakHandle* weakHandle = reinterpret_cast<WeakRefObject<void>::WeakHandle*>(ud->weakRefHandle);
				if (weakHandle && !weakHandle->ptr)
					return static_cast<BaseUType*>(nullptr);
			}

			// drop ownership flag when ToCpp is specified
			// so Lua can no longer delete object (C++ now has to)
			if (toCpp)
				ud->flags &= ~BOX_UD_FLAG_OWNED;

			return reinterpret_cast<BaseUType*>(reinterpret_cast<uintptr_t>(ud->objPtr) + upcastBaseInfo.offset);
		}
	}

	static void* GetThis(lua_State* L, bool& isConst)
	{
		isConst = false;

		static_assert(std::is_fundamental_v<BaseUType> == false, "ThisGetter used for fundamental type");

		void* objPtr = lua_touserdata(L, 1);

		if constexpr (PushType<BaseUType>::value == BY_VALUE)
		{
			return objPtr;
		}
		else
		{
			BoxUD* ud = static_cast<BoxUD*>(objPtr);
			if (!ud)
				return nullptr;

			isConst = (ud->flags & BOX_UD_FLAG_CONST);

			if constexpr (PushType<BaseUType>::value == WEAK_REF)
			{
				WeakRefObject<void>::WeakHandle* weakHandle = reinterpret_cast<WeakRefObject<void>::WeakHandle*>(ud->weakRefHandle);
				if (weakHandle && !weakHandle->ptr)
					return nullptr;
			}

			return ud->objPtr;
		}
	}
};

template<typename T, typename WT>
static void PushValue(lua_State* L, const T& value)
{
	if constexpr (std::is_same_v<T, std::nullptr_t>)
	{
		lua_pushnil(L);
	}
	else if constexpr (std::is_same_v<T, void*> || std::is_same_v<T, const void*>)
	{
		lua_pushlightuserdata(L, value);
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		lua_pushboolean(L, value);
	}
	else if constexpr (
		   std::is_same_v<T, long>
		|| std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, char>
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
	else if constexpr (std::is_base_of_v<bindings::LuaCFunctionProto, BaseRefType<T>>)
	{
		lua_pushlightuserdata(L, value.funcPtr);

		constexpr int tupleSize = std::tuple_size<typename BaseRefType<T>::TupleVal>::value;
		std::apply([&](auto&&... args) {
			((PushValue(L, args)), ...);
		}, value.upValues);
		lua_pushcclosure(L, value.luaFuncImpl, tupleSize + 1);
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
	else if constexpr (binder::IsRefPtr<T>::value || binder::IsWeakPtr<T>::value)
	{
		using UT = BaseType<StripWeakPtrT<StripRefPtrT<BaseType<T>>>>;

		// can't be used for RefPtr but it's ok for weak pointer
		constexpr int retTraitFlag = HasToLuaReturnTrait<WT>::value ? BOX_UD_FLAG_OWNED : 0;
		if (value)
			PushGet<UT>::Push(L, value.Ref(), (std::is_const_v<UT> ? BOX_UD_FLAG_CONST : 0) | retTraitFlag);
		else
			lua_pushnil(L);
	}
	else if constexpr (binder::IsObject<T>::value)
	{
		// IDK yet
		lua_pushvalue(L, binder::ObjectIndexGetter::Get(value));
	}
	else
	{
		static_assert(std::is_integral_v<BaseType<T>> == false, "PushValue<Class> cannot be used on integral types");

		using UT = BaseType<StripWeakPtrT<T>>;

		constexpr int retTraitFlag = (HasToLuaReturnTrait<WT>::value ? BOX_UD_FLAG_OWNED : 0) | (std::is_const_v<BaseTypeWithCv<WT>> ? BOX_UD_FLAG_CONST : 0);
		if constexpr (std::is_pointer_v<T>)
		{
			if (value != nullptr)
				PushGet<UT>::Push(L, *value, retTraitFlag);
			else
				lua_pushnil(L);
		}
		else
			PushGet<UT>::Push(L, value, retTraitFlag);
	}
}

template<typename RT, typename OBJPTR>
struct ObjPtrGetter
{
	using Result = ResultWithValue<OBJPTR>;

	static_assert(std::is_trivial<RT>::value == false, "ObjPtrGetter cannot be used on trivial types");

	template<bool SilentTypeCheck, bool AllowUpcasting>
	static Result Get(lua_State* L, int index, int argType, bool toCpp, bool& isConst)
	{
		if (argType != LUA_TUSERDATA)
		{
			EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<RT>::value, lua_typename(L, argType));
			if constexpr (!SilentTypeCheck)
			{
				if (argType != LUA_TNIL)
					luaL_argerror(L, index, err);
			}

			return Result::Failure(std::move(err));
		}

		// retrieve userdata name which is class name
		const char* className = nullptr;
		{
			const int type = luaL_getmetafield(L, index, "__name");
			className = lua_tostring(L, -1);
			lua_pop(L, 1);
		}

		bool isValidUdType = CString::Compare(className, LuaBaseTypeAlias<RT>::value) == 0;
		runtime::BaseClassInfo baseInfo;
		if constexpr (AllowUpcasting)
		{
			// perform type compatibility check
			if (!isValidUdType)
			{
				baseInfo = bindings::BaseClassStorage::GetUpcastingBaseClassInfo(className, LuaBaseTypeAlias<RT>::value);
				isValidUdType = baseInfo.name.IsValid();
			}
		}

		if (isValidUdType)
		{
			OBJPTR objPtr(PushGet<RT>::Get(L, index, toCpp, isConst, baseInfo));
			return Result{ {}, true, {}, std::move(objPtr) };
		}

		// we have incompatible types
		EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<RT>::value, className);
		if constexpr (!SilentTypeCheck)
			luaL_argerror(L, index, err);

		return Result::Failure(std::move(err));
	}
};


// Lua type getters
template<typename T, bool SilentTypeCheck, bool AllowUpcasting>
static decltype(auto) GetValue(lua_State* L, int index)
{
	const int top = lua_gettop(L);

	if (index > top)
	{
		if constexpr (!SilentTypeCheck)
			luaL_error(L, "insufficient number of arguments");
	}

	const int argType = lua_type(L, index);
	auto CheckType = [argType](lua_State* L, int index, int type) -> bool
	{
		if constexpr (SilentTypeCheck)
		{
			if (argType != type)
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
		using Result = ResultWithValue<bool>;

		if (!CheckType(L, index, LUA_TBOOLEAN))
			return Result::Failure(EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType)));

		return Result{ {}, true, {}, lua_toboolean(L, index) != 0 };
    }
	else if constexpr (
		   std::is_same_v<T, long>
		|| std::is_same_v<T, int>
		|| std::is_same_v<T, uint>
		|| std::is_same_v<T, char>
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
		using Result = ResultWithValue<BaseType<T>>;

		if (!CheckType(L, index, LUA_TNUMBER))
			return Result::Failure(EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType)));

		return Result{ {}, true, {}, static_cast<T>(lua_tointeger(L, index)) };
	}
	else if constexpr (
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>)
	{
		using Result = ResultWithValue<BaseType<T>>;

		if (!CheckType(L, index, LUA_TNUMBER))
			return Result::Failure(EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType)));

		return Result{ {}, true, {}, static_cast<T>(lua_tonumber(L, index)) };
    }
	else if constexpr (
		std::is_same_v<T, void*>
		|| std::is_same_v<T, const void*>)
	{
		using Result = ResultWithValue<T>;

		if (!CheckType(L, index, LUA_TLIGHTUSERDATA))
			return Result::Failure(EqString::Format("expected %s, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType)));

		void* udPtr = lua_touserdata(L, index);
		return Result{ {}, true, {}, udPtr };
	}
	else if constexpr (std::is_same_v<BaseType<T>, LuaRawRef>)
	{
		using Result = ResultWithValue<BaseType<T>>;
		return Result{ {}, true, {}, LuaRawRef(L, index, argType) };
	}
	else if constexpr (std::is_same_v<BaseType<T>, LuaFunctionRef>)
	{
		using Result = ResultWithValue<BaseType<T>>;

		if (argType != LUA_TNIL && !CheckType(L, index, LUA_TFUNCTION))
			return Result::Failure();

		return Result{ {}, true, {}, BaseType<T>(L, index) };
	}
	else if constexpr (
		   std::is_same_v<BaseType<T>, LuaTableRef>
		|| std::is_same_v<BaseType<T>, LuaTable>)
	{
		using Result = ResultWithValue<BaseType<T>>;

		if (argType != LUA_TNIL && !CheckType(L, index, LUA_TTABLE))
			return Result::Failure();

		return Result{ {}, true, {}, BaseType<T>(L, index) };

	}
	else if constexpr (binder::IsString<T>::value)
	{
		if (argType != LUA_TSTRING)
		{
			EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType));
			if constexpr (!SilentTypeCheck)
			{
				if (argType == LUA_TNIL)
				{
					if constexpr (!std::is_pointer_v<T>)
						luaL_argerror(L, index, err);
				}
				else
					luaL_argerror(L, index, err);
			}

			if constexpr (binder::IsEqString<T>::value)
			{
				// TODO: make EqStringRef support optional values
				static_assert(!std::is_pointer_v<T>, "passing EqString[Ref] by pointer is not supported yet");

				using BaseStringType = BaseType<T>;
				using Result = ResultWithValue<BaseStringType>;

				return Result::Failure(std::move(err));
			}
			else
			{
				using Result = ResultWithValue<const char*>;
				return Result{ {}, true, {}, nullptr};
			}
		}

		size_t len = 0;
		const char* value = lua_tolstring(L, index, &len);

		if constexpr (binder::IsEqString<T>::value)
		{
			static_assert(!std::is_pointer_v<T>, "passing EqString[Ref] by pointer is not supported yet");

			using BaseStringType = BaseType<T>;
			using Result = ResultWithValue<BaseStringType>;
			return Result{ {}, true, {}, BaseStringType(value, len) };
		}
		else
		{
			using Result = ResultWithValue<const char*>;
			return Result{ {}, true, {}, value };
		}
	}
	else if constexpr (binder::IsObject<T>::value)
	{
		using Result = ResultWithValue<BaseType<T>>;
		return Result{ {}, true, {}, {L, index} };
	}
	else if constexpr (binder::IsRefPtr<T>::value)
	{
		// simple return without conversion
		static_assert(!HasToCppParamTrait<T>::value, "can't use ToCpp trait on CRefPtr");

		using RT = StripRefPtrT<BaseType<T>>;
		using REFPTR = CRefPtr<RT>;

		bool isConst; // TODO: support!
		return ObjPtrGetter<RT, REFPTR>::template Get<SilentTypeCheck, AllowUpcasting>(L, index, argType, false, isConst);
	}
	else if constexpr (binder::IsWeakPtr<T>::value)
	{
		using RT = StripWeakPtrT<BaseType<T>>;
		using WEAKPTR = CWeakPtr<RT>;
		constexpr bool toCpp = HasToCppParamTrait<T>::value;

		bool isConst; // TODO: support!
		return ObjPtrGetter<RT, WEAKPTR>::template Get<SilentTypeCheck, AllowUpcasting>(L, index, argType, toCpp, isConst);
	}
	else
	{
		// TODO: make ObjPtrGetter compatible with code below as the code is mostly the same except EmitArgError and is_reference
		using UT = std::remove_const_t<StripTraitsT<StripObjectT<T>>>;
		using Result = ResultWithValue<UT>;

		static_assert(std::is_integral_v<BaseType<T>> == false, "GetValue<Class> cannot be used on integral types");

		auto EmitArgError = [L, index, argType](EqString&& err) {
			if constexpr (!SilentTypeCheck)
			{
				if (argType == LUA_TNIL)
				{
					if constexpr (!std::is_pointer_v<T>)
						luaL_argerror(L, index, err);
				}
				else
					luaL_argerror(L, index, err);
			}
			return Result::Failure(std::move(err));
		};

		if (argType != LUA_TUSERDATA)
			return EmitArgError(EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, lua_typename(L, argType)));

		// retrieve userdata name which is class name
		const char* className = nullptr;
		{
			const int type = luaL_getmetafield(L, index, "__name");
			className = lua_tostring(L, -1);
			lua_pop(L, 1);
		}

		bool isValidUdType = CString::Compare(className, LuaBaseTypeAlias<T>::value) == 0;
		runtime::BaseClassInfo baseInfo;
		if constexpr (AllowUpcasting)
		{
			// perform type compatibility check
			if (!isValidUdType)
			{
				baseInfo = bindings::BaseClassStorage::GetUpcastingBaseClassInfo(className, LuaBaseTypeAlias<T>::value);
				isValidUdType = baseInfo.name.IsValid();
			}
		}

		if (isValidUdType)
		{
			constexpr bool toCpp = HasToCppParamTrait<T>::value;

			bool isConst;
			BaseType<UT>* objPtr = static_cast<BaseType<UT>*>(PushGet<BaseType<UT>>::Get(L, index, toCpp, isConst, baseInfo));

			if constexpr (!std::is_const_v<BaseTypeWithCv<UT>>)
			{
				if(isConst)
					return EmitArgError(EqString::Format("got const %s for non-const argument", LuaBaseTypeAlias<T>::value));
			}

			if constexpr (std::is_reference_v<UT>)
			{
				if(!objPtr)
					return EmitArgError(EqString::Format("%s weak pointer is nil", LuaBaseTypeAlias<T>::value));

				return Result{ {}, true, {}, reinterpret_cast<UT>(*objPtr) };
			}
			else
				return Result{ {}, true, {}, reinterpret_cast<UT>(objPtr) };
		}

		// we have incompatible types
		// on pointer types return nullptr, on reference push runtime error
		EqString err = EqString::Format("%s expected, got %s", LuaBaseTypeAlias<T>::value, className);
		if constexpr (!SilentTypeCheck)
			luaL_argerror(L, index, err);

		return Result::Failure(std::move(err));
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
	runtime::PushValue<T, const T>(L, value);
	lua_setglobal(L, fieldName);
}

template<typename T>
static void SetGlobal(lua_State* L, const char* fieldName, T& value)
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
			return Result{ {}, false, "is not callable"};

		lua_State* L = func.GetState();

		runtime::StackGuard g(L);

		lua_pushcfunction(L, HandleRuntimeError);
		const int errIdx = lua_gettop(L);

		func.Push();
		PushArguments(L, std::forward<Args>(args)...);

		return InvokeFunc(L, std::move(g), sizeof...(Args), errIdx);
	}

	// NOTE: this variant used in ScriptState::CallFunction
	static Result Invoke(lua_State* L, int funcIndex, int popCnt, Args... args)
	{
		if (lua_type(L, funcIndex) != LUA_TFUNCTION)
			return Result{ {}, false, "is not callable"};

		runtime::StackGuard g(L, -popCnt);

		lua_pushcfunction(L, HandleRuntimeError);
		const int errIdx = lua_gettop(L);

		lua_pushvalue(L, funcIndex);
		PushArguments(L, std::forward<Args>(args)...);

		return InvokeFunc(L, std::move(g), sizeof...(Args), errIdx);
	}

private:
	static void PushArguments(lua_State* L) {}

	template<typename First, typename... Rest>
	static void PushArguments(lua_State* L, First first, Rest... rest)
	{
		PushValue(L, first);
		PushArguments(L, rest...);
	}

	static Result InvokeFunc(lua_State* L, runtime::StackGuard&& guard, int numArgs, int errIdx)
	{
		constexpr int ReturnCount = IsAny<R>::value ? LUA_MULTRET : (std::is_void_v<R> ? 0 : 1);
		const int res = lua_pcall(L, numArgs, ReturnCount, errIdx);

		const int retValues = lua_gettop(L) - guard.Pos() - (errIdx != 0 ? 1 : 0);

		if (res == 0)
		{
			if constexpr (ReturnCount == 0)
			{
				return Result{ std::move(guard), true };
			}
			else if constexpr (ReturnCount == LUA_MULTRET)
			{
				// Any
				if constexpr (R::COUNT > 0)
				{
					if (retValues < R::COUNT)
						return Result{ std::move(guard), false, EqString::Format("insufficient return value count (got %d, required %d)", retValues, R::COUNT) };
				}
				return Result{ std::move(guard), true};
			}
			else
			{
				if (retValues < ReturnCount)
					return Result{ std::move(guard), false, EqString::Format("insufficient return value count (got %d, required %d)", retValues, ReturnCount) };

				auto value = runtime::GetValue<R, true>(L, -1);
				return Result{ std::move(guard), value.success, value.success ? EqStringRef() : EqString::Format("return value: %s", value.errorMessage), *value};
			}
		}

		const char* errorMessage = nullptr;
		if (res != LUA_ERRMEM)
		{
			SetLuaErrorFromTopOfStack(L);
			errorMessage = GetLastError(L);
		}

		return Result{ std::move(guard), false, errorMessage };
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
		ESL_VERBOSE_LOG("ctor(default) %s, byval %d", ScriptClass<T>::className, PushType<T>::value == BY_VALUE);
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
		ESL_VERBOSE_LOG("ctor(...) %s, byval %d", ScriptClass<T>::className, PushType<T>::value == BY_VALUE);
		runtime::New<BaseUType>(L, *runtime::GetValue<Args, true>(L, IDX + 1)...);
	}

	static int Func(lua_State* L) 
	{
		Invoke(L, std::index_sequence_for<Args...>{});
		return 1;
	}
};

template<typename ... Args>
struct CheckLuaStateArg : std::false_type {};

template<typename First, typename ... Rest>
struct CheckLuaStateArg<First, Rest...> : std::is_same<BaseRefType<First>, ScriptState> {};

//---------------------------------------------------------------
// Member function binder

template<auto FuncPtr, typename Traits>
struct MemberFunctionBinder {};

template <auto FuncPtr, typename T, typename Traits, typename R, typename ... Args>
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
		{
			esl::ScriptState state(L);
			return (thisPtr->*FuncPtr)(state, *runtime::GetValue<std::tuple_element_t<IDX + 1, typename Traits::TArgs>, false>(L, IDX + 2)...);
		}
		else
			return (thisPtr->*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, typename Traits::TArgs>, false>(L, IDX + 2)...);
	}

	static int FuncImpl(T* thisPtr, lua_State* L)
	{
		constexpr int ArgCount = sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0);

		if constexpr (IsAny<R>::value)
		{
			Invoke(thisPtr, L, std::make_index_sequence<ArgCount>{});
			return R::COUNT;
		}
		else if constexpr (std::is_void_v<R>)
		{
			Invoke(thisPtr, L, std::make_index_sequence<ArgCount>{});
			return 0;
		}
		else
		{
			R ret = Invoke(thisPtr, L, std::make_index_sequence<ArgCount>{});
			runtime::PushValue<R, typename Traits::TR>(L, ret);
			return 1;
		}
	}
};

// Non-const binder
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...), typename Traits>
struct MemberFunctionBinder<FuncPtr, Traits> : public esl::ScriptBind
{
	int Func(lua_State* L) { return MemberFunction<FuncPtr, T, Traits, R, Args...>::FuncImpl(static_cast<T*>(this->thisPtr), L); }
	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
	static bool IsConst() { return false; }
};

// Const binder, only has const below
template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const, typename Traits>
struct MemberFunctionBinder<FuncPtr, Traits> : public esl::ScriptBind
{
	int Func(lua_State* L) { return MemberFunction<FuncPtr, T, Traits, R, Args...>::FuncImpl(static_cast<T*>(this->thisPtr), L); }
	static auto GetFuncArgsSignature() { return runtime::ArgsSignature<Args...>::Get(); }
	static int GetFuncArgsCount() { return sizeof...(Args); }
	static bool IsConst() { return true; }
};

template<auto FuncPtr>
struct MemberFunctionBinderNoTraits;

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...)>
struct MemberFunctionBinderNoTraits<FuncPtr> : public MemberFunctionBinder<FuncPtr, FuncSignature<R, Args...>> {};

template<typename T, typename R, typename ... Args, R(T::* FuncPtr)(Args...) const>
struct MemberFunctionBinderNoTraits<FuncPtr> : public MemberFunctionBinder<FuncPtr, FuncSignature<R, Args...>> {};

template<auto FuncPtr, typename Traits>
static auto BindMemberFunction() 
{ 
	using BindFunc = esl::ScriptBind::BindFunc;

	if constexpr (std::is_void_v<typename Traits::TR> && std::tuple_size_v<typename Traits::TArgs> == 0)
		return static_cast<BindFunc>(&MemberFunctionBinderNoTraits<FuncPtr>::Func);
	else
		return static_cast<BindFunc>(&MemberFunctionBinder<FuncPtr, Traits>::Func);
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
		{
			esl::ScriptState state(L);
			return (*FuncPtr)(state, *runtime::GetValue<std::tuple_element_t<IDX + 1, typename Traits::TArgs>, false>(L, IDX + 1)...);
		}
		else
			return (*FuncPtr)(*runtime::GetValue<std::tuple_element_t<IDX, typename Traits::TArgs>, false>(L, IDX + 1)...);
	}

	static int Func(lua_State* L)
	{
		constexpr int ArgCount = sizeof...(Args) - (HasLuaStateArg::value ? 1 : 0);

		if constexpr (IsAny<R>::value)
		{
			Invoke(L, std::make_index_sequence<ArgCount>{});
			return R::COUNT;
		}
		else if constexpr (std::is_void_v<R>)
		{
			Invoke(L, std::make_index_sequence<ArgCount>{});
			return 0;
		}
		else
		{
			R ret = Invoke(L, std::make_index_sequence<ArgCount>{});
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

template<typename UR = void, typename ... UArgs, typename Func, typename... UpVals>
static bindings::LuaCFunction<UpVals...> BindCFunction(Func f, UpVals... upVals)
{
	bindings::LuaCFunction<UpVals...> funcInfo;
	funcInfo.upValues = std::tie(upVals...);
	funcInfo.funcPtr = reinterpret_cast<void*>(f);

	if constexpr (std::is_void_v<UR> && sizeof...(UArgs) == 0)
		funcInfo.luaFuncImpl = &FunctionBinderNoTraits<Func>::Func;
	else
		funcInfo.luaFuncImpl = &FunctionBinder<Func, FuncSignature<UR, UArgs...>>::Func;
	return funcInfo;
}

//---------------------------------------------------------------
// Variable binder. Generates appropriate getters/setters

template<typename T, auto MemberVar>
struct MemberVariableBinder;

template<typename T, typename V, V T::* MemberVar>
struct MemberVariableBinder<T, MemberVar> : public esl::ScriptBind
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
static auto BindMemberVariableGetter()
{
	return static_cast<typename esl::ScriptBind::BindFunc>(&MemberVariableBinder<T, V>::GetterFunc);
}

template<typename T, auto V>
static auto BindMemberVariableSetter()
{
	return static_cast<typename esl::ScriptBind::BindFunc>(&MemberVariableBinder<T, V>::SetterFunc);
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
			runtime::New<T>(L, -lhs);
		}
		else if constexpr (OpType == OP_not)
		{
			runtime::New<T>(L, !lhs);
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
	m.staticFunc = &runtime::DestroyImpl<T>;
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
	m.data = reinterpret_cast<void*>(funcInfo.funcPtr);
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
	m.signature = binder::MemberFunctionBinder<F, Traits>::GetFuncArgsSignature();
	m.numArgs = binder::MemberFunctionBinder<F, Traits>::GetFuncArgsCount();
	m.isConst = binder::MemberFunctionBinder<F, Traits>::IsConst();
	m.func = binder::BindMemberFunction<F, Traits>();
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
	m.func = binder::BindMemberVariableSetter<T, V>();
	m.getFunc = binder::BindMemberVariableGetter<T, V>();
	return m;
}

template<typename T>
template<auto V, auto F>
Member ClassBinder<T>::MakeVariableExSet(const char* name)
{
	Member m;
	m.type = MEMB_VAR;
	m.name = name;
	m.signature = GetVariableTypeName<T, V>();
	m.func = binder::BindMemberFunction<F, binder::FuncSignatureDefault>();
	m.getFunc = binder::BindMemberVariableGetter<T, V>();
	return m;
}

template<typename T>
template<auto FGET, auto FSET>
Member ClassBinder<T>::MakeVariableExGetSet(const char* name)
{
	Member m;
	m.type = MEMB_VAR;
	m.name = name;
	// only setter needs valid signature
	// TODO: check setter if it has multiple or no arguments
	m.signature = binder::MemberFunctionBinder<FSET, binder::FuncSignatureDefault>::GetFuncArgsSignature();
	m.func = binder::BindMemberFunction<FSET, binder::FuncSignatureDefault>();
	m.getFunc = binder::BindMemberFunction<FGET, binder::FuncSignatureDefault>();
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

template <typename Base, typename Derived>
constexpr intptr_t ComputeBaseClassOffset()
{
	// HACK: using 0 will make offset 0 regardless of what type it is (nullptr optimization)
	constexpr intptr_t HACK_OFFSET = 0x1000;
	Derived* v = reinterpret_cast<Derived*>(HACK_OFFSET);
	return reinterpret_cast<intptr_t>(static_cast<Base*>(v)) - HACK_OFFSET;
}

template<typename T>
void BaseClassStorage::Add()
{
	if (!ScriptClass<T>::baseClassName || *ScriptClass<T>::baseClassName == 0)
		return;

	Info& info = *GetBaseClassNames().insert(ScriptClass<T>::classId);
	info.name = ScriptClass<T>::baseClassName;

	if constexpr (!std::is_void_v<typename BaseScriptClass<T>::BindType>)
	{
		using BaseClass = typename BaseScriptClass<T>::BindType;
		info.offset = ComputeBaseClassOffset<BaseClass, T>();
	}
	else
	{
		info.offset = 0;
	}
}

template<typename T>
BaseClassStorage::Info BaseClassStorage::Get()
{
	return Get(ScriptClass<T>::className);
}
}

#pragma warning(pop)