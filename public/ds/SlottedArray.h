#pragma once

template<typename T>
class SlottedArray
{
public:
	SlottedArray(PPSourceLine sl);

	T&			operator[](const int idx) { return m_items[idx]; }
	const T&	operator[](const int idx) const { return m_items[idx]; }
	bool		operator()(const int idx) { return m_setItems[idx]; }

	T*			ptr();
	const T*	ptr() const;

	void		clear(bool deallocate = false);

	int			numElem() const;
	int			numSlots() const;

	int			add(T& item);

	void		remove(const int idx);

private:
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
inline void SlottedArray<T>::clear(bool deallocate)
{
	m_items.clear(deallocate);
	m_freeList.clear(deallocate);
	m_setItems.clear();
	if(deallocate)
		m_setItems.resize(64);
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
inline int SlottedArray<T>::add(T& item)
{
	if (m_freeList.numElem())
	{
		const int idx = m_freeList.popBack();
		return idx;
	}

	const int idx = m_items.append(item);
	if(m_setItems.numBits() < m_items.numAllocated() + 1)
		m_setItems.resize(m_items.numAllocated() + 1);

	m_setItems.set(idx, true);
	return idx;
}

template<typename T>
inline void SlottedArray<T>::remove(const int idx)
{
	ASSERT(idx >= 0);
	ASSERT(idx < m_items.numElem());

	m_freeList.append(idx);
	m_setItems.set(idx, false);
	m_items[idx] = T{};
}