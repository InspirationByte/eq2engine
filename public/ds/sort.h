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

template< class T, typename COMPAREFUNC = PairSortCompareFunc<T> >
int heapMoveDown(T* list, int begin, int mid, int end, const COMPAREFUNC& comparator)
{
	if (mid < begin || mid >= end)
	{
		ASSERT_FAIL("invalid mid index");
		return -1;
	}

	const int count = end - begin;
	T x = std::move(list[mid]);

	int cur = mid - begin;
	int next = cur * 2 + 1; // next left child
	if (next + 1 < count && comparator(list[begin + next], list[begin + next + 1]) < 0)
		++next;

	while (next < count && comparator(x, list[next]) < 0)
	{
		list[begin + cur] = std::move(list[begin + next]);
		cur = next;
		next = next * 2 + 1; // next left child
		if (next + 1 < count && comparator(list[begin + next], list[begin + next + 1]) < 0)
			++next;
	}
	list[begin + cur] = std::move(x);
	return cur;
}

template< class T, typename COMPAREFUNC = PairSortCompareFunc<T> >
int heapMoveUp(T* list, int begin, int mid, int end, const COMPAREFUNC& comparator)
{
	if (mid < begin || mid >= end)
	{
		ASSERT_FAIL("invalid mid index");
		return -1;
	}

	T x = std::move(list[mid]);

	int cur = mid - begin;
	int prev = (cur + 1) / 2 - 1;

	while (prev >= 0 && comparator(x, list[begin + prev]) > 0)
	{
		list[cur] = std::move(list[begin + prev]);
		cur = prev;
		prev = (prev + 1) / 2 - 1;
	}
	list[begin + cur] = std::move(x);
	return cur;
}

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
void heapMake(T* list, int begin, int end, const COMPAREFUNC& comparator)
{
	const int count = end - begin;
	for (int i = count / 2; i >= 0; --i)
		heapMoveDown(list, begin, begin + i, end, comparator);
}

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
void heapSort(T* list, int first, int last, const COMPAREFUNC& comparator)
{
	heapMake(list, first, last + 1, comparator);
	for (int cur = last; cur > first; --cur)
	{
		QuickSwap(list[first], list[cur]);
		heapMoveDown(list, first, first, cur, comparator);
	}
}

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
void insertionSort(T* list, int first, int last, const COMPAREFUNC& comparator)
{
	for (int cur = first + 1; cur <= last; ++cur)
	{
		const int res = comparator(list[cur - 1], list[cur]);
		if (res <= 0)
			continue;
		
		T x = std::move(list[cur]);
		int prev = cur;
		do
		{
			list[prev] = std::move(list[prev - 1]);
			--prev;
		}while (prev > first && comparator(list[prev - 1], x) > 0);

		list[prev] = std::move(x);
	}
}

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
void introSort(T* list, int first, int last, int depth, const COMPAREFUNC& comparator)
{
	constexpr const int MIN_SORT_RANGE = 10;
	while (first + MIN_SORT_RANGE < last)
	{
		--depth;
		if (depth == 0)
		{
			heapSort(list, first, last, comparator);
			return;
		}

		const int mid = first + (last - first + 1) / 2;
		if (comparator(list[first], list[mid]) > 0)
			QuickSwap(list[first], list[mid]);

		if(comparator(list[first], list[last]) > 0)
			QuickSwap(list[first], list[last]);

		if (comparator(list[mid], list[last]) > 0)
			QuickSwap(list[mid], list[last]);

		QuickSwap(list[mid], list[last - 1]);
		const int mi = last - 1;

		int lo = first + 1;
		int hi = last - 2;
		for (;;)
		{
			while (comparator(list[lo], list[mi]) < 0)
				++lo;

			while (lo < hi && comparator(list[hi], list[mi]) >= 0)
				--hi;

			if (lo >= hi)
				break;

			QuickSwap(list[lo], list[hi]);
			++lo;
			--hi;
		}

		QuickSwap(list[lo], list[last - 1]);
		introSort(list, lo + 1, last, depth, comparator);
		last = lo - 1;
	}
	insertionSort(list, first, last, comparator);
}

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
bool arrayIsSorted(T* list, int begin, int end, const COMPAREFUNC& comparator)
{
	for (int i = begin + 1; i < end; ++i)
	{
		if (comparator(list[i - 1], list[i]) > 0)
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

template< typename T, typename COMPAREFUNC = PairSortCompareFunc<T> >
void arraySort(T* list, const COMPAREFUNC& comparator, int begin, int end)
{
	if (begin+1 >= end)
		return;
	introSort(list, begin, end-1, introSortDepth(end - begin), comparator);

	//ASSERT(arrayIsSorted(list, begin, end, comparator));
}

// array wrapper
template< typename ARRAY_TYPE, typename COMPAREFUNC = PairSortCompareFunc<typename ARRAY_TYPE::ITEM> >
void arraySort(ARRAY_TYPE& arr, const COMPAREFUNC& comparator)
{
	arraySort(arr.ptr(), comparator, 0, arr.numElem());
}

// -----------------------------------------------------------------
// Search in arrays
// -----------------------------------------------------------------

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
int arrayFindIndexF(const T* arr, int length, const COMPAREFUNC& comparator)
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