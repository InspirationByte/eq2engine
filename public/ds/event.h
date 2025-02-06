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
		Sub(SubscriptionObject& sub) : m_sub(sub) {};
		~Sub();

		void		Unsubscribe();

		operator	bool() const { return m_sub; }
		Sub&		operator=(SubscriptionObject& sub)
		{
			Unsubscribe();
			m_sub.Assign(&sub);
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

	void				Clear();

	SubscriptionObject&	Subscribe(const EqFunction<SIGNATURE>& func, bool runOnce = false);

	void				GetSubscriptionsFlat(Array<SubscriptionObject*>& list);
	int					GetSubscriptionCount() const { return m_count; }

	SubscriptionObject&	operator+=(const EqFunction<SIGNATURE>& func) { return Subscribe(func); }

	template<typename... Args>
	void				operator()(Args&&... args);

private:
	template<typename FUNC>
	void					ForEach(FUNC func);

	SubscriptionObject*	m_subs{ nullptr };
	volatile int		m_count{ 0 };
	const PPSourceLine	m_sl;
};

#include "event.inl"