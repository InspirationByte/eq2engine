#pragma once

#include <typeinfo>	// required for ASSERT messages
#include "core/platform/assert.h"

template<typename T, typename STORAGE_TYPE>
class ArrayBase;

// check if it's ds Array type. NOT THE SAME AS is_array
template< typename T >
struct IsArray : std::false_type {};

template<typename T, typename T2>
struct IsArray<ArrayBase<T, T2>> : std::true_type {};

template< typename T > class ArrayRef;
template< typename T > class ArrayCRef;

template<typename T>
struct IsArray<ArrayRef<T>> : std::true_type {};

template<typename T>
struct IsArray<ArrayCRef<T>> : std::true_type {};

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
		: m_data(elemPtr), m_numElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayRef(ARRAY_TYPE& otherArray)
	{
		if constexpr (std::is_array<ARRAY_TYPE>::value)
		{
			m_data = otherArray;
			m_numElem = elementsOf(otherArray);
		}
		else
		{
			m_data = otherArray.ptr();
			m_numElem = otherArray.numElem();
		}
	}

	template<int N>
	ArrayRef(T(&otherArray)[N])
		: m_data(otherArray), m_numElem(N)
	{
	}

	ArrayRef<T>& operator=(std::nullptr_t)
	{
		m_data = nullptr;
		m_numElem = 0;
		return *this;
	}

	template<typename ARRAY_TYPE>
	ArrayRef<T>& operator=(const ARRAY_TYPE& other)
	{
		m_data = other.m_data;
		m_numElem = other.m_numElem;
		return *this;
	}

	ArrayRef<T>& operator=(const ArrayRef<T>& other)
	{
		m_data = const_cast<T*>(other.m_data);
		m_numElem = other.m_numElem;
		return *this;
	}

	T& operator[](int index)
	{
		ASSERT_MSG(index >= 0 && index < m_numElem, "ArrayRef<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_numElem);
		return m_data[index];
	}

	const T& operator[](int index) const
	{
		ASSERT_MSG(index >= 0 && index < m_numElem, "ArrayRef<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_numElem);
		return m_data[index];
	}

	Iterator		begin() const { return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const { return Iterator(*const_cast<SelfType*>(this), m_numElem); }

	// returns number of elements in list
	int				numElem() const { return m_numElem; }

	// returns true if empty
	bool			isEmpty() const { return numElem() == 0; }

	// returns a pointer to the list
	T*				ptr() { return m_data; }
	const T*		ptr() const { return m_data; }

	// returns true if index is in range
	bool			inRange(int index) const { return index >= 0 && index < m_numElem; }

	// returns front item
	T&				front() { ASSERT(m_numElem > 0); return m_data[0]; }

	// returns front item
	const T&		front() const { ASSERT(m_numElem > 0); return m_data[0]; }

	// returns back item
	T&				back() { ASSERT(m_numElem > 0); return m_data[m_numElem - 1]; }

	// returns back item
	const T&		back() const { ASSERT(m_numElem > 0); return m_data[m_numElem - 1]; }

protected:
	T*				m_data{ nullptr };
	int				m_numElem{ 0 };
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
		: m_data(elemPtr), m_numElem(numElem)
	{
	}

	template<typename ARRAY_TYPE>
	ArrayCRef(const ARRAY_TYPE& otherArray)
	{
		if constexpr (std::is_array<ARRAY_TYPE>::value)
		{
			m_data = otherArray;
			m_numElem = elementsOf(otherArray);
		}
		else
		{
			static_assert(IsArray<ARRAY_TYPE>::value, "Not Array or ArrayCRef");
			m_data = otherArray.ptr();
			m_numElem = otherArray.numElem();
		}
	}

	template<int N>
	ArrayCRef(T(&otherArray)[N])
		: m_data(otherArray), m_numElem(N)
	{
	}

	ArrayRef<T>& operator=(std::nullptr_t)
	{
		m_data = nullptr;
		m_numElem = 0;
		return *this;
	}

	template<typename ARRAY_TYPE>
	ArrayCRef<T>& operator=(const ARRAY_TYPE& other)
	{
		m_data = other.ptr();
		m_numElem = other.numElem();
		return *this;
	}

	ArrayCRef<T>& operator=(const ArrayRef<T>& other)
	{
		m_data = other.m_data;
		m_numElem = other.m_numElem;
		return *this;
	}

	ArrayCRef<T>& operator=(const ArrayCRef<T>& other)
	{
		m_data = other.m_data;
		m_numElem = other.m_numElem;
		return *this;
	}

	const T& operator[](int index) const
	{
		ASSERT_MSG(index >= 0 && index < m_numElem, "ArrayCRef<%s> invalid index %d (numElem = %d)", typeid(T).name(), index, m_numElem);

		return m_data[index];
	}

	Iterator		begin() const { return Iterator(*const_cast<SelfType*>(this), 0); }
	Iterator		end() const { return Iterator(*const_cast<SelfType*>(this), m_numElem); }

	// returns number of elements in list
	int				numElem() const { return m_numElem; }

	// returns true if empty
	bool			isEmpty() const { return numElem() == 0; }

	// returns a pointer to the list
	const T*		ptr() const { return m_data; }

	// returns true if index is in range
	bool			inRange(int index) const { return index >= 0 && index < m_numElem; }

	// returns front item
	const T&		front() const { ASSERT(m_numElem > 0); return m_data[0]; }

	// returns back item
	const T&		back() const { ASSERT(m_numElem > 0); return m_data[m_numElem - 1]; }

protected:
	const T*		m_data{ nullptr };
	int				m_numElem{ 0 };
};
