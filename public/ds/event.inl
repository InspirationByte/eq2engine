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
	SubscriptionObject* newSub = PPNewSL(m_sl) SubscriptionObject();
	newSub->func = func;
	newSub->runOnce = runOnce;

	// sub->next = m_subs;
	// m_subs = sub;

	while(true)
	{
		SubscriptionObject* oldHead = Atomic::Load(m_subs);
		newSub->next = oldHead;

		if(Atomic::CompareExchange(m_subs, oldHead, newSub) == oldHead)
			break;
	}
	
	return CWeakPtr(newSub);
}

template<typename SIGNATURE>
template<typename... Params>
void Event<SIGNATURE>::operator()(Params&&... args)
{
	SubscriptionObject* sub = Atomic::Load(m_subs);
	SubscriptionObject* prevSub = nullptr;
	while (sub)
	{
		if (sub->unsubscribe || sub->runOnce)
		{
			if(sub->runOnce)
			{
				sub->runOnce = false;
				sub->unsubscribe = true;
				
				sub->func(std::forward<Params>(args)...);
			}

			// if(sub == m_subs)
			//		m_subs = sub->next;
			// else
			//		prevSub->next = sub->next;
			
			SubscriptionObject* del = Atomic::Exchange(sub, Atomic::Load(sub->next));
			if(Atomic::CompareExchange(m_subs, del, sub) != del)
			{
				if(prevSub)
				{
					if(Atomic::CompareExchange(prevSub->next, del, sub) != del)
					{
						// try unlink and delete next time
						continue;
					}
				}
				else
					continue;
			}
			delete del;

			continue;
		}
		else
		{
			sub->func(std::forward<Params>(args)...);

			// goto next
			prevSub = Atomic::Exchange(sub, sub->next);
		}
	}
}
