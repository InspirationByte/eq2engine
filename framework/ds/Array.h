//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Dynamic array (vector) of elements
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <typeinfo>	// required for ASSERT messages
#include "ds/rawitem.h"
#include "ds/sort.h"

template< typename T >
class ArrayStorageBase
{
public:
	/*
	Any array storage class must be implemented with those methods:

	void				free();
	void				resize(int newSize, int& numOfElements);
	void				swap(ArrayStorageBase& other);
	void 				swap(T*& other, int& otherNumElem);

	int					getSize() const;
	int					getGranularity() const;
	void				setGranularity(int newGranularity);

	T*					getData();
	const T*			getData() const;
	const PPSourceLine	getSL() const;
	*/
	static void	destructElements(T* dest, int count);
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

//--------------------------------------------------------

template< typename T >
class DynamicArrayStorage
{
public:
	DynamicArrayStorage(const PPSourceLine& sl, int granularity)
		: m_sl(sl)
		, m_granularity(granularity)
	{
		ASSERT(m_granularity > 0); 
	}

	DynamicArrayStorage(const DynamicArrayStorage& other)
		: m_sl(other.m_sl)
		, m_granularity(other.m_granularity)
	{
		ASSERT(m_granularity > 0); 

		int numElem = 0;
		resize(other.m_size, numElem);
	}

	DynamicArrayStorage(DynamicArrayStorage&& other)
		: m_sl(other.m_sl)
		, m_granularity(other.m_granularity)
	{
		ASSERT(m_granularity > 0); 

		m_data = other.m_data;
		m_size = other.m_size;
		other.m_data = nullptr;
		other.m_size = 0;
	}

	~DynamicArrayStorage()
	{
		delete[] reinterpret_cast<ubyte*>(m_data);
	}

	void free()
	{
		delete[] reinterpret_cast<ubyte*>(m_data);
		m_data = nullptr;
		m_size = 0;
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

		const int newCapacityWithGranularity = (newCapacity + m_granularity - 1) / m_granularity * m_granularity;
		if (newCapacityWithGranularity == m_size)
			return;

		m_size = newCapacityWithGranularity;
		if (m_size < numOfElements)
			numOfElements = m_size;

		m_data = reinterpret_cast<T*>(PPDReAlloc(m_data, m_size * sizeof(T), m_sl));
	}

	void swap(DynamicArrayStorage& other)
	{
		QuickSwap(m_size, other.m_size);
		QuickSwap(m_granularity, other.m_granularity);
		QuickSwap(m_data, other.m_data);
	}

	void swap(T*& other, int& otherNumElem)
	{
		QuickSwap(m_data, other);
		m_size = otherNumElem;
	}

	int					getSize() const						{ return m_size; }
	int					getGranularity() const				{ return m_granularity;}
	void				setGranularity(int newGranularity)	{ ASSERT(newGranularity > 0); m_granularity = newGranularity; }

	T*					getData()							{ return m_data; }
	const T*			getData() const						{ return m_data; }
	const PPSourceLine	getSL() const						{ return m_sl; }

protected:
	const PPSourceLine	m_sl;
	T* 					m_data{ nullptr };

	int					m_size{ 0 };
	int					m_granularity{ 16 };
};

//--------------------------------------------------------

template< typename T, int SIZE >
class FixedArrayStorage
{
public:
	FixedArrayStorage() = default;
	FixedArrayStorage(const FixedArrayStorage& other) = default;

	FixedArrayStorage(FixedArrayStorage&& other)
	{
		memmove(m_data, other.m_data, SIZE * sizeof(T));
	}

	void free() {}

	void resize(int newSize, int& numOfElements)
	{
		ASSERT_MSG(newSize <= SIZE, "FixedArrayStorage<%s> capacity is %d, required %d", typeid(T).name(), getSize(), newSize);
	}

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

	int					getSize() const						{ return SIZE; }
	int					getGranularity() const				{ return 1; }
	void				setGranularity(int newGranularity)	{}

