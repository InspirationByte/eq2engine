//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Dynamic array (vector) of elements
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ds/sort.h"

template< typename T >
class ArrayStorageBase
{
public:
	/*
		Any array storage class must be implemented with those methods:

	void		free() = 0;
	void		resize(int newSize, int& numOfElements) = 0;

	int			getSize() const = 0;
	int			getGranularity() const { return 1; }
	void		setGranularity(int newGranularity) {}

	T*			getData() = 0;
	const T*	getData() const = 0;
	*/
	static void			destructElements(T* dest, int count);
};

template< typename T >
class DynamicArrayStorage
{
public:
	DynamicArrayStorage(const PPSourceLine& sl, int granularity)
		: m_sl(sl), m_nGranularity(granularity)
	{
	}

	~DynamicArrayStorage()
	{
		delete[] reinterpret_cast<ubyte*>(m_pListPtr);
	}

	void free()
	{
		delete[] reinterpret_cast<ubyte*>(m_pListPtr);
		m_pListPtr = nullptr;
		m_nSize = 0;
	}

	void resize(int newCapacity, int& numOfElements)
	{
		ASSERT_MSG(newCapacity >= 0, "DynamicArrayStorage<%s> requested capacity is %d", typeid(T).name(), newCapacity);

		// free up the listPtr if no data is being reserved
		if (newCapacity <= 0)
		{
			free();
			return;
		}

		// not changing the elemCount, so just exit
		if (newCapacity <= m_nSize)
			return;

		T* temp = m_pListPtr;
		m_nSize = (newCapacity + m_nGranularity - 1) / m_nGranularity * m_nGranularity;

		const int oldNumOfElems = numOfElements;
		if (m_nSize < numOfElements)
			numOfElements = m_nSize;

		// copy the old m_pListPtr into our new one
		m_pListPtr = reinterpret_cast<T*>(PPNewSL(m_sl) ubyte[m_nSize * sizeof(T)]);

		if (temp)
		{
			for (int i = 0; i < numOfElements; i++)
				new(&m_pListPtr[i]) T(std::move(temp[i]));

			// delete the old m_pListPtr if it exists
			PPFree(temp);
		}
	}

	int			getSize() const						{ return m_nSize; }
	int			getGranularity() const				{ return m_nGranularity;}
	void		setGranularity(int newGranularity)	{ m_nGranularity = newGranularity; }

	T*			getData()							{ return m_pListPtr; }
	const T*	getData() const						{ return m_pListPtr; }

	void swap(DynamicArrayStorage& other)
	{
		QuickSwap(m_nSize, other.m_nSize);
		QuickSwap(m_nGranularity, other.m_nGranularity);
		QuickSwap(m_pListPtr, other.m_pListPtr);
	}

	void swap(T*& other, int& otherNumElem)
	{
		QuickSwap(m_pListPtr, other);
		m_nSize = otherNumElem;
	}

	const PPSourceLine	getSL() const { return m_sl; }

protected:
	const PPSourceLine	m_sl;
	T* 					m_pListPtr{ nullptr };

	int					m_nSize{ 0 };
	int					m_nGranularity{ 16 };
};

template< typename T, int SIZE >
class FixedArrayStorage
{
public:
	FixedArrayStorage()
	{
	}

	void free()
	{
	}

	void resize(int newSize, int& numOfElements)
	{
		ASSERT_MSG(newSize <= SIZE, "FixedArrayStorage<%s> capacity is %d, required %d", typeid(T).name(), getSize(), newSize);
	}

	int			getSize() const						{ return SIZE; }
	int			getGranularity() const				{ return 1; }
	void		setGranularity(int newGranularity)	{}

	T*			getData()							{ return (T*)(&m_data[0]); }
	const T*	getData() const						{ return (T*)(&m_data[0]); }

	void swap(FixedArrayStorage& other)
	{
		ASSERT(other.getSize() == SIZE);
		for (int i = 0; i < SIZE; ++i)
			QuickSwap(getData()[i], other.getData()[i]);
	}

	void swap(T*& other, int& otherNumElem)
	{
		ASSERT(otherNumElem == SIZE);
		for (int i = 0; i < SIZE; ++i)
			QuickSwap(getData()[i], getData()[i]);
	}

	const PPSourceLine	getSL() const { return PPSourceLine::Empty(); }
protected:
	ubyte			m_data[SIZE * sizeof(T)];
};

