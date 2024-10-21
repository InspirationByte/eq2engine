//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Events implementation
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//--------------------------------------------------------------

template<typename SIGNATURE>
struct EventSubscriptionObject : public WeakRefObject<EventSubscriptionObject<SIGNATURE>>
{
	EqFunction<SIGNATURE>		func{ nullptr };
	EventSubscriptionObject*	next{ nullptr };
	int							unsubscribe{ false };
	int							runOnce{ false };
};

template<typename SIGNATURE>
class Event
{
public:
	using SubscriptionObject = EventSubscriptionObject<SIGNATURE>;
	using SubscriptionPtr = CWeakPtr<SubscriptionObject>;

	struct Sub
	{
		Sub() = default;
		Sub(const SubscriptionPtr sub) : m_sub(sub) {};
		~Sub();

		void		Unsubscribe();

		operator	bool() const { return m_sub; }
		Sub&		operator=(SubscriptionPtr sub)
		{
			Unsubscribe();
			m_sub = sub;
			return *this;
		}

	private:
		SubscriptionPtr m_sub;
	};

	Event(PPSourceLine sl)
		: m_sl(sl)
	{
	}
	~Event<SIGNATURE>();

	void					Clear();

	const SubscriptionPtr	Subscribe(const EqFunction<SIGNATURE>& func, bool runOnce = false);
	const SubscriptionPtr	operator+=(const EqFunction<SIGNATURE>& func) { return Subscribe(func); }

	template<typename... Params>
	void					operator()(Params&&... args);

	void					GetSubscriptionsFlat(Array<SubscriptionObject*>& list);

private:
	template<typename FUNC>
	void					ForEach(FUNC func);

	SubscriptionObject*		m_subs{ nullptr };
	const PPSourceLine		m_sl;
};

#include "event.inl"