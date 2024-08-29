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
static int sortCompare(const T& a, const T& b)
{
	return (a > b) - (a < b);
}

// -----------------------------------------------------------------
// IntroSort algorithm (hybrid of QuickSort, HeapSort, InsetionSort)
// -----------------------------------------------------------------

#define SORT_HEAP_AT(INDEX) *(begin + (INDEX))

template<typename ITER, typename CMP>
int heapMoveDown(ITER begin, ITER mid, ITER end, const CMP& comparator)
{
	using ValueType = std::remove_pointer_t<ITER>;

	if (mid < begin || mid >= end)
	{
		ASSERT_FAIL("invalid mid index");
		return -1;
	}

	const int count = end - begin;
	ValueType x = std::move(*mid);

	int cur = mid - begin;
	int next = cur * 2 + 1; // next left child
	if (next + 1 < count && comparator(SORT_HEAP_AT(next), SORT_HEAP_AT(next + 1)) < 0)
		++next;

	while (next < count && comparator(x, SORT_HEAP_AT(next)) < 0)
	{
		SORT_HEAP_AT(cur) = std::move(SORT_HEAP_AT(next));
		cur = next;
		next = next * 2 + 1; // next left child
		if (next + 1 < count && comparator(SORT_HEAP_AT(next), SORT_HEAP_AT(next + 1)) < 0)
			++next;
	}
	SORT_HEAP_AT(cur) = std::move(x);
	return cur;
}

template<typename ITER, typename CMP>
int heapMoveUp(ITER begin, ITER mid, ITER end, const CMP& comparator)
{
	using ValueType = std::remove_pointer_t<ITER>;

	if (mid < begin || mid >= end)
	{
		ASSERT_FAIL("invalid mid index");
		return -1;
	}

	ValueType x = std::move(*mid);

	int cur = mid - begin;
	int prev = (cur + 1) / 2 - 1;

	while (prev >= 0 && comparator(x, SORT_HEAP_AT(prev)) > 0)
	{
		SORT_HEAP_AT(cur) = std::move(SORT_HEAP_AT(prev));
		cur = prev;
		prev = (prev + 1) / 2 - 1;
	}
	SORT_HEAP_AT(cur) = std::move(x);
	return cur;
}

#undef SORT_HEAP_AT

template<typename ITER, typename CMP>
void heapMake(ITER begin, ITER end, const CMP& comparator)
{
	const int count = end - begin;
	for (int i = count / 2; i >= 0; --i)
		heapMoveDown(begin, begin + i, end, comparator);
}

template<typename ITER, typename CMP>
void heapSort(ITER first, ITER last, const CMP& comparator)
{
	heapMake(first, last + 1, comparator);
	for (ITER cur = last; cur > first; --cur)
	{
		QuickSwap(*first, *cur);
		heapMoveDown(first, first, cur, comparator);
	}
}

template<typename ITER, typename CMP>
void insertionSort(ITER first, ITER last, const CMP& comparator)
{
	using ValueType = std::remove_pointer_t<ITER>;

	for (ITER cur = first + 1; cur <= last; ++cur)
	{
		const int res = comparator(*(cur - 1), *cur);
		if (res <= 0)
			continue;
		
		ValueType x = std::move(*cur);
		ITER prev = cur;
		do
		{
			*prev = std::move(*(prev - 1));
			--prev;
		} while (prev > first && comparator(*(prev - 1), x) > 0);

		*prev = std::move(x);
	}
}

