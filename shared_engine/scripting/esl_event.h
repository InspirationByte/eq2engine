#pragma once

#include "esl_luaref.h"
#include "esl_bind.h"

namespace esl
{
class LuaEvent : public WeakRefObject<LuaEvent>
{
	friend struct Handle;
public:
	struct Handle : public RefCountedObject<Handle>
	{
		~Handle();
		Handle() = default;
		Handle(LuaEvent* event, uint id);
		Handle(Handle&& other) noexcept;
		Handle& operator=(Handle&& other) noexcept;

		CWeakPtr<LuaEvent>	owner;
		uint				id{ static_cast<uint>(-1) };
	};

	using HandlePtr = CRefPtr<Handle>;
	using FunctionSet = Map<int, LuaFunctionRef>;

	HandlePtr	AddHandler(const LuaFunctionRef& func);
	void		RemoveHandler(const HandlePtr& handle);
	int			GetHandlerCount() const { return m_eventHandlers.size(); }
	void		Clear();

	FunctionSet::Iterator	GetFirstHandler() { return m_eventHandlers.begin(); }
protected:
	void			RemoveHandlerInternal(uint handle);

	FunctionSet		m_eventHandlers{ PP_SL };
	uint			m_handleIdCnt{ 0 };
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
	bool Invoke(Args... args) const
	{
		bool allSuccess = true;
		for (auto it = m_abstract.GetFirstHandler(); !it.atEnd(); ++it)
		{
			esl::runtime::StackGuard g(it.value().GetState());
			if (!EventFuncLuaCall::Invoke(it.value(), args...))
				allSuccess = false;
		}
		return allSuccess;
	}

private:
	LuaEvent& m_abstract;
};

} // esl

