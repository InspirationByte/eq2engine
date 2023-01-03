//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Game events
//////////////////////////////////////////////////////////////////////////////////

template<typename SIGNATURE>
struct EventSubscriptionObject_t
{
	EventSubscriptionObject_t*	next{ nullptr };
	EqFunction<SIGNATURE>		func{ nullptr };
	bool						unsubscribe{ false };
};

template<typename SIGNATURE>
void Event<SIGNATURE>::Sub::Unsubscribe()
{
	if (m_sub)
		m_sub->unsubscribe = true;
	m_sub = nullptr;
}

//-------------------------------------------------------------------------------------------

template<typename SIGNATURE>
Event<SIGNATURE>::~Event<SIGNATURE>()
{
	Clear();
}

template<typename SIGNATURE>
void Event<SIGNATURE>::Clear()
{
	Threading::CScopedMutex m(m_mutex);

	SubscriptionObject* sub = m_subs;
	while (sub)
	{
		SubscriptionObject* nextSub = sub->next;
		delete sub;
		sub = nextSub;
	}
	m_subs = nullptr;
}

template<typename SIGNATURE>
const EventSubscriptionObject_t<SIGNATURE>* Event<SIGNATURE>::Subscribe(const EqFunction<SIGNATURE>& func)
{
	Threading::CScopedMutex m(m_mutex);

	SubscriptionObject* sub = PPNew SubscriptionObject();
	sub->func = func;
	sub->next = m_subs;
	m_subs = sub;

	return sub;
}

template<typename SIGNATURE>
template<typename... Params>
void Event<SIGNATURE>::operator()(Params&&... args)
{
	Threading::CScopedMutex m(m_mutex);

	SubscriptionObject* sub = m_subs;
	SubscriptionObject* prevSub = nullptr;
	while (sub)
	{
		if (sub->unsubscribe)
		{
			if (m_subs == sub)		// quickly migrate to next element
				m_subs = sub->next;

			if (prevSub)			// link previous with next
				prevSub->next = sub->next;

			delete sub;
			sub = nullptr;

			if(prevSub)				// goto (del)sub->next
				sub = prevSub->next;

			continue;
		}

		// set the right handler and go
		sub->func(std::forward<Params>(args)...);

		// goto next
		prevSub = sub;
		sub = sub->next;
	}
}
