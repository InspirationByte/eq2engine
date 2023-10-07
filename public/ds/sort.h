//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Sort algorithms
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template< typename T >
using PairCompareFunc = bool (*)(const T& a, const T& b);

template< typename T >
using PairSortCompareFunc = int (*)(const T& a, const T& b);

template< typename T >
using PairIndexedSortCompareFunc = int (*)(int idxA, int idxB);

// -----------------------------------------------------------------
// Partition exchange sort (QuickSort)
// -----------------------------------------------------------------
template< typename T, typename C = PairSortCompareFunc<T> >
inline int partition(T* list, C comparator, int p, int r)
{
	const T& pivot = list[p];
	int left = p;

	for (int i = p + 1; i <= r; i++)
	{
		if (comparator(list[i], pivot) < 0)
		{
			++left;
			QuickSwap(list[i], list[left]);
		}
	}

	QuickSwap(list[p], list[left]);
	return left;
}

template< typename T, typename C = PairSortCompareFunc<T> >
inline void quickSort(T* list, C comparator, int p, int r)
{
	if (p >= r)
		return;

	int q = partition(list, comparator, p, r);

	quickSort(list, comparator, p, q - 1);
	quickSort(list, comparator, q + 1, r);
}

template< typename T, typename C = PairIndexedSortCompareFunc<T> >
inline void quickSortIdx(T* list, C comparator, int p, int r)
{
	quickSort(list, [&](const T& a, const T& b){ return comparator((int)(&a - list), (int)(&b - list));}, p, r);
}

// -----------------------------------------------------------------
// Shell sort
// -----------------------------------------------------------------
template< typename T, typename C = PairSortCompareFunc<T> >
inline void shellSort(T* list, C comparator, int i0, int i1)
{
	const int SHELLJMP = 3; //2 or 3
	const int n = i1 - i0;

	int gap;
	for (gap = 1; gap < n; gap = gap * SHELLJMP + 1);

	for (gap = int(gap / SHELLJMP); gap > 0; gap = int(gap / SHELLJMP))
	{
		for (int i = i0; i < i1 - gap; i++)
		{
			for (int j = i; (j >= i0) && comparator(list[j], list[j + gap]) > 0; j -= gap)
			{
				QuickSwap(list[j], list[j + gap]);
			}
		}
	}
}

template< typename T, typename C = PairIndexedSortCompareFunc<T> >
inline void shellSortIdx(T* list, C comparator, int i0, int i1)
{
	shellSort(list, [&](const T& a, const T& b){ return comparator((int)(&a - list), (int)(&b - list));}, i0, i1);
}

// array wrapper
template< typename ARRAY_TYPE, typename C = PairSortCompareFunc<typename ARRAY_TYPE::ITEM> >
void shellSort(ARRAY_TYPE& arr, C comparator, int i0 = 0, int i1 = 0)
{
	shellSort(arr.ptr(), comparator, i0, i1 == 0 ? arr.numElem()-1 : i1);
}

template< typename ARRAY_TYPE, typename C = PairIndexedSortCompareFunc<typename ARRAY_TYPE::ITEM> >
void shellSortIdx(ARRAY_TYPE& arr, C comparator, int i0 = 0, int i1 = 0)
{
	shellSortIdx(arr.ptr(), comparator, i0, i1 == 0 ? arr.numElem()-1 : i1);
}

// array wrapper
template< typename ARRAY_TYPE, typename C = PairSortCompareFunc<typename ARRAY_TYPE::ITEM> >
void quickSort(ARRAY_TYPE& arr, C comparator, int p = 0, int r = 0)
{
	quickSort(arr.ptr(), comparator, p, r == 0 ? arr.numElem() - 1 : r);
}

template< typename ARRAY_TYPE, typename C = PairIndexedSortCompareFunc<typename ARRAY_TYPE::ITEM> >
void quickSortIdx(ARRAY_TYPE& arr, C comparator, int p = 0, int r = 0)
{
	quickSortIdx(arr.ptr(), comparator, p, r == 0 ? arr.numElem() - 1 : r);
}

// finds the index for the given element
template< typename T, typename K>
int arrayFindIndex(const T* arr, int length, const K& key)
{
	for (int i = 0; i < length; ++i)
	{
		if (arr[i] == key)
			return i;
	}
	return -1;
}

