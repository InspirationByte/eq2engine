// Thread-local storage with cleanup and reinitialization support

#pragma once

template<class T>
class ThreadLocalStorage
{
public:
	ThreadLocalStorage(PPSourceLine sl) : m_storage(sl) {}
	
	void	Clear();
	T&		Get(bool* justCreated = nullptr);

    template<typename FUNC>
	void	ForEach(FUNC fn) const;

    int     GetThreadCount() const { return m_threadCnt; }
    int     GetItemCount() const { return m_storage.size(); }
protected:
	void	DeleteStorageItem(int id);

	struct TlsID
	{
		TlsID(ThreadLocalStorage& owner, int id) : owner(owner), id(id) {}
		~TlsID() { owner.DeleteStorageItem(id); }

		ThreadLocalStorage&		owner;
		int 					id;
	};

	mutable Threading::CEqReadWriteLock	m_rwLock;
	Map<int, T>		m_storage{ PP_SL };
	volatile int	m_threadCnt = 0;
};

template<class T>
void ThreadLocalStorage<T>::Clear()
{
	Threading::CScopedWriteLocker lock(m_rwLock);
	m_storage.clear();
}

template<class T>
T& ThreadLocalStorage<T>::Get(bool* justCreated)
{
	static thread_local TlsID tlsId(*this, Atomic::Increment(m_threadCnt));

	m_rwLock.LockRead();
	auto it = m_storage.find(tlsId.id);
	if(!it)
	{
		m_rwLock.UnlockRead();
		{
			Threading::CScopedWriteLocker lock(m_rwLock);
			it = m_storage.insert(tlsId.id);
		}
		if(justCreated)
			*justCreated = true;
	}
	else
	{
		m_rwLock.UnlockRead();
		if(justCreated)
			*justCreated = false;
	}
	return *it;
}

template<class T>
template<typename FUNC>
void ThreadLocalStorage<T>::ForEach(FUNC fn) const
{
	Threading::CScopedReadLocker lock(m_rwLock);
    for(T& item : m_storage)
        fn(item);
}

template<class T>
void ThreadLocalStorage<T>::DeleteStorageItem(int id)
{
	Threading::CScopedWriteLocker lock(m_rwLock);
	m_storage.remove(id);
}