template< typename T>
void ArrayStorageBase<T>::destructElements(T* dest, int count)
{
	ASSERT(count >= 0);
	while (count--)
	{
		dest->~T();
		++dest;
	}
}

//----------------------------------------------


template< typename T, typename STORAGE_TYPE >
class ArrayBase : STORAGE_TYPE
{
	using SelfType = ArrayBase<T, STORAGE_TYPE>;
public:
	using ITEM = T;

	struct Iterator
	{
		SelfType&	array;
		int			index = 0;

		Iterator(SelfType& array, int from)
			: array(array), index(from)
		{}

		bool	operator==(Iterator& it) const { return it.index == index; }
		bool	operator!=(Iterator& it) const { return it.index != index; }

		bool	atEnd() const	{ return index >= array.numElem(); }
		T&		operator*()		{ return array[index]; }
		void	operator++()	{ ++index; }
	};

	ArrayBase();
	ArrayBase(const PPSourceLine& sl, int granularity = 16);

	~ArrayBase<T, STORAGE_TYPE>();

	const T&		operator[](int index) const;
	T&				operator[](int index);
	SelfType&		operator=(const SelfType& other);

	Iterator		begin() const	{ return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const		{ return Iterator(*const_cast<SelfType*>(this), m_nNumElem); }

	// cleans list
	void			clear(bool deallocate = false);

	// returns true if index is in range
	bool			inRange(int index) const;

	// returns number of elements in list
	int				numElem() const;

	// returns number elements which satisfies to the condition
	template< typename COMPAREFUNC >
	int				numElem(COMPAREFUNC comparator) const;

	// returns number of elements allocated for
	int				numAllocated() const;

	// sets new granularity
	void			setGranularity(int newgranularity);

	// get the current granularity
	int				getGranularity() const;

	// resizes the list
	void			resize(int newsize);

	// reserve capacity
	void			reserve(int requiredSize);

	// sets the number of elements in list and resize to exactly this number if necessary
	void			setNum(int newnum, bool resize = true);

	// returns a pointer to the list
	T*				ptr();

	// returns a pointer to the list
	const T*		ptr() const;

	// returns front item
	T&				front();

	// returns front item
	const T&		front() const;

	// returns back item
	T&				back();

	// returns back item
	const T&		back() const;

	// removes front item from list
	T				popFront();

	// removes back item from list
	T				popBack();

	// appends element
	int				append(const T& obj);

	// appends element
	int				append(T&& obj);

	// emplaces element
	template<typename... Args>
	int				appendEmplace(Args&&... args);

	// append a empty element to be filled
	T&				append();

	// appends another array
	int				append(const T* other, int count);

	// appends another list
	template< typename CT, typename OTHER_STORAGE_TYPE>
	int				append(const ArrayBase<CT, OTHER_STORAGE_TYPE>& other);

	// appends another list with transformation
	// return false to not add the element
	template< typename T2, typename OTHER_STORAGE_TYPE, typename TRANSFORMFUNC >
	int				append(const ArrayBase<T2, OTHER_STORAGE_TYPE>& other, TRANSFORMFUNC transform);

	// inserts the element at the given index
	int				insert(const T& obj, int index = 0);

	// inserts the element at the given index
	int				insert(T&& obj, int index = 0);

	// inserts the new element at the given index
	T&				insert(int index = 0);

	// adds unique element
	int				addUnique(const T& obj);

	// adds unique element
	template< typename PAIRCOMPAREFUNC = PairCompareFunc<T> >
	int				addUnique(const T& obj, PAIRCOMPAREFUNC comparator);

	// returns iterator for the given element
	Iterator		find(const T& obj) const;

	// returns iterator for the given element
	template< typename PAIRCOMPAREFUNC = PairCompareFunc<T> >
	Iterator		find(const T& obj, PAIRCOMPAREFUNC comparator) const;

	// returns first found element which satisfies to the condition
	template< typename COMPAREFUNC >
	Iterator		find(COMPAREFUNC comparator) const;

	// removes the element at the given index
	bool			removeIndex(int index);

	// removes specified count of elements from specified index
	bool			removeRange(int index, int count);

	// removes the element at the given index (fast)
	bool			fastRemoveIndex(int index);

	// removes the element
	bool			remove(const T& obj);

	// removes the element
	bool			fastRemove(const T& obj);

	// swap the contents of the lists
	void			swap(ArrayBase<T, STORAGE_TYPE>& other);

	// swap the contents of the lists - raw
	void			swap(T*& other, int& otherNumElem);

	// assure list has given number of elements, but leave them uninitialized
	void			assureSize(int newSize);

	// assure list has given number of elements and initialize any new elements
	template<typename... Args>
	void			assureSizeEmplace(int newSize, Args&&... args);

protected:
	void			ensureCapacity(int newElement);

	int				m_nNumElem{ 0 };
};

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase()
{
	static_assert(!std::is_same_v<STORAGE_TYPE, DynamicArrayStorage<T>>, "PP_SL constructor is required to use");
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase(const PPSourceLine& sl, int granularity)
	: STORAGE_TYPE(sl, granularity)
{
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::~ArrayBase()
{
	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr, m_nNumElem);
}

// -----------------------------------------------------------------
// Frees up the memory allocated by the list.  Assumes that T automatically handles freeing up memory.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::clear(bool deallocate)
{
	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr, m_nNumElem);

	if (deallocate)
	{
		STORAGE_TYPE::free();
	}

	m_nNumElem = 0;
}

// -----------------------------------------------------------------
// Returns the number of elements currently contained in the list.
// Note that this is NOT an indication of the memory allocated.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::numElem() const
{
	return m_nNumElem;
}

// -----------------------------------------------------------------
// returns number elements which satisfies to the condition
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename COMPAREFUNC >
inline int ArrayBase<T, STORAGE_TYPE>::numElem(COMPAREFUNC comparator) const
{
	int theCount = 0;
	const T* listPtr = STORAGE_TYPE::getData();

	for (int i = 0; i < m_nNumElem; i++)
	{
		if (comparator(listPtr[i]))
			theCount++;
	}

	return theCount;
}

// -----------------------------------------------------------------
// Returns the number of elements currently allocated for.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::numAllocated() const
{
	return STORAGE_TYPE::getSize();
}

// -----------------------------------------------------------------
// Sets the base size of the array and resizes the array to match.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::setGranularity(int newgranularity)
{
	int newsize;

	ASSERT(newgranularity > 0);

	STORAGE_TYPE::setGranularity(newgranularity);

	resize(STORAGE_TYPE::getSize());
}

// -----------------------------------------------------------------
// Returns the current granularity.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::getGranularity() const
{
	return STORAGE_TYPE::getGranularity();
}

// -----------------------------------------------------------------
// Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::operator[](int index) const
{
	ASSERT_MSG(index >= 0 && index < m_nNumElem, "Array<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_nNumElem);

	return STORAGE_TYPE::getData()[index];
}

// -----------------------------------------------------------------
// Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::operator[](int index)
{
	ASSERT_MSG(index >= 0 && index < m_nNumElem, "Array<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_nNumElem);

	return STORAGE_TYPE::getData()[index];
}

// -----------------------------------------------------------------
// Copies the contents and size attributes of another list.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>& ArrayBase<T, STORAGE_TYPE>::operator=(const ArrayBase<T, STORAGE_TYPE>& other)
{
	STORAGE_TYPE::setGranularity(other.STORAGE_TYPE::getGranularity());

	reserve(other.STORAGE_TYPE::getSize());

	m_nNumElem = other.m_nNumElem;

	T* listPtr = STORAGE_TYPE::getData();
	const T* otherListPtr = other.STORAGE_TYPE::getData();

	if (STORAGE_TYPE::getSize())
	{
		for (int i = 0; i < m_nNumElem; i++)
			new(&listPtr[i]) T(otherListPtr[i]);
	}

	return *this;
}



// -----------------------------------------------------------------
// Allocates memory for the amount of elements requested while keeping the contents intact.
// Contents are copied using their = operator so that data is correnctly instantiated.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::resize(int newSize)
{
	STORAGE_TYPE::resize(newSize, m_nNumElem);
}


// -----------------------------------------------------------------
// Reserves memory for the amount of elements requested
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::reserve(int requiredSize)
{
	if (requiredSize > STORAGE_TYPE::getSize())
		STORAGE_TYPE::resize(requiredSize, m_nNumElem);
}

// -----------------------------------------------------------------
// Resize to the exact size specified irregardless of granularity
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::setNum(int newnum, bool shrinkResize)
{
	ASSERT(newnum >= 0);

	if (shrinkResize || newnum > STORAGE_TYPE::getSize())
		resize(newnum);

	// initialize new elements
	T* listPtr = STORAGE_TYPE::getData();
	for (int i = m_nNumElem; i < newnum; ++i)
	{
		PPSLPlacementNew<T>(&listPtr[i], STORAGE_TYPE::getSL());
	}

	m_nNumElem = newnum;
}

// -----------------------------------------------------------------
// Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.
// Note: may return NULL if the list is empty.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline T* ArrayBase<T, STORAGE_TYPE>::ptr()
{
	return STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.
// Note: may return NULL if the list is empty.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T* ArrayBase<T, STORAGE_TYPE>::ptr() const
{
	return STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns front item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::front()
{
	ASSERT(m_nNumElem > 0);
	return *STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns front item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::front() const
{
	ASSERT(m_nNumElem > 0);
	return *STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::back()
{
	ASSERT(m_nNumElem > 0);
	return STORAGE_TYPE::getData()[m_nNumElem - 1];
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::back() const
{
	ASSERT(m_nNumElem > 0);
	return STORAGE_TYPE::getData()[m_nNumElem - 1];
}

// -----------------------------------------------------------------
// Removes front item from list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::popFront()
{
	ASSERT(m_nNumElem > 0);
	T item = STORAGE_TYPE::getData()[0];
	removeIndex(0);
	return item;
}

// -----------------------------------------------------------------
// Removes back item from list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::popBack()
{
	ASSERT(m_nNumElem > 0);
	T* listPtr = STORAGE_TYPE::getData();
	T item = listPtr[m_nNumElem - 1];
	ArrayStorageBase<T>::destructElements(listPtr + m_nNumElem - 1, 1);
	m_nNumElem--;
	return item;
}

template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::ensureCapacity(int newElements)
{
	resize(m_nNumElem + newElements);
}

// -----------------------------------------------------------------
// Increases the size of the list by one element and copies the supplied data into it.
// Returns the index of the new element.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(const T& obj)
{
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	new(&listPtr[m_nNumElem]) T(obj);
	m_nNumElem++;

	return m_nNumElem - 1;
}

// -----------------------------------------------------------------
// Increases the size of the list by one element and copies the supplied data into it.
// Returns the index of the new element.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(T&& obj)
{
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	new(&listPtr[m_nNumElem]) T(std::move(obj));
	m_nNumElem++;

	return m_nNumElem - 1;
}

// -----------------------------------------------------------------
// Increases the size of the list by one element and copies the supplied data into it.
// Returns the index of the new element.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template<typename... Args>
inline int ArrayBase<T, STORAGE_TYPE>::appendEmplace(Args&&... args)
{
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	new(&listPtr[m_nNumElem]) T(std::forward<Args>(args)...);
	m_nNumElem++;

	return m_nNumElem - 1;
}

// -----------------------------------------------------------------
// append a new empty element to be filled
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::append()
{
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	T& newItem = listPtr[m_nNumElem];
	m_nNumElem++;
	
	PPSLPlacementNew<T>(&newItem, STORAGE_TYPE::getSL());

	return newItem;
}

// -----------------------------------------------------------------
// adds the other list to this one
// Returns the size of the new combined list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(const T* other, int count)
{
	if(!count)
		return numElem();

	ensureCapacity(count);

	// append the elements
	for (int i = 0; i < count; i++)
		append(other[i]);

	return numElem();
}

// -----------------------------------------------------------------
// adds the other list to this one
// Returns the size of the new combined list
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename CT, typename OTHER_STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(const ArrayBase<CT, OTHER_STORAGE_TYPE>& other)
{
	const int count = other.numElem();
	if (!count)
		return numElem();

	ensureCapacity(count);

	// append the elements
	for (int i = 0; i < count; i++)
		append(other[i]);

	return numElem();
}

// -----------------------------------------------------------------
// appends other list with transformation
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename T2, typename OTHER_STORAGE_TYPE, typename TRANSFORMFUNC>
inline int ArrayBase<T, STORAGE_TYPE>::append(const ArrayBase<T2, OTHER_STORAGE_TYPE>& other, TRANSFORMFUNC transform)
{
	const int count = other.numElem();
	if (!count)
		return numElem();

	ensureCapacity(count);

	// try transform and append the elements
	for (int i = 0; i < count; i++)
	{
		T newObj;
		if (transform(newObj, other[i]))
			append(other[i]);
	}


	return numElem();
}


// -----------------------------------------------------------------
// Increases the elemCount of the list by at least one element if necessary
// and inserts the supplied data into it.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::insert(T const& obj, int index)
{
	ensureCapacity(1);

	if (index < 0)
		index = 0;
	else if (index > m_nNumElem)
		index = m_nNumElem;

	T* listPtr = STORAGE_TYPE::getData();

	for (int i = m_nNumElem; i > index; --i)
	{
		new(&listPtr[i]) T(std::move(listPtr[i - 1]));
	}

	m_nNumElem++;
	new(&listPtr[index]) T(obj);

	return index;
}

// -----------------------------------------------------------------
// Increases the elemCount of the list by at least one element if necessary
// and inserts the supplied data into it.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::insert(T&& obj, int index)
{
	ensureCapacity(1);

	if (index < 0)
		index = 0;
	else if (index > m_nNumElem)
		index = m_nNumElem;

	T* listPtr = STORAGE_TYPE::getData();

	for (int i = m_nNumElem; i > index; --i)
	{
		new(&listPtr[i]) T(std::move(listPtr[i - 1]));
	}

	m_nNumElem++;
	new(&listPtr[index]) T(std::move(obj));

	return index;
}

// -----------------------------------------------------------------
// Increases the elemCount of the list by at least one element if necessary
// and returns new element
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::insert(int index)
{
	ensureCapacity(1);

	if (index < 0)
		index = 0;
	else if (index > m_nNumElem)
		index = m_nNumElem;

	T* listPtr = STORAGE_TYPE::getData();

	for (int i = m_nNumElem; i > index; --i)
	{
		new(&listPtr[i]) T(std::move(listPtr[i - 1]));
	}

	m_nNumElem++;

	PPSLPlacementNew<T>(&listPtr[index], STORAGE_TYPE::getSL());

	return listPtr[index];
}

// -----------------------------------------------------------------
// Adds the data to the m_pListPtr if it doesn't already exist.  Returns the index of the data in the m_pListPtr.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique(T const& obj)
{
	int index;

	index = arrayFindIndex(*this, obj);

	if (index < 0)
		index = append(obj);

	return index;
}


// -----------------------------------------------------------------
// Adds the data to the m_pListPtr if it doesn't already exist.  Returns the index of the data in the m_pListPtr.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename PAIRCOMPAREFUNC >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique(T const& obj, PAIRCOMPAREFUNC comparator)
{
	int index;

	index = arrayFindIndexF(*this, obj, comparator);

	if (index < 0)
		index = append(obj);

	return index;
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_pListPtr and returns iterator.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline typename ArrayBase<T, STORAGE_TYPE>::Iterator ArrayBase<T, STORAGE_TYPE>::find(T const& obj) const
{
	const int index = arrayFindIndex(*this, obj);
	return index == -1 ? end() : Iterator(*const_cast<SelfType*>(this), index);
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_pListPtr and returns iterator.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename PAIRCOMPAREFUNC >
inline typename ArrayBase<T, STORAGE_TYPE>::Iterator ArrayBase<T, STORAGE_TYPE>::find(T const& obj, PAIRCOMPAREFUNC comparator) const
{
	const int index = arrayFindIndexF(*this, obj, comparator);
	return index == -1 ? end() : Iterator(*const_cast<SelfType*>(this), index);
}

// -----------------------------------------------------------------
// Returns first element which satisfies to the condition
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename COMPAREFUNC >
inline typename ArrayBase<T, STORAGE_TYPE>::Iterator ArrayBase<T, STORAGE_TYPE>::find(COMPAREFUNC comparator) const
{
	const int index = arrayFindIndexF(*this, comparator);
	return index == -1 ? end() : Iterator(*const_cast<SelfType*>(this), index);
}

// -----------------------------------------------------------------
// Removes the element at the specified index and moves all data following the element down to fill in the gap.
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the index is outside the bounds of the m_pListPtr.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::removeIndex(int index)
{
	ASSERT(index >= 0);
	ASSERT(index < m_nNumElem);

	if ((index < 0) || (index >= m_nNumElem))
		return false;

	m_nNumElem--;

	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr + index, 1);

	for (int i = index; i < m_nNumElem; i++)
		listPtr[i] = std::move(listPtr[i + 1]);

	return true;
}

// -----------------------------------------------------------------
// Removes specified count of elements from specified index and moves all data following the element down to fill in the gap.
// The number of elements in the m_pListPtr is reduced by count.  Returns false if the index + count is outside the bounds of the m_pListPtr.
// Note that the elements are not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::removeRange(int index, int count)
{
	ASSERT(count >= 0);
	ASSERT(index >= 0);

	if ((index < 0) || (index + count >= m_nNumElem))
		return false;

	if (!count)
		return true;

	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr + index, count);

	m_nNumElem -= count;

	for (int i = index; i < m_nNumElem; i++)
		listPtr[i] = std::move(listPtr[i + count]);

	return true;
}

// -----------------------------------------------------------------
// Removes the element at the specified index, without data movement (only last element will be placed at the removed element's position)
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the index is outside the bounds of the m_pListPtr.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemoveIndex(int index)
{
	ASSERT(index >= 0);
	ASSERT(index < m_nNumElem);

	if ((index < 0) || (index >= m_nNumElem))
		return false;

	m_nNumElem--;

	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr + index, 1);

	if (m_nNumElem > 0)
		new(&listPtr[index]) T(std::move(listPtr[m_nNumElem]));

	return true;
}


// -----------------------------------------------------------------
// Removes the element if it is found within the m_pListPtr and moves all data following the element down to fill in the gap.
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the data is not found in the m_pListPtr.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::remove(T const& obj)
{
	int index;

	index = arrayFindIndex(*this, obj);

	if (index >= 0)
		return removeIndex(index);

	return false;
}


// -----------------------------------------------------------------
// Removes the element if it is found within the m_pListPtr without data movement (only last element will be placed at the removed element's position)
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the data is not found in the m_pListPtr.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemove(T const& obj)
{
	int index;

	index = arrayFindIndex(*this, obj);

	if (index >= 0)
		return fastRemoveIndex(index);

	return false;
}

// -----------------------------------------------------------------
// Returns true if index is in array range
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::inRange(int index) const
{
	return index >= 0 && index < m_nNumElem;
}

// -----------------------------------------------------------------
// Swaps the contents of two lists
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(ArrayBase<T, STORAGE_TYPE>& other)
{
	QuickSwap(m_nNumElem, other.m_nNumElem);
	STORAGE_TYPE::swap(other);
}

// -----------------------------------------------------------------
// swap the contents of the lists - raw
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(T*& other, int& otherNumElem)
{
	QuickSwap(m_nNumElem, otherNumElem);
	STORAGE_TYPE::swap(other, otherNumElem);
}

// -----------------------------------------------------------------
// Makes sure the list has at least the given number of elements.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::assureSize(int newSize)
{
	m_nNumElem = newSize;
	ensureCapacity(0);
}

// -----------------------------------------------------------------
// Makes sure the listPtr has at least the given number of elements and initialize any elements not yet initialized.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template<typename... Args>
inline void ArrayBase<T, STORAGE_TYPE>::assureSizeEmplace(int newSize, Args&&... args)
{
	const int oldSize = m_nNumElem;
	m_nNumElem = newSize;

	ensureCapacity(0);

	T* listPtr = STORAGE_TYPE::getData();
	for (int i = oldSize; i < newSize; i++)
		new(&listPtr[i]) T(std::forward<Args>(args)...);
}

//-------------------------------------------

template< typename T >
using Array = ArrayBase<T, DynamicArrayStorage<T>>;

template< typename T, int SIZE >
using FixedArray = ArrayBase<T, FixedArrayStorage<T, SIZE>>;

//-------------------------------------------

// non-const array ref
template< typename T >
class ArrayRef
{
	using SelfType = ArrayRef<T>;
public:
	using ITEM = T;

	struct Iterator
	{
		SelfType& array;
		int index = 0;

		Iterator(SelfType& array, int from)
			: array(array), index(from)
		{}

		bool	operator==(Iterator& it) const { return it.index == index; }
		bool	operator!=(Iterator& it) const { return it.index != index; }

		bool	atEnd() const { return index >= array.numElem(); }
		T&		operator*() { return array[index]; }
		void	operator++() { ++index; }
	};

	ArrayRef(std::nullptr_t) {}

	ArrayRef(T* elemPtr, int numElem)
		: m_pListPtr(elemPtr), m_nNumElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayRef(ARRAY_TYPE& otherArray)
		: m_pListPtr(otherArray.ptr()), m_nNumElem(otherArray.numElem())
	{
	}

	template<int N>
	ArrayRef(T(&otherArray)[N])
		: m_pListPtr(otherArray), m_nNumElem(N)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayRef<T>& operator=(const ARRAY_TYPE& other)
	{
		m_pListPtr = other.ptr();
		m_nNumElem = other.numElem();
		return *this;
	}

	ArrayRef<T>& operator=(const ArrayRef<T>& other)
	{
		m_pListPtr = other.ptr();
		m_nNumElem = other.numElem();
		return *this;
	}

	T& operator[](int index)
	{
		ASSERT(index >= 0);
		ASSERT(index < m_nNumElem);

		return m_pListPtr[index];
	}

	Iterator		begin() const { return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const { return Iterator(*const_cast<SelfType*>(this), m_nNumElem); }

	// returns number of elements in list
	int				numElem() const { return m_nNumElem; }

	// returns a pointer to the list
	T*				ptr() { return m_pListPtr; }

	// returns true if index is in range
	bool			inRange(int index) const { return index >= 0 && index < m_nNumElem; }

	// returns front item
	T&				front() { ASSERT(m_nNumElem > 0); return *m_pListPtr; }

	// returns front item
	const T&		front() const { ASSERT(m_nNumElem > 0); return *m_pListPtr; }

	// returns back item
	T&				back() { ASSERT(m_nNumElem > 0); return *m_pListPtr[m_nNumElem - 1]; }

	// returns back item
	const T&		back() const { ASSERT(m_nNumElem > 0); return *m_pListPtr[m_nNumElem - 1]; }

protected:
	T*				m_pListPtr{ nullptr };
	int				m_nNumElem{ 0 };
};

// const array ref
template< typename T >
class ArrayCRef
{
	using SelfType = ArrayCRef<T>;
public:
	using ITEM = T;

	struct Iterator
	{
		SelfType& array;
		int index = 0;

		Iterator(SelfType& array, int from)
			: array(array), index(from)
		{}

		bool		operator==(Iterator& it) const { return it.index == index; }
		bool		operator!=(Iterator& it) const { return it.index != index; }

		bool		atEnd() const { return index >= array.numElem(); }
		const T&	operator*() const { return array[index]; }
		void		operator++() { ++index; }
	};

	ArrayCRef(std::nullptr_t) {}

	ArrayCRef(const T* elemPtr, int numElem)
		: m_pListPtr(elemPtr), m_nNumElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayCRef(const ARRAY_TYPE& otherArray)
		: m_pListPtr(otherArray.ptr()), m_nNumElem(otherArray.numElem())
	{
	}

	template<int N>
	ArrayCRef(T(&otherArray)[N])
		: m_pListPtr(otherArray), m_nNumElem(N)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayCRef<T>& operator=(const ARRAY_TYPE& other)
	{
		m_pListPtr = other.ptr();
		m_nNumElem = other.numElem();
		return *this;
	}

	ArrayCRef<T>& operator=(const ArrayRef<T>& other)
	{
		m_pListPtr = other.ptr();
		m_nNumElem = other.numElem();
		return *this;
	}

	ArrayCRef<T>& operator=(const ArrayCRef<T>& other)
	{
		m_pListPtr = other.ptr();
		m_nNumElem = other.numElem();
		return *this;
	}

	const T& operator[](int index) const
	{
		ASSERT(index >= 0);
		ASSERT(index < m_nNumElem);

		return m_pListPtr[index];
	}

	Iterator		begin() const { return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const { return Iterator(*const_cast<SelfType*>(this), m_nNumElem); }

	// returns number of elements in list
	int				numElem() const { return m_nNumElem; }

	// returns a pointer to the list
	const T*		ptr() const { return m_pListPtr; }

	// returns true if index is in range
	bool			inRange(int index) const { return index >= 0 && index < m_nNumElem; }

	// returns front item
	const T&		front() const { ASSERT(m_nNumElem > 0); return *m_pListPtr; }

	// returns back item
	const T&		back() const { ASSERT(m_nNumElem > 0); return *m_pListPtr[m_nNumElem - 1]; }

protected:
	const T*		m_pListPtr{ nullptr };
	int				m_nNumElem{ 0 };
};


//-------------------------------------------
// nesting Source-line constructor helper
template<typename ITEM>
struct PPSLValueCtor<Array<ITEM>>
{
	Array<ITEM> x;
	PPSLValueCtor<Array<ITEM>>(const PPSourceLine& sl) : x(sl) {}
};