template< typename ARRAY_TYPE, typename K>
int arrayFindIndex(const ARRAY_TYPE& arr, const K& key)
{
	return arrayFindIndex(arr.ptr(), arr.numElem(), key);
}

// finds the index for the given element
template< typename T, typename K, typename PAIRCOMPAREFUNC = PairCompareFunc<T> >
int arrayFindIndexF(const T* arr, int length, const K& key, const PAIRCOMPAREFUNC& comparator)
{
	for (int i = 0; i < length; ++i)
	{
		if (comparator(arr[i], key))
			return i;
	}
	return -1;
}

template< typename ARRAY_TYPE, typename K, typename PAIRCOMPAREFUNC = PairCompareFunc<typename ARRAY_TYPE::ITEM> >
int arrayFindIndexF(const ARRAY_TYPE& arr, const K& key, const PAIRCOMPAREFUNC& comparator)
{
	return arrayFindIndexF(arr.ptr(), arr.numElem(), key, comparator);
}

// returns first found element which satisfies to the condition
template< typename T, typename COMPAREFUNC >
int arrayFindIndexF(const T& arr, int length, const COMPAREFUNC& comparator)
{
	for (int i = 0; i < length; ++i)
	{
		if (comparator(arr[i]))
			return i;
	}
	return -1;
}

template< typename ARRAY_TYPE, typename COMPAREFUNC >
int arrayFindIndexF(const ARRAY_TYPE& arr, const COMPAREFUNC& comparator)
{
	return arrayFindIndexF(arr.ptr(), arr.numElem(), comparator);
}

// reverses the order of array
template< typename T >
void arrayReverse(T* arr, int start = 0, int count = 0)
{
	for (int i = start, j = count - 1; i < j; i++, j--)
		QuickSwap(arr[i], arr[j]);
}

template< typename ARRAY_TYPE >
void arrayReverse(ARRAY_TYPE& arr, int start = 0, int count = -1)
{
	if (start == 0 && count == -1)
		count = arr.numElem();
	arrayReverse(arr.ptr(), start, count);
}

enum class SortedFind : int
{
	FIRST,
	LAST,
	FIRST_GREATER,
	FIRST_GEQUAL,
	LAST_LEQUAL,
};

template<SortedFind FIND, typename T, typename K, typename COMPAREFUNC>
int arraySortedFindIndexExt(const T* arr, int length, const K& key, const COMPAREFUNC& comparator)
{
	if (length == 0)
		return -1;

	int lo = 0;
	int hi = length - 1;
	while (hi - lo > 1)
	{
		const int mid = (hi + lo) >> 1;
		const int res = comparator(arr[mid], key);

		if constexpr (FIND == SortedFind::FIRST || FIND == SortedFind::FIRST_GREATER)
		{
			if (res >= 0)
				hi = mid;
			else
				lo = mid;
		}
		else // for LAST, FIRST_GEQUAL, and LAST_LEQUAL
		{
			if (res > 0)
				hi = mid;
			else
				lo = mid;
		}
	}

	if constexpr (FIND == SortedFind::FIRST)
	{
		if (comparator(arr[lo], key) == 0)
			return lo;
		if (comparator(arr[hi], key) == 0)
			return hi;
	}
	else if constexpr (FIND == SortedFind::LAST)
	{
		if (comparator(arr[hi], key) == 0)
			return hi;
		if (comparator(arr[lo], key) == 0)
			return lo;
	}
	else if constexpr (FIND == SortedFind::FIRST_GREATER)
	{
		if (comparator(arr[lo], key) > 0)
			return lo;
		if (comparator(arr[hi], key) > 0)
			return hi;
	}
	else if constexpr (FIND == SortedFind::FIRST_GEQUAL)
	{
		if (comparator(arr[lo], key) >= 0)
			return lo;
		if (hi != lo && comparator(arr[hi], key) >= 0)
			return hi;
	}
	else if constexpr (FIND == SortedFind::LAST_LEQUAL)
	{
		if (comparator(arr[hi], key) <= 0)
			return hi;
		if (lo != hi && comparator(arr[lo], key) <= 0)
			return lo;
	}

	return -1;
}

template<SortedFind FIND, typename ARRAY_TYPE, typename K, typename COMPAREFUNC>
int arraySortedFindIndexExt(const ARRAY_TYPE& arr, const K& key, const COMPAREFUNC& comparator)
{
	return arraySortedFindIndexExt<FIND>(arr.ptr(), arr.numElem(), key, comparator);
}