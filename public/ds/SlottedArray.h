#pragma once

template<typename T>
class SlottedArray
{
public:
	SlottedArray(PPSourceLine sl);

	T&			operator[](const int idx);
	const T&	operator[](const int idx) const;
	bool		operator()(const int idx) const;

	T*			ptr();
	const T*	ptr() const;

	void		clear(bool deallocate = false);
	void		reserve(int count);

	int			numElem() const;
	int			numSlots() const;

	int			add(const T& item);
	void		remove(const int idx);

protected:
	Array<T>	m_items{ PP_SL };
	Array<int>	m_freeList{ PP_SL };
	BitArray	m_setItems{ PP_SL };
};

//---------------------------------------------

template<typename T>
inline SlottedArray<T>::SlottedArray(PPSourceLine sl)
	: m_items(sl)
	, m_freeList(sl)
	, m_setItems(sl)
{
}

template<typename T>
inline T& SlottedArray<T>::operator[](const int idx)
{
	ASSERT_MSG(m_setItems[idx], "invalid slot %d", idx);
	return m_items[idx];
}

template<typename T>
inline const T& SlottedArray<T>::operator[](const int idx) const
{
	ASSERT_MSG(m_setItems[idx], "invalid slot %d", idx);
	return m_items[idx];
}

template<typename T>
inline bool SlottedArray<T>::operator()(const int idx) const
{
	ASSERT_MSG(m_items.inRange(idx), "invalid index %d (numElem=%d)", idx, m_items.numElem());
	return m_setItems[idx];
}

template<typename T>
inline void SlottedArray<T>::clear(bool deallocate)
{
	m_items.clear(deallocate);
	m_freeList.clear(deallocate);
	m_setItems.clear();
	if(deallocate)
		m_setItems.resize(64);
}

template<typename T>
inline void SlottedArray<T>::reserve(int count)
{
	m_items.reserve(count);
	if (m_setItems.numBits() < m_items.numAllocated() + 1)
		m_setItems.resize(m_items.numAllocated() + 1);
}

template<typename T>
inline int SlottedArray<T>::numElem() const
{
	return m_items.numElem() - m_freeList.numElem();
}

template<typename T>
inline int SlottedArray<T>::numSlots() const
{
	return m_items.numElem();
}

template<typename T>
inline T* SlottedArray<T>::ptr()
{
	return m_items.ptr();
}

template<typename T>
inline const T* SlottedArray<T>::ptr() const
{
	return m_items.ptr();
}

template<typename T>
inline int SlottedArray<T>::add(const T& item)
{
	int idx = -1;
	if (m_freeList.numElem())
	{
		idx = m_freeList.popBack();
		m_items[idx] = item;
	}
	else
	{
		idx = m_items.append(item);
		if(m_setItems.numBits() < m_items.numAllocated() + 1)
			m_setItems.resize(m_items.numAllocated() + 1);
	}

	m_setItems.set(idx, true);
	return idx;
}

template<typename T>
inline void SlottedArray<T>::remove(const int idx)
{
	if(!m_setItems[idx])
	{
		ASSERT_FAIL("Trying to remove invalid slot %d", idx);
		return;
	}

	m_items[idx] = T{};
	m_setItems.set(idx, false);
	m_freeList.append(idx);
}