	T*					getData()							{ return (T*)(&m_data[0]); }
	const T*			getData() const						{ return (T*)(&m_data[0]); }
	const PPSourceLine	getSL() const 						{ return PPSourceLine::Empty(); }
protected:
	RawItem<T, alignof(T)>		m_data[SIZE];
	static_assert(sizeof(T) * SIZE == sizeof(m_data));
};

//--------------------------------------------------------

template< typename T, int SIZE >
class BufferedArrayStorage : FixedArrayStorage<T, SIZE>
{
	using FixedStorage = FixedArrayStorage<T, SIZE>;
public:
	BufferedArrayStorage(const PPSourceLine& sl, int granularity)
		: m_dynamicStorage(sl, granularity)
	{
	}

	BufferedArrayStorage(const BufferedArrayStorage& other)
		: m_dynamicStorage(other.m_dynamicStorage)
	{
	}

	BufferedArrayStorage(BufferedArrayStorage&& other)
		: m_dynamicStorage(std::move(other.m_dynamicStorage))
	{
		memmove(FixedStorage::m_data, other.m_data, SIZE * sizeof(T));
	}

	void free()
	{
		m_dynamicStorage.free();
	}

	void resize(int newCapacity, int& numOfElements)
	{
		const bool wasFixedStorage = m_dynamicStorage.m_data == nullptr;
		if(newCapacity > SIZE)
		{
			m_dynamicStorage.resize(newCapacity, numOfElements);
			
			// copy data from fixed storage
			if(!wasFixedStorage)
				memmove(m_dynamicStorage.m_data, FixedStorage::m_data, SIZE * sizeof(T));
		}
	}

	void swap(BufferedArrayStorage& other)
	{
		FixedStorage::swap(other);
		m_dynamicStorage.swap(other.m_dynamicStorage);
	}

	void swap(T*& other, int& otherNumElem)
	{
		ASSERT_FAIL("BufferedArrayStorage: swap unsupported");
	}

	int					getSize() const						{ return max(SIZE, m_dynamicStorage.m_size); }
	int					getGranularity() const				{ return m_dynamicStorage.m_granularity;}
	void				setGranularity(int newGranularity)	{ m_dynamicStorage.m_granularity = newGranularity; }

	T*					getData()							{ return m_dynamicStorage.m_data ? m_dynamicStorage.m_data : FixedStorage::m_data; }
	const T*			getData() const						{ return m_dynamicStorage.m_data ? m_dynamicStorage.m_data : FixedStorage::m_data; }
	const PPSourceLine	getSL() const 						{ return m_dynamicStorage.m_sl; }

	DynamicArrayStorage<T>&			getDynamicStorage()			{ return m_dynamicStorage; }
	const DynamicArrayStorage<T>&	getDynamicStorage() const	{ return m_dynamicStorage; }

protected:
	DynamicArrayStorage<T>		m_dynamicStorage;
};

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
	ArrayBase(const ArrayBase& other);
	ArrayBase(ArrayBase&& other);
	ArrayBase(std::initializer_list<T> list, const PPSourceLine& sl, int granularity = 16);

	~ArrayBase<T, STORAGE_TYPE>();

	const T&		operator[](int index) const;
	T&				operator[](int index);

	SelfType&		operator=(const SelfType& other);

