//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Dynamic array (vector) of elements
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define USE_QSORT

template< typename T >
using PairCompareFunc = bool (*)(const T& a, const T& b);

template< typename T >
using PairSortCompareFunc = int (*)(const T& a, const T& b);

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
		delete [] reinterpret_cast<ubyte*>(m_pListPtr);
	}

	void free()
	{
		delete [] reinterpret_cast<ubyte*>(m_pListPtr);
		m_pListPtr = nullptr;
		m_nSize = 0;
	}

	void resize(int newSize, int& numOfElements)
	{
		ASSERT(newSize >= 0);

		// free up the listPtr if no data is being reserved
		if (newSize <= 0)
		{
			free();
			return;
		}

		// not changing the elemCount, so just exit
		if (newSize == m_nSize)
			return;

		T* temp = m_pListPtr;
		m_nSize = newSize;

		const int oldNumOfElems = numOfElements;
		if (m_nSize < numOfElements)
			numOfElements = m_nSize;

		// copy the old m_pListPtr into our new one
		m_pListPtr = reinterpret_cast<T*>(PPNewSL(m_sl) ubyte[m_nSize * sizeof(T)]);

		if (temp)
		{
			for (int i = 0; i < numOfElements; i++)
				new(&m_pListPtr[i]) T(std::move(temp[i]));

			//ArrayStorageBase<T>::destructElements(temp, oldNumOfElems);

			// delete the old m_pListPtr if it exists
			PPFree(temp);
		}
	}

	int getSize() const
	{
		return m_nSize;
	}

	int getGranularity() const
	{
		return m_nGranularity;
	}

	void setGranularity(int newGranularity)
	{
		m_nGranularity = newGranularity;
	}

	T* getData()
	{
		return m_pListPtr;
	}

	const T* getData() const
	{
		return m_pListPtr;
	}

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
protected:
	const PPSourceLine	m_sl;
	T*					m_pListPtr{ nullptr };

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
		ASSERT_MSG(newSize <= SIZE, "Exceeded FixedArrayStorage size (%d, requires %d)", getSize(), newSize);
	}

	int getSize() const
	{
		return SIZE;
	}

	int getGranularity() const 
	{
		return 1; 
	}

	void setGranularity(int newGranularity) {}

	T* getData()
	{
		return (T*)(&m_data[0]);
	}
	
	const T* getData() const
	{
		return (T*)(&m_data[0]);
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

template< typename T, typename STORAGE_TYPE >
class ArrayBase
{
public:
	ArrayBase();
	ArrayBase(const PPSourceLine& sl, int granularity = 16 );

	~ArrayBase<T, STORAGE_TYPE>();

	const T&					operator[]( int index ) const;
	T&							operator[]( int index );
	ArrayBase<T, STORAGE_TYPE>&	operator=( const ArrayBase<T, STORAGE_TYPE>&other );

	// cleans list
	void			clear( bool deallocate = false );

	// returns true if index is in range
	bool			inRange(int index) const;


	// returns number of elements in list
	int				numElem() const;

	// returns number elements which satisfies to the condition
	template< typename COMPAREFUNC >
	int				numElem( COMPAREFUNC comparator ) const;

	// returns number of elements allocated for
	int				numAllocated() const;

	// sets new granularity
	void			setGranularity( int newgranularity );

	// get the current granularity
	int				getGranularity() const;

	// resizes the list
	void			resize( int newsize );

	// sets the number of elements in list and resize to exactly this number if necessary
	void			setNum( int newnum, bool resize = true );

	// returns a pointer to the list
	T *				ptr();

	// returns a pointer to the list
	const T *		ptr() const;

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
	int				append( const T& obj );

	// appends element
	int				append(T&& obj);

	// append a empty element to be filled
	T&				append();

	// appends another list
	template< typename CT, typename OTHER_STORAGE_TYPE>
	int				append( const ArrayBase<CT, OTHER_STORAGE_TYPE>& other );

	// appends another array
	int				append( const T *other, int count );

	// appends another list with transformation
	// return false to not add the element
	template< typename T2, typename OTHER_STORAGE_TYPE, typename TRANSFORMFUNC >
	int				append( const ArrayBase<T2, OTHER_STORAGE_TYPE>& other, TRANSFORMFUNC transform );

	// inserts the element at the given index
	int				insert( const T & obj, int index = 0 );

	// inserts the new element at the given index
	T&				insert(int index = 0);

	// adds unique element
	int				addUnique( const T & obj );

	// adds unique element
	template< typename PAIRCOMPAREFUNC = PairCompareFunc<T> >
	int				addUnique( const T & obj, PAIRCOMPAREFUNC comparator);

	// finds the index for the given element
	int				findIndex( const T & obj ) const;

	// finds the index for the given element
	template< typename PAIRCOMPAREFUNC = PairCompareFunc<T> >
	int				findIndex( const T & obj, PAIRCOMPAREFUNC comparator) const;

	// returns first found element which satisfies to the condition
	template< typename COMPAREFUNC >
	int				findIndex( COMPAREFUNC comparator ) const;

	// removes the element at the given index
	bool			removeIndex( int index );

	// removes specified count of elements from specified index
	bool			removeRange(int index, int count);

	// removes the element at the given index (fast)
	bool			fastRemoveIndex( int index );

	// removes the element
	bool			remove( const T & obj );

	// removes the element
	bool			fastRemove( const T & obj );

	// swap the contents of the lists
	void			swap(ArrayBase<T, STORAGE_TYPE>&other );

	// swap the contents of the lists - raw
	void			swap(T*& other, int& otherNumElem);

	// reverses the order of array
	void			reverse();

	// assure list has given number of elements, but leave them uninitialized
	void			assureSize( int newSize);

	// assure list has given number of elements and initialize any new elements
	void			assureSize( int newSize, const T &initValue );

	// sorts list using quick sort algorithm
	template< typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
	void			sort(SORTPAIRCOMPAREFUNC comparator);

	template< typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
	void			shellSort(SORTPAIRCOMPAREFUNC comparator, int i0, int i1);

	template< typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
	void			quickSort(SORTPAIRCOMPAREFUNC comparator, int p, int r);

protected:
	STORAGE_TYPE		m_storage;
	int					m_nNumElem{ 0 };
};

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase()
{
	static_assert(!std::is_same_v<STORAGE_TYPE, DynamicArrayStorage<T>>, "PP_SL constructor is required to use");
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::ArrayBase(const PPSourceLine& sl, int granularity )
	: m_storage(sl, granularity)
{
}

template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>::~ArrayBase()
{
	T* listPtr = m_storage.getData();
	ArrayStorageBase<T>::destructElements(listPtr, m_nNumElem);
}

// -----------------------------------------------------------------
// Frees up the memory allocated by the list.  Assumes that T automatically handles freeing up memory.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::clear(bool deallocate)
{
	T* listPtr = m_storage.getData();
	ArrayStorageBase<T>::destructElements(listPtr, m_nNumElem);

	if ( deallocate )
	{
		m_storage.free();
	}

	m_nNumElem		= 0;
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
inline int ArrayBase<T, STORAGE_TYPE>::numElem( COMPAREFUNC comparator ) const
{
	int theCount = 0;
	const T* listPtr = m_storage.getData();

	for(int i = 0; i < m_nNumElem; i++)
	{
		if(comparator(listPtr[i]))
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
	return m_storage.getSize();
}

// -----------------------------------------------------------------
// Sets the base size of the array and resizes the array to match.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::setGranularity( int newgranularity )
{
	int newsize;

	ASSERT( newgranularity > 0);

	m_storage.setGranularity(newgranularity);

	if (m_nNumElem)
	{
		// resize it to the closest level of granularity
		newsize = m_nNumElem + m_storage.getGranularity() - 1;
		newsize -= newsize % m_storage.getGranularity();

		if ( newsize != m_storage.getSize())
		{
			resize( newsize );
		}
	}
}

// -----------------------------------------------------------------
// Returns the current granularity.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::getGranularity() const
{
	return m_storage.getGranularity();
}

// -----------------------------------------------------------------
// Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T & ArrayBase<T, STORAGE_TYPE>::operator[]( int index ) const
{
	ASSERT( index >= 0 );
	ASSERT( index < m_nNumElem );

	return m_storage.getData()[ index ];
}

// -----------------------------------------------------------------
// Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T & ArrayBase<T, STORAGE_TYPE>::operator[]( int index )
{
	ASSERT( index >= 0 );
	ASSERT( index < m_nNumElem );

	return m_storage.getData()[ index ];
}

// -----------------------------------------------------------------
// Copies the contents and size attributes of another list.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline ArrayBase<T, STORAGE_TYPE>& ArrayBase<T, STORAGE_TYPE>::operator=( const ArrayBase<T, STORAGE_TYPE> &other )
{
	m_storage.setGranularity(other.m_storage.getGranularity());

	assureSize(other.m_storage.getSize());

	m_nNumElem = other.m_nNumElem;

	T* listPtr = m_storage.getData();
	const T* otherListPtr = other.m_storage.getData();

	if (m_storage.getSize())
	{
		for( int i = 0; i < m_nNumElem; i++ )
			new(&listPtr[i]) T(otherListPtr[i]);
	}

	return *this;
}



// -----------------------------------------------------------------
// Allocates memory for the amount of elements requested while keeping the contents intact.
// Contents are copied using their = operator so that data is correnctly instantiated.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::resize( int newSize )
{
	m_storage.resize(newSize, m_nNumElem);
}

// -----------------------------------------------------------------
// Resize to the exact size specified irregardless of granularity
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::setNum( int newnum, bool shrinkResize )
{
	ASSERT( newnum >= 0 );

	if (shrinkResize || newnum > m_storage.getSize())
		resize( newnum );

	// initialize new elements
	T* listPtr = m_storage.getData();
	for (int i = m_nNumElem; i < newnum; ++i)
		new(&listPtr[i]) T();

	m_nNumElem = newnum;
}

// -----------------------------------------------------------------
// Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.
// Note: may return NULL if the list is empty.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline T * ArrayBase<T, STORAGE_TYPE>::ptr()
{
	return m_storage.getData();
}

// -----------------------------------------------------------------
// Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.
// Note: may return NULL if the list is empty.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T * ArrayBase<T, STORAGE_TYPE>::ptr() const
{
	return m_storage.getData();
}

// -----------------------------------------------------------------
// Returns front item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::front()
{
	ASSERT(m_nNumElem > 0);
	return *m_storage.getData();
}

// -----------------------------------------------------------------
// Returns front item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::front() const
{
	ASSERT(m_nNumElem > 0);
	return *m_storage.getData();
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::back()
{
	ASSERT(m_nNumElem > 0);
	return m_storage.getData()[m_nNumElem - 1];
}

// -----------------------------------------------------------------
// Returns back item
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline const T& ArrayBase<T, STORAGE_TYPE>::back() const
{
	ASSERT(m_nNumElem > 0);
	return m_storage.getData()[m_nNumElem - 1];
}

// -----------------------------------------------------------------
// Removes front item from list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T ArrayBase<T, STORAGE_TYPE>::popFront()
{
	ASSERT(m_nNumElem > 0);
	T item = m_storage.getData()[0];
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
	T* listPtr = m_storage.getData();
	T item = listPtr[m_nNumElem - 1];
	ArrayStorageBase<T>::destructElements(listPtr + m_nNumElem - 1, 1);
	m_nNumElem--;
	return item;
}

// -----------------------------------------------------------------
// Increases the size of the list by one element and copies the supplied data into it.
// Returns the index of the new element.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(const T& obj)
{
	if ( !m_storage.getData())
		resize(m_storage.getGranularity());

	if ( m_nNumElem == m_storage.getSize())
	{
		int newsize;

		ASSERT(m_storage.getGranularity() > 0);

		newsize = m_storage.getSize() + m_storage.getGranularity();
		resize( newsize - newsize % m_storage.getGranularity());
	}

	T* listPtr = m_storage.getData();

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
	if ( !m_storage.getData())
		resize(m_storage.getGranularity());

	if ( m_nNumElem == m_storage.getSize())
	{
		int newsize;

		ASSERT(m_storage.getGranularity() > 0);

		newsize = m_storage.getSize() + m_storage.getGranularity();
		resize( newsize - newsize % m_storage.getGranularity());
	}

	T* listPtr = m_storage.getData();

	new(&listPtr[m_nNumElem]) T(std::move(obj));
	m_nNumElem++;

	return m_nNumElem - 1;
}

// -----------------------------------------------------------------
// append a new empty element to be filled
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::append()
{
	if (!m_storage.getData())
		resize(m_storage.getGranularity());

	if (m_nNumElem == m_storage.getSize())
	{
		int newsize;

		ASSERT(m_storage.getGranularity() > 0);

		newsize = m_storage.getSize() + m_storage.getGranularity();
		resize(newsize - newsize % m_storage.getGranularity());
	}

	T& newItem = m_storage.getData()[m_nNumElem++];
	new(&newItem) T();
	return newItem;
}

// -----------------------------------------------------------------
// adds the other list to this one
// Returns the size of the new combined list
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename CT, typename OTHER_STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append(const ArrayBase<CT, OTHER_STORAGE_TYPE>& other)
{
	int nOtherElems = other.numElem();

	if ( !m_storage.getData())
	{
		ASSERT(m_storage.getGranularity() > 0);

		// pre-allocate for a bigger list to do less memory resize access
		int newSize = nOtherElems+ m_storage.getGranularity();

		resize( newSize - (newSize % m_storage.getGranularity()) );
	}
	else
	{
		// pre-allocate for a bigger list to do less memory resize access
		if ( m_nNumElem+nOtherElems >= m_storage.getSize())
		{
			int newSize = nOtherElems + m_nNumElem + m_storage.getGranularity();

			resize( newSize - (newSize % m_storage.getGranularity()) );
		}
	}

	// append the elements
	for (int i = 0; i < nOtherElems; i++)
		append(other[i]);

	return numElem();
}

// -----------------------------------------------------------------
// adds the other list to this one
// Returns the size of the new combined list
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::append( const T *other, int count )
{
	if ( !m_storage.getData() )
	{
		ASSERT(m_storage.getGranularity() > 0);

		// pre-allocate for a bigger list to do less memory resize access
		int newSize = count + m_storage.getGranularity();

		resize( newSize - (newSize % m_storage.getGranularity()) );
	}
	else
	{
		// pre-allocate for a bigger list to do less memory resize access
		if ( m_nNumElem+count >= m_storage.getSize())
		{
			int newSize = count + m_nNumElem + m_storage.getGranularity();

			resize( newSize - (newSize % m_storage.getGranularity()) );
		}
	}

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
inline int ArrayBase<T, STORAGE_TYPE>::append( const ArrayBase<T2, OTHER_STORAGE_TYPE> &other, TRANSFORMFUNC transform )
{
	int nOtherElems = other.numElem();

	// it can't predict how many elements it will have. So this is slower implementation

	// try transform and append the elements
	for (int i = 0; i < nOtherElems; i++)
	{
		T newObj;
		if(transform(newObj, other[i]))
			append(other[i]);
	}


	return numElem();
}


// -----------------------------------------------------------------
// Increases the elemCount of the list by at least one element if necessary
// and inserts the supplied data into it.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::insert( T const & obj, int index )
{
	if ( !m_storage.getData())
		resize(m_storage.getGranularity());

	if ( m_nNumElem == m_storage.getSize())
	{
		int newsize;

		ASSERT(m_storage.getGranularity() > 0);

		newsize = m_storage.getSize() + m_storage.getGranularity();
		resize( newsize - newsize % m_storage.getGranularity());
	}

	if ( index < 0 )
		index = 0;
	else if ( index > m_nNumElem )
		index = m_nNumElem;

	T* listPtr = m_storage.getData();

	for (int i = m_nNumElem; i > index; --i)
	{
		new(&listPtr[i]) T(listPtr[i - 1]);
	}

	m_nNumElem++;
	new(&listPtr[index]) T(obj);

	return index;
}

// -----------------------------------------------------------------
// Increases the elemCount of the list by at least one element if necessary
// and returns new element
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline T& ArrayBase<T, STORAGE_TYPE>::insert(int index)
{
	if (!m_storage.getData())
		resize(m_storage.getGranularity());

	if (m_nNumElem == m_storage.getSize())
	{
		int newsize;

		ASSERT(m_storage.getGranularity() > 0);

		newsize = m_storage.getSize() + m_storage.getGranularity();
		resize(newsize - newsize % m_storage.getGranularity());
	}

	if (index < 0)
		index = 0;
	else if (index > m_nNumElem)
		index = m_nNumElem;

	T* listPtr = m_storage.getData();

	for (int i = m_nNumElem; i > index; --i)
	{
		new(&listPtr[i]) T(listPtr[i - 1]);
	}

	m_nNumElem++;
	new(&listPtr[index]) T();

	return listPtr[index];
}

// -----------------------------------------------------------------
// Adds the data to the m_pListPtr if it doesn't already exist.  Returns the index of the data in the m_pListPtr.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique( T const & obj )
{
	int index;

	index = findIndex( obj );

	if ( index < 0 )
		index = append( obj );

	return index;
}


// -----------------------------------------------------------------
// Adds the data to the m_pListPtr if it doesn't already exist.  Returns the index of the data in the m_pListPtr.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename PAIRCOMPAREFUNC >
inline int ArrayBase<T, STORAGE_TYPE>::addUnique( T const & obj, PAIRCOMPAREFUNC comparator)
{
	int index;

	index = findIndex( obj, comparator );

	if ( index < 0 )
		index = append( obj );

	return index;
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_pListPtr and returns it's index.
// Returns -1 if the data is not found.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline int ArrayBase<T, STORAGE_TYPE>::findIndex( T const & obj ) const
{
	const T* listPtr = m_storage.getData();
	for(int i = 0; i < m_nNumElem; i++ )
	{
		if ( listPtr[ i ] == obj )
			return i;
	}
	return -1;
}

// -----------------------------------------------------------------
// Searches for the specified data in the m_pListPtr and returns it's index.
// Returns -1 if the data is not found.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename PAIRCOMPAREFUNC >
inline int ArrayBase<T, STORAGE_TYPE>::findIndex( T const & obj, PAIRCOMPAREFUNC comparator ) const
{
	const T* listPtr = m_storage.getData();
	for(int i = 0; i < m_nNumElem; i++ )
	{
		if ( comparator(listPtr[ i ], obj) )
			return i;
	}
	return -1;
}

// -----------------------------------------------------------------
// returns first element which satisfies to the condition
// Returns NULL if the data is not found.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename COMPAREFUNC >
inline int ArrayBase<T, STORAGE_TYPE>::findIndex( COMPAREFUNC comparator  ) const
{
	const T* listPtr = m_storage.getData();
	for( int i = 0; i < m_nNumElem; i++ )
	{
		if ( comparator(listPtr[i]) )
			return i;
	}
	return -1;
}

// -----------------------------------------------------------------
// Removes the element at the specified index and moves all data following the element down to fill in the gap.
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the index is outside the bounds of the m_pListPtr.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::removeIndex( int index )
{
	ASSERT( index >= 0 );
	ASSERT( index < m_nNumElem );

	if (( index < 0 ) || ( index >= m_nNumElem ))
		return false;

	m_nNumElem--;

	T* listPtr = m_storage.getData();
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

	T* listPtr = m_storage.getData();
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
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemoveIndex( int index )
{
	ASSERT( index >= 0 );
	ASSERT( index < m_nNumElem );

	if (( index < 0 ) || ( index >= m_nNumElem ))
		return false;

	m_nNumElem--;

	T* listPtr = m_storage.getData();
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
inline bool ArrayBase<T, STORAGE_TYPE>::remove( T const & obj )
{
	int index;

	index = findIndex( obj );

	if (index >= 0)
		return removeIndex( index );

	return false;
}


// -----------------------------------------------------------------
// Removes the element if it is found within the m_pListPtr without data movement (only last element will be placed at the removed element's position)
// The number of elements in the m_pListPtr is reduced by one.  Returns false if the data is not found in the m_pListPtr.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the m_pListPtr.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::fastRemove( T const & obj )
{
	int index;

	index = findIndex( obj );

	if (index >= 0)
		return fastRemoveIndex( index );

	return false;
}

// -----------------------------------------------------------------
// Returns true if index is in array range
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline bool ArrayBase<T, STORAGE_TYPE>::inRange( int index ) const
{
	return index >= 0 && index < m_nNumElem;
}

// -----------------------------------------------------------------
// Swaps the contents of two lists
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(ArrayBase<T, STORAGE_TYPE>& other )
{
	QuickSwap( m_nNumElem, other.m_nNumElem );

	m_storage.swap(other.m_storage);
}

// -----------------------------------------------------------------
// swap the contents of the lists - raw
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::swap(T*& other, int& otherNumElem)
{
	QuickSwap(m_nNumElem, otherNumElem);

	m_storage.swap(other, otherNumElem);
}

// -----------------------------------------------------------------
// reverses the order of array
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::reverse()
{
	T* listPtr = m_storage.getData();
	for (int i = 0, j = m_nNumElem - 1; i < j; i++, j--) 
	{
		QuickSwap(listPtr[i], listPtr[j]);
	}
}

// -----------------------------------------------------------------
// Makes sure the list has at least the given number of elements.
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::assureSize( int newSize )
{
	int newNum = newSize;

	if ( newSize > m_storage.getSize())
	{
		ASSERT(m_storage.getGranularity() > 0);

		newSize += m_storage.getGranularity() - 1;
		newSize -= newSize % m_storage.getGranularity();

		resize( newSize );
	}

	m_nNumElem = newNum;
}

// -----------------------------------------------------------------
// Makes sure the listPtr has at least the given number of elements and initialize any elements not yet initialized.
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
inline void ArrayBase<T, STORAGE_TYPE>::assureSize( int newSize, const T &initValue )
{
	int newNum = newSize;

	if ( newSize > m_storage.getSize())
	{
		ASSERT(m_storage.getGranularity() > 0);

		newSize += m_storage.getGranularity() - 1;
		newSize -= newSize % m_storage.getGranularity();

		m_nNumElem = m_storage.getSize();

		resize( newSize );

		T* listPtr = m_storage.getData();
		for ( int i = m_nNumElem; i < newSize; i++ )
			new(&listPtr[i]) T(initValue);
	}

	m_nNumElem = newNum;
}

// -----------------------------------------------------------------
// Sorts array of the elements by the comparator
// -----------------------------------------------------------------

template< typename T, typename STORAGE_TYPE >
template< typename SORTPAIRCOMPAREFUNC >
inline void ArrayBase<T, STORAGE_TYPE>::sort(SORTPAIRCOMPAREFUNC comparator)
{
#ifdef USE_QSORT
	quickSort(comparator, 0, m_nNumElem - 1);
#else
	shellSort(comparator, 0, m_nNumElem - 1);
#endif
}

// -----------------------------------------------------------------
// Shell sort
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename SORTPAIRCOMPAREFUNC >
inline void ArrayBase<T, STORAGE_TYPE>::shellSort(SORTPAIRCOMPAREFUNC comparator, int i0, int i1)
{
	const int SHELLJMP = 3; //2 or 3

	int n = i1-i0;

	int gap;

	for(gap = 1; gap < n; gap = gap*SHELLJMP+1);

	T* listPtr = m_storage.getData();

	for(gap = int(gap/SHELLJMP); gap > 0; gap = int(gap/SHELLJMP))
	{
		for( int i = i0; i < i1-gap; i++)
		{
			for( int j = i; (j >= i0) && (comparator)(listPtr[j], listPtr[j + gap]); j -= gap )
			{
				QuickSwap(listPtr[j], listPtr[j + gap]);
			}
		}
	}
}

// -----------------------------------------------------------------
// Partition exchange sort
// -----------------------------------------------------------------
template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline int partition(T* list, SORTPAIRCOMPAREFUNC comparator, int p, int r)
{
	T pivot = list[p];
	int left = p;

	for (int i = p + 1; i <= r; i++)
	{
		if (comparator(list[i], pivot) < 0)
		{
			left++;

			QuickSwap(list[i], list[left]);
		}
	}

	QuickSwap(list[p], list[left]);
	return left;
}

// -----------------------------------------------------------------
// Partition exchange sort
// -----------------------------------------------------------------
template< typename T, typename STORAGE_TYPE >
template< typename SORTPAIRCOMPAREFUNC >
inline void ArrayBase<T, STORAGE_TYPE>::quickSort(SORTPAIRCOMPAREFUNC comparator, int p, int r)
{
	if (p < r)
	{
		T* listPtr = m_storage.getData();
		int q = partition(listPtr, comparator, p, r);

		quickSort(comparator, p, q - 1);
		quickSort(comparator, q + 1, r);
	}
}

// -----------------------------------------------------------------
// Partition exchange sort
// -----------------------------------------------------------------
template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline void quickSort(T* list, SORTPAIRCOMPAREFUNC comparator, int p, int r)
{
	if (p < r)
	{
		int q = partition(list, comparator, p, r);

		quickSort(list, comparator, p, q - 1);
		quickSort(list, comparator, q + 1, r);
	}
}

template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline void shellSort(T* list, int numElems, SORTPAIRCOMPAREFUNC comparator)
{
	const int SHELLJMP = 3; //2 or 3

	int i0 = 0;
	int i1 = numElems-1;

	int n = i1-i0;

	int gap;

	for(gap = 1; gap < n; gap = gap*SHELLJMP+1);

	for(gap = int(gap/SHELLJMP); gap > 0; gap = int(gap/SHELLJMP))
	{
		for( int i = i0; i < i1-gap; i++)
		{
			for( int j = i; (j >= i0) && (comparator)(list[j], list[j + gap]); j -= gap )
			{
				QuickSwap(list[j], list[j + gap]);
			}
		}
	}
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
public:
	ArrayRef(T* elemPtr, int numElem)
		: m_pListPtr(elemPtr), m_nNumElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayRef(ARRAY_TYPE& otherArray)
		: m_pListPtr(otherArray.ptr()), m_nNumElem(otherArray.numElem())
	{
	}

	T&				operator[](int index)
	{
		ASSERT(index >= 0);
		ASSERT(index < m_nNumElem);

		return m_pListPtr[index];
	}

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
	T*				m_pListPtr;
	const int		m_nNumElem;
};

// const array ref
template< typename T >
class ArrayCRef
{
public:
	ArrayCRef(const T* elemPtr, int numElem)
		: m_pListPtr(elemPtr), m_nNumElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayCRef(ARRAY_TYPE& otherArray)
		: m_pListPtr(otherArray.ptr()), m_nNumElem(otherArray.numElem())
	{
	}

	const T&		operator[](int index) const
	{
		ASSERT(index >= 0);
		ASSERT(index < m_nNumElem);

		return m_pListPtr[index];
	}

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
	const T*		m_pListPtr;
	const int		m_nNumElem;
};

//-------------------------------------------
// nesting Source-line constructor helper
template<typename ITEM>
struct PPSLValueCtor<Array<ITEM>>
{
	Array<ITEM> x;
	PPSLValueCtor<Array<ITEM>>(const PPSourceLine& sl) : x(sl) {}
};