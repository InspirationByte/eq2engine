//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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
const CWeakPtr<EventSubscriptionObject<SIGNATURE>> Event<SIGNATURE>::Subscribe(const EqFunction<SIGNATURE>& func, bool runOnce /*= false*/)
{
	SubscriptionObject* sub = PPNewSL(m_sl) SubscriptionObject();
	sub->func = func;
	sub->runOnce = runOnce;

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
			// if(sub == m_subs)
			//		m_subs = sub->next;
			// else
			//		prevSub->next = sub->next;
			
			SubscriptionObject* del = Atomic::Exchange(sub, Atomic::Load(sub->next));
			
			Atomic::CompareExchange(m_subs, del, Atomic::Load(del->next));

			if (prevSub)
				Atomic::Exchange(prevSub->next, Atomic::Load(del->next));

			delete del;
			continue;
		}

		EqFunction<SIGNATURE> func = sub->func;
		Atomic::CompareExchange(sub->unsubscribe, false, sub->runOnce);

		// goto next
		prevSub = Atomic::Exchange(sub, sub->next);

		// exec
		func(std::forward<Params>(args)...);
	}
}