template<typename ITER, typename CMP >
void introSort(ITER first, ITER last, int depth, const CMP& comparator)
{
	constexpr const int MIN_SORT_RANGE = 10;
	while (first + MIN_SORT_RANGE < last)
	{
		--depth;
		if (depth == 0)
		{
			heapSort(first, last, comparator);
			return;
		}

		ITER mid = first + (last - first + 1) / 2;
		if (comparator(*first, *mid) > 0)
			QuickSwap(*first, *mid);

		if(comparator(*first, *last) > 0)
			QuickSwap(*first, *last);

		if (comparator(*mid, *last) > 0)
			QuickSwap(*mid, *last);

		QuickSwap(*mid, *(last - 1));
		ITER mi = last - 1;

		ITER lo = first + 1;
		ITER hi = last - 2;
		for (;;)
		{
			while (comparator(*lo, *mi) < 0)
				++lo;

			while (lo < hi && comparator(*hi, *mi) >= 0)
				--hi;

			if (lo >= hi)
				break;

			QuickSwap(*lo, *hi);
			++lo;
			--hi;
		}

		QuickSwap(*lo, * (last - 1));
		introSort(lo + 1, last, depth, comparator);
		last = lo - 1;
	}
	insertionSort(first, last, comparator);
}

template<typename ITER, typename CMP >
bool arrayIsSorted(ITER begin, ITER end, const CMP& comparator)
{
	for (ITER it = begin + 1; it < end; ++it)
	{
		if (comparator(*(it - 1), *it) > 0)
			return false;
	}
	return true;
}

inline int introSortDepth(int count)
{
	int res = 0;
	while (count > 1)
	{
		count >>= 1;
		++res;
	}
	return res * 2;
}

template<typename ITER, typename CMP>
void arraySort(ITER begin, ITER end, const CMP& comparator)
{
	if (begin + 1 >= end)
		return;
	introSort(begin, end - 1, introSortDepth(end - begin), comparator);

	ASSERT(arrayIsSorted(begin, end, comparator));
}

// array wrapper
template< typename ARRAY_TYPE, typename CMP >
void arraySort(ARRAY_TYPE& arr, const CMP& comparator)
{
	arraySort(arr.ptr(), arr.ptr() + arr.numElem(), comparator);
}

// -----------------------------------------------------------------
// Search in arrays
// -----------------------------------------------------------------

// finds the index for the given element
template< typename ITER, typename K>
int arrayFindIndex(ITER begin, ITER end, const K& key)
{
	for (ITER it = begin; it < end; ++it)
	{
		if (*it == key)
			return int(it - begin);
	}
	return -1;
}

template< typename ARRAY_TYPE, typename K>
int arrayFindIndex(const ARRAY_TYPE& arr, const K& key)
{
	return arrayFindIndex(arr.ptr(), arr.ptr() + arr.numElem(), key);
}

// finds the index for the given element
template<typename ITER, typename K, typename CMP>
int arrayFindIndexF(ITER begin, ITER end, const K& key, const CMP& comparator)
{
	for (ITER it = begin; it < end; ++it)
	{
		if (comparator(*it, key))
			return int(it - begin);
	}
	return -1;
}

template< typename ARRAY_TYPE, typename K, typename CMP>
int arrayFindIndexF(const ARRAY_TYPE& arr, const K& key, const CMP& comparator)
{
	return arrayFindIndexF(arr.ptr(), arr.ptr() + arr.numElem(), key, comparator);
}

// returns first found element which satisfies to the condition
template<typename ITER, typename CMP >
int arrayFindIndexF(ITER begin, ITER end, const CMP& comparator)
{
	for (ITER it = begin; it < end; ++it)
	{
		if (comparator(*it))
			return int(it - begin);
	}
	return -1;
}

template<typename ARRAY_TYPE, typename CMP>
int arrayFindIndexF(const ARRAY_TYPE& arr, const CMP& comparator)
{
	return arrayFindIndexF(arr.ptr(), arr.ptr() + arr.numElem(), comparator);
}

// reverses the order of array
template< typename T >
void arrayReverse(T* arr, int start = 0, int count = 0)
{
	for (int i = start, j = count - 1; i < j; ++i, --j)
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

template<SortedFind FIND, typename T, typename K, typename CMP>
int arraySortedFindIndexExt(const T* arr, int length, const K& key, const CMP& comparator)
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

template<SortedFind FIND, typename ARRAY_TYPE, typename K, typename CMP>
int arraySortedFindIndexExt(const ARRAY_TYPE& arr, const K& key, const CMP& comparator)
{
	return arraySortedFindIndexExt<FIND>(arr.ptr(), arr.numElem(), key, comparator);
}