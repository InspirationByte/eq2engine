#pragma once

#include "esl_luaref.h"
#include "esl_bind.h"

namespace esl
{
class LuaEvent
{
public:
	struct FuncRefWrapper : esl::LuaFunctionRef
	{
		FuncRefWrapper() = default;
		FuncRefWrapper(const esl::LuaFunctionRef& funcRef) : esl::LuaFunctionRef(funcRef) {}
		FuncRefWrapper(const FuncRefWrapper& other) : esl::LuaFunctionRef(other) {}
		FuncRefWrapper(FuncRefWrapper&& other) noexcept : esl::LuaFunctionRef(std::move(other)) {}

		FuncRefWrapper& operator=(FuncRefWrapper&& other) noexcept { *(static_cast<LuaRawRef*>(this)) = std::move(other); return *this; }

		inline friend bool operator==(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref == b.m_ref; }
		inline friend bool operator!=(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref != b.m_ref; }
		inline friend bool operator>(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref > b.m_ref; }
		inline friend bool operator<(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref < b.m_ref; }
		inline friend bool operator>=(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref >= b.m_ref; }
		inline friend bool operator<=(const FuncRefWrapper& a, const FuncRefWrapper& b) { return a.m_ref <= b.m_ref; }
	};

	using FunctionSet = Set<FuncRefWrapper>;

	void		AddHandler(const esl::LuaFunctionRef& func);
	void		RemoveHandler(const esl::LuaFunctionRef& func);
	void		Clear();

	FunctionSet::Iterator	GetFirstHandler() { return m_eventHandlers.begin(); }
protected:
	Set<FuncRefWrapper>		m_eventHandlers{ PP_SL };
};

template<typename ... Args>
class LuaEventCaller
{
public:
	LuaEventCaller(LuaEvent& abstract) :
		m_abstract(abstract)
	{
	}

	using EventFuncLuaCall = esl::runtime::FunctionCall<void, Args...>;
	void Invoke(Args... args) const
	{
		for (auto it = m_abstract.GetFirstHandler(); !it.atEnd(); ++it)
		{
			esl::runtime::StackGuard g(it.key().GetState());
			EventFuncLuaCall::Invoke(it.key(), args...);
		}
	}

private:
	LuaEvent& m_abstract;
};

} // esl
