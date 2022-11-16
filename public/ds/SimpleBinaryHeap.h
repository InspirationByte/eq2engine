//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Simple binary heap
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class T>
class SimpleBinaryHeap
{
public:
	SimpleBinaryHeap(const PPSourceLine& sl)
		: m_objects(sl)
	{
	}

	void	Add(T newItem);
	T		PopMin();

	void	UpHeap(int i);
	void	DownHeap(int i);

	bool	HasItems() const { return m_count; }

private:
	Array<T>	m_objects;
	int			m_count{ 0 };
};

template<class T>
void SimpleBinaryHeap<T>::Add(T newItem)
{
	m_objects.assureSize(m_count + 1);
	m_objects[m_count] = newItem;
	UpHeap(m_count++);
}

template<class T>
T SimpleBinaryHeap<T>::PopMin()
{
	T result = m_objects[0];
	m_count--;
	if (m_count > 0)
	{
		QuickSwap(m_objects[m_count], m_objects[0]);
		DownHeap(0);
	}

	return result;
}

template<class T>
void SimpleBinaryHeap<T>::UpHeap(int i)
{
	while (i > 0)
	{
		int parent = (i - 1) / 2;
		if (T::Compare(m_objects[i], m_objects[parent])) {
			break;
		}
		QuickSwap(m_objects[i], m_objects[parent]);
		i = parent;
	}
}

template<class T>
void SimpleBinaryHeap<T>::DownHeap(int i)
{
	while (i < m_count / 2)
	{
		int child = i * 2 + 1;
		if (child + 1 < m_count && T::Compare(m_objects[child], m_objects[child + 1]))
			++child;

		if (T::Compare(m_objects[child], m_objects[i]))
			break;

		QuickSwap(m_objects[i], m_objects[child]);
		i = child;
	}
}
