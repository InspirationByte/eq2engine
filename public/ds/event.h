//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Events implementation
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//--------------------------------------------------------------

template<typename SIGNATURE>
struct EventSubscriptionObject_t;

template<typename SIGNATURE>
class Event
{
public:
	using SubscriptionObject = EventSubscriptionObject_t<SIGNATURE>;

	struct Sub
	{
		Sub() = default;
		Sub(const SubscriptionObject* sub) : m_sub(sub) {};
		~Sub();

		void		Unsubscribe();

		operator	bool() const { return m_sub; }
		Sub&		operator=(const SubscriptionObject* sub)
		{
			Unsubscribe();
			m_sub = (SubscriptionObject*)sub;
			return *this;
		}

	private:
		SubscriptionObject* m_sub{ nullptr };
	};

	~Event<SIGNATURE>();

	void						Clear();

	const SubscriptionObject*	Subscribe(const EqFunction<SIGNATURE>& func);
	const SubscriptionObject*	operator+=(const EqFunction<SIGNATURE>& func) { return Subscribe(func); }

	template<typename... Params>
	void						operator()(Params&&... args);

private:
	SubscriptionObject*			m_subs{ nullptr };
	Threading::CEqMutex			m_mutex;
};

#include "event.inl"