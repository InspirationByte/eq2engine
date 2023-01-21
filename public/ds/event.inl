//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Game events
//////////////////////////////////////////////////////////////////////////////////

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
const CWeakPtr<EventSubscriptionObject<SIGNATURE>> Event<SIGNATURE>::Subscribe(const EqFunction<SIGNATURE>& func)
{
	SubscriptionObject* sub = PPNew SubscriptionObject();
	sub->func = func;

	// sub->next = m_subs;
	// m_subs = sub;
	Atomic::Store(sub->next, Atomic::Exchange(m_subs, sub));

	return CWeakPtr(sub);
}

template<typename SIGNATURE>
template<typename... Params>
void Event<SIGNATURE>::operator()(Params&&... args)
{
	SubscriptionObject* sub = Atomic::Load(m_subs);
	SubscriptionObject* prevSub = nullptr;
	while (sub)
	{
		if (sub->unsubscribe)
		{
			// Soapy: Not sure, could be wrong
			SubscriptionObject* del = sub;
			
			if (prevSub)
				Atomic::Exchange(prevSub->next, Atomic::Load(sub->next));
			else
				Atomic::CompareExchange(m_subs, sub, Atomic::Load(sub->next));

			sub = Atomic::Load(sub->next);

			delete del;
			continue;
		}

		// set the right handler and go
		sub->func(std::forward<Params>(args)...);

		// goto next
		prevSub = sub;
		sub = Atomic::Load(sub->next);
	}
}