	Iterator		begin() const	{ return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const		{ return Iterator(*const_cast<SelfType*>(this), m_numElem); }

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

	// returns true if empty
	bool			isEmpty() const { return numElem() == 0; }

	// returns true if element count matches allocated count. Use on fixed arrays
	bool			isFull() const { return numElem() == numAllocated(); }

	// sets new granularity
	void			setGranularity(int newgranularity);

	// get the current granularity
	int				getGranularity() const;

	// resizes the list
	void			resize(int newsize);

	// reserve capacity
	void			reserve(int requiredSize);

	// sets the number of elements in list and resize to exactly this number if necessary
	void			setNum(int newnum, bool shrinkResize = true);

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

	// removes front item from list unsorted
	T				fastPopFront();

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
	template< typename CMP >
	int				addUnique(const T& obj, CMP comparator);

	// returns iterator for the given element
	Iterator		find(const T& obj) const;

	// returns iterator for the given element
	template< typename CMP >
	Iterator		find(const T& obj, CMP comparator) const;

	// returns first found element which satisfies to the condition
	template< typename COMPAREFUNC >
	Iterator		find(COMPAREFUNC comparator) const;

	// removes the element at the given index
	bool			removeIndex(int index);

	// removes specified count of elements from specified index
	bool			removeRange(int index, int count);

	// removes the element at the given index unsorted
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
	// ensures that the array is allocated to the capacity of numElem() + newElementCount
	void			ensureCapacity(int newElementCount);

	int				m_numElem{ 0 };
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
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase(const ArrayBase& other)
	: STORAGE_TYPE(other)
{
	m_numElem = other.m_numElem;

	T* listPtr = STORAGE_TYPE::getData();
	const T* otherListPtr = other.STORAGE_TYPE::getData();

	if (STORAGE_TYPE::getSize())
	{
		for (int i = 0; i < m_numElem; i++)
			new(&listPtr[i]) T(otherListPtr[i]);
	}
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase(ArrayBase&& other)
	: STORAGE_TYPE(std::move(other))
{
	m_numElem = other.m_numElem;
	other.m_numElem = 0;
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase(std::initializer_list<T> list, const PPSourceLine& sl, int granularity)
	: STORAGE_TYPE(sl, granularity)
{
	reserve(static_cast<int>(list.size()));
	for (auto& item : list)
		append(item);
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::~ArrayBase()
{
	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr, m_numElem);
}

// -----------------------------------------------------------------
// Frees up the memory allocated by the list.  Assumes that T automatically handles freeing up memory.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::clear(bool deallocate)
{
	const int oldNumElem = m_numElem;
	m_numElem = 0;

	T* listPtr = STORAGE_TYPE::getData();
	ArrayStorageBase<T>::destructElements(listPtr, oldNumElem);

	if (deallocate)
		STORAGE_TYPE::free();
}

// -----------------------------------------------------------------
// Returns the number of elements currently contained in the list.
// Note that this is NOT an indication of the memory allocated.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::numElem() const
{
	return m_numElem;
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

	for (int i = 0; i < m_numElem; i++)
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
	ASSERT_MSG(inRange(index), "Array<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_numElem);

	return STORAGE_TYPE::getData()[index];
}

// -----------------------------------------------------------------
// Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::operator[](int index)
{
	ASSERT_MSG(inRange(index), "Array<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_numElem);

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

	m_numElem = other.m_numElem;

	T* listPtr = STORAGE_TYPE::getData();
	const T* otherListPtr = other.STORAGE_TYPE::getData();

	if (STORAGE_TYPE::getSize())
	{
		for (int i = 0; i < m_numElem; i++)
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
	if (newSize < m_numElem)
	{
		T* listPtr = STORAGE_TYPE::getData();
		ArrayStorageBase<T>::destructElements(listPtr + newSize, m_numElem - newSize);
	}

	STORAGE_TYPE::resize(newSize, m_numElem);
}

// -----------------------------------------------------------------
// Reserves memory for the amount of elements requested
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::reserve(int requiredSize)
{
	if (requiredSize > STORAGE_TYPE::getSize())
		STORAGE_TYPE::resize(requiredSize, m_numElem);
}

// -----------------------------------------------------------------
// Resize to the exact size specified irregardless of granularity
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::setNum(int newNum, bool shrinkResize)
{
	ASSERT(newNum >= 0);

	if (newNum < m_numElem)
	{
		T* listPtr = STORAGE_TYPE::getData();
		ArrayStorageBase<T>::destructElements(listPtr + newNum, m_numElem - newNum);
	}
	else
	{
		if (shrinkResize || newNum > STORAGE_TYPE::getSize())
			resize(newNum);

		// initialize new elements
		T* listPtr = STORAGE_TYPE::getData();
		for (int i = m_numElem; i < newNum; ++i)
		{
			PPSLPlacementNew<T>(std::addressof(listPtr[i]), STORAGE_TYPE::getSL());
		}
	}

	m_numElem = newNum;
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
	ASSERT(m_numElem > 0);

	return *STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns front item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::front() const
{
	ASSERT(m_numElem > 0);

	return *STORAGE_TYPE::getData();
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::back()
{
	ASSERT(m_numElem > 0);

	return STORAGE_TYPE::getData()[m_numElem - 1];
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::back() const
{
	ASSERT(m_numElem > 0);

	return STORAGE_TYPE::getData()[m_numElem - 1];
}

// -----------------------------------------------------------------
// Removes front item from list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::popFront()
{
	ASSERT(m_numElem > 0);

	T item = STORAGE_TYPE::getData()[0];
	removeIndex(0);
	return item;
}

// -----------------------------------------------------------------
// Removes front item from list unsorted
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::fastPopFront()
{
	ASSERT(m_numElem > 0);

	T item = STORAGE_TYPE::getData()[0];
	fastRemoveIndex(0);
	return item;
}

// -----------------------------------------------------------------
// Removes back item from list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::popBack()
{
	ASSERT(m_numElem > 0);
	T* listPtr = STORAGE_TYPE::getData();

	const int lastItemIdx = --m_numElem;

	// copy item
	T item = listPtr[lastItemIdx];
	listPtr[lastItemIdx].~T();

	return item;
}

template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::ensureCapacity(int newElementCount)
{
	reserve(m_numElem + newElementCount);
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

	const int index = m_numElem++;
	new(&listPtr[index]) T(obj);
	return index;
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

	const int index = m_numElem++;
	new(&listPtr[index]) T(std::move(obj));
	return index;
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

	const int index = m_numElem++;
	new(&listPtr[index]) T{ std::forward<Args>(args)... };
	return index;
}

// -----------------------------------------------------------------
// append a new empty element to be filled
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::append()
{
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	T& newItem = listPtr[m_numElem++];
	PPSLPlacementNew<T>(std::addressof(newItem), STORAGE_TYPE::getSL());

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
			append(newObj);
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
	ASSERT(index >= 0);
	ASSERT(index <= m_numElem);
	
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	T* dstPtr = listPtr + index;
	const int rest = m_numElem - index;
	if(rest > 0)
		memmove(dstPtr + 1, dstPtr, rest * sizeof(T));

	++m_numElem;

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
	ASSERT(index >= 0);
	ASSERT(index <= m_numElem);
	
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	T* dstPtr = listPtr + index;
	const int rest = m_numElem - index;
	if(rest > 0)
		memmove(dstPtr + 1, dstPtr, rest * sizeof(T));

	++m_numElem;

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
	ASSERT(index >= 0);
	ASSERT(index <= m_numElem);
	
	ensureCapacity(1);

	T* listPtr = STORAGE_TYPE::getData();

	T* dstPtr = listPtr + index;
	const int rest = m_numElem - index;
	if(rest > 0)
		memmove(dstPtr + 1, dstPtr, rest * sizeof(T));

	++m_numElem;

	PPSLPlacementNew<T>(std::addressof(listPtr[index]), STORAGE_TYPE::getSL());

	return listPtr[index];
}

// -----------------------------------------------------------------
// Adds the data to the m_data if it doesn't already exist.  Returns the index of the data in the m_data.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique(T const& obj)
{
	int index = arrayFindIndex(*this, obj);
	if (index < 0)
		index = append(obj);

	return index;
}


// -----------------------------------------------------------------
// Adds the data to the m_data if it doesn't already exist.  Returns the index of the data in the m_data.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename CMP >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique(T const& obj, CMP comparator)
{
	int index = arrayFindIndexF(*this, obj, comparator);
	if (index < 0)
		index = append(obj);

	return index;
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_data and returns iterator.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline typename ArrayBase<T, STORAGE_TYPE>::Iterator ArrayBase<T, STORAGE_TYPE>::find(T const& obj) const
{
	const int index = arrayFindIndex(*this, obj);
	return index == -1 ? end() : Iterator(*const_cast<SelfType*>(this), index);
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_data and returns iterator.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename CMP >
inline typename ArrayBase<T, STORAGE_TYPE>::Iterator ArrayBase<T, STORAGE_TYPE>::find(T const& obj, CMP comparator) const
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
// The number of elements in the m_data is reduced by one.  Returns false if the index is outside the bounds of the m_data.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_data.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::removeIndex(int index)
{
	return removeRange(index, 1);
}

// -----------------------------------------------------------------
// Removes specified count of elements from specified index and moves all data following the element down to fill in the gap.
// The number of elements in the m_data is reduced by count. Returns false if no items has been removed
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::removeRange(int index, int count)
{
	ASSERT(index >= 0);
	ASSERT(index + count <= m_numElem);

	count = min(index + count, m_numElem) - index;
	if (index >= m_numElem || count <= 0)
		return false;

	T* listPtr = STORAGE_TYPE::getData();

	ArrayStorageBase<T>::destructElements(listPtr + index, count);

	const int last = index + count;
	const int rest = m_numElem - last;
	if(rest > 0)
		memmove(&listPtr[index], &listPtr[last], rest * sizeof(T));

	m_numElem -= count;
	return true;
}

// -----------------------------------------------------------------
// Removes the element at the specified index, without data movement (only last element will be placed at the removed element's position)
// The number of elements in the m_data is reduced by one.  Returns false if the index is outside the bounds of the m_data.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_data.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemoveIndex(int index)
{
	ASSERT(index >= 0);
	ASSERT(index < m_numElem);

	if (index < 0 || index >= m_numElem)
		return false;

	T* listPtr = STORAGE_TYPE::getData();
	listPtr[index].~T();

	const int lastItemIdx = --m_numElem;
	if (lastItemIdx > 0)
		memcpy(&listPtr[index], &listPtr[lastItemIdx], sizeof(T));

	return true;
}


// -----------------------------------------------------------------
// Removes the element if it is found within the m_data and moves all data following the element down to fill in the gap.
// The number of elements in the m_data is reduced by one.  Returns false if the data is not found in the m_data.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_data.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::remove(T const& obj)
{
	const int  index = arrayFindIndex(*this, obj);
	if (index < 0)
		return false;
	return removeIndex(index);
}


// -----------------------------------------------------------------
// Removes the element if it is found within the m_data without data movement (only last element will be placed at the removed element's position)
// The number of elements in the m_data is reduced by one.  Returns false if the data is not found in the m_data.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_data.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemove(T const& obj)
{
	const int  index = arrayFindIndex(*this, obj);
	if (index < 0)
		return false;
	return fastRemoveIndex(index);
}

// -----------------------------------------------------------------
// Returns true if index is in array range
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::inRange(int index) const
{
	return index >= 0 && index < m_numElem;
}

// -----------------------------------------------------------------
// Swaps the contents of two lists
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(ArrayBase<T, STORAGE_TYPE>& other)
{
	QuickSwap(m_numElem, other.m_numElem);
	STORAGE_TYPE::swap(other);
}

// -----------------------------------------------------------------
// swap the contents of the lists - raw
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(T*& other, int& otherNumElem)
{
	QuickSwap(m_numElem, otherNumElem);
	STORAGE_TYPE::swap(other, otherNumElem);
}

// -----------------------------------------------------------------
// Makes sure the list has at least the given number of elements.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::assureSize(int newSize)
{
	m_numElem = newSize;
	ensureCapacity(0);
}

// -----------------------------------------------------------------
// Makes sure the listPtr has at least the given number of elements and initialize any elements not yet initialized.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template<typename... Args>
inline void ArrayBase<T, STORAGE_TYPE>::assureSizeEmplace(int newSize, Args&&... args)
{
	const int oldSize = m_numElem;
	m_numElem = newSize;

	ensureCapacity(0);

	T* listPtr = STORAGE_TYPE::getData();
	for (int i = oldSize; i < newSize; i++)
		new(&listPtr[i]) T{ std::forward<Args>(args)... };
}

//-------------------------------------------

template< typename T >
using Array = ArrayBase<T, DynamicArrayStorage<T>>;

template< typename T, int SIZE >
using FixedArray = ArrayBase<T, FixedArrayStorage<T, SIZE>>;

template< typename T, int SIZE >
using BufferedArray = ArrayBase<T, BufferedArrayStorage<T, SIZE>>;

//-------------------------------------------

template<typename T>
struct PPSLValueCtor;

//-------------------------------------------
// nesting Source-line constructor helper
template<typename ITEM>
struct PPSLValueCtor<Array<ITEM>>
{
	Array<ITEM> x;
	PPSLValueCtor<Array<ITEM>>(const PPSourceLine& sl) : x(sl) {}
};

