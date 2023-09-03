#pragma once

#include "esl_runtime.h"

// Lua Reference
namespace esl
{
class LuaRawRef
{
public:
	virtual ~LuaRawRef();
	LuaRawRef() = default;
	LuaRawRef(lua_State* L, int idx, int type);
	LuaRawRef(const LuaRawRef& other);
	LuaRawRef(LuaRawRef&& other) noexcept;

	LuaRawRef& operator=(const LuaRawRef& other);
	LuaRawRef& operator=(LuaRawRef&& other) noexcept;
	LuaRawRef& operator=(std::nullptr_t);
	bool operator==(LuaRawRef const& rhs) const;

	void Release();

	// Pushes the referenced Lua value onto the stack
	void Push() const;
	bool IsValid() const;

	operator bool() const { return IsValid(); }

	lua_State* GetState() const { return m_state; }

protected:
	void Unref();

	lua_State*	m_state{ nullptr };
	int			m_ref{ LUA_NOREF };
};

template<int TYPE>
class LuaRef : public LuaRawRef
{
public:
	LuaRef() = default;
	LuaRef(lua_State* L, int idx) : LuaRawRef(L, idx, TYPE) {}
	LuaRef(const LuaRef& other) : LuaRawRef(other) {}
	LuaRef(LuaRef&& other) noexcept : LuaRawRef(std::move(other)) {}

	LuaRef& operator=(const LuaRef& other)		{ *(static_cast<LuaRawRef*>(this)) = other; return *this; }
	LuaRef& operator=(LuaRef&& other) noexcept	{ *(static_cast<LuaRawRef*>(this)) = std::move(other); return *this; }
	LuaRef& operator=(std::nullptr_t)			{ *(static_cast<LuaRawRef*>(this)) = nullptr; return *this;}
	bool operator==(LuaRef const& rhs) const	{ return *(static_cast<const LuaRawRef*>(this)) == rhs; }
};

using LuaFunctionRef = LuaRef<LUA_TFUNCTION>;
using LuaTableRef = LuaRef<LUA_TTABLE>;

//---------------------------------------------------
// TODO: esl_luatable.h

class LuaTable : public LuaTableRef
{
public:
	struct IPairsIterator
	{
		lua_State* L{ nullptr };
		int tableIndex{ INT_MAX };
		int arrayIndex{ INT_MAX };

		IPairsIterator(const esl::LuaTable& table);
		~IPairsIterator();
		bool			AtEnd() const;
		int				operator*() const;
		IPairsIterator& operator++();
	};

	LuaTable() = default;
	LuaTable(lua_State* L, int idx) : LuaTableRef(L, idx) {}
	LuaTable(const LuaTable& other) : LuaTableRef(other) {}
	LuaTable(LuaTable&& other) noexcept : LuaTableRef(std::move(other)) {}

	LuaTable& operator=(const LuaTable& other)		{ *(static_cast<LuaRawRef*>(this)) = other; return *this; }
	LuaTable& operator=(LuaTable&& other) noexcept	{ *(static_cast<LuaRawRef*>(this)) = std::move(other); return *this; }
	LuaTable& operator=(std::nullptr_t)				{ *(static_cast<LuaRawRef*>(this)) = nullptr; return *this;}
	bool operator==(LuaTable const& rhs) const		{ return *(static_cast<const LuaRawRef*>(this)) == *(static_cast<const LuaRawRef*>(&rhs)); }

	template<typename T>
	using Result = ResultWithValue<T>;

	template<typename V, typename K>
	Result<V> Get(const K& key) const;

	template<typename K>
	bool HasKey(const K& key) const;

	template<typename V, typename K>
	void Set(const K & key, const V & value);

	template<typename K>
	void Remove(K const& key);

	int Length() const;

	IPairsIterator IPairs() const { return IPairsIterator(*this); }
};

template<typename V, typename K>
LuaTable::Result<V> LuaTable::Get(const K& key) const
{
	if (!IsValid())
		return Result<V>{false, {}};
	runtime::StackGuard g(m_state);
	Push();

	runtime::PushValue(m_state, key);
	lua_gettable(m_state, -2);
	return runtime::GetValue<V, true>(m_state, -1);
}

template<typename K>
bool LuaTable::HasKey(const K& key) const
{
	if (!IsValid())
		return false;
	runtime::StackGuard g(m_state);
	Push();

	runtime::PushValue(m_state, key);
	lua_gettable(m_state, -2);
	return lua_type(m_state, -1) != LUA_TNIL;
}

template<typename V, typename K>
void LuaTable::Set(const K& key, const V& value)
{
	if (!IsValid())
		return;
	runtime::StackGuard g(m_state);
	Push();

	runtime::PushValue(m_state, key);
	runtime::PushValue(m_state, value);
	lua_settable(m_state, -3);
}

template<typename K>
void LuaTable::Remove(K const& key)
{
	if (!IsValid())
		return;
	runtime::StackGuard g(m_state);
	Push();

	runtime::PushValue(m_state, key);
	lua_pushnil(m_state);
	lua_settable(m_state, -3);
}

}