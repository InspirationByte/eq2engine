#pragma once

#include <typeinfo>	// required for ASSERT messages
#include "core/platform/assert.h"

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
		ASSERT_MSG(index >= 0 && index < m_nNumElem, "ArrayRef<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_nNumElem);

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
	T&				front() { ASSERT(m_nNumElem > 0); return m_pListPtr[0]; }

	// returns front item
	const T&		front() const { ASSERT(m_nNumElem > 0); return m_pListPtr[0]; }

	// returns back item
	T&				back() { ASSERT(m_nNumElem > 0); return m_pListPtr[m_nNumElem - 1]; }

	// returns back item
	const T&		back() const { ASSERT(m_nNumElem > 0); return m_pListPtr[m_nNumElem - 1]; }

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
		ASSERT_MSG(index >= 0 && index < m_nNumElem, "ArrayCRef<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_nNumElem);

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
	const T&		front() const { ASSERT(m_nNumElem > 0); return m_pListPtr[0]; }

	// returns back item
	const T&		back() const { ASSERT(m_nNumElem > 0); return m_pListPtr[m_nNumElem - 1]; }

protected:
	const T*		m_pListPtr{ nullptr };
	int				m_nNumElem{ 0 };
};
