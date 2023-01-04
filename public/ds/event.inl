//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Game events
//////////////////////////////////////////////////////////////////////////////////

template<typename SIGNATURE>
struct EventSubscriptionObject_t
{
	EqFunction<SIGNATURE>		func{ nullptr };
	mutable bool				unsubscribe{ false };

	EventSubscriptionObject_t*	next{ nullptr };
};

template<typename SIGNATURE>
Event<SIGNATURE>::Sub::~Sub()
{
	Unsubscribe(); 
}

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
			SubscriptionObject* del = sub;
			if (prevSub)
			{
				prevSub->next = sub->next;
			}
			else if (m_subs == sub)
			{
				m_subs = sub->next;
			}

			delete del;

			sub = sub->next;
			continue;
		}

		// set the right handler and go
		sub->func(std::forward<Params>(args)...);

		// goto next
		prevSub = sub;
		sub = sub->next;
	}
}
