//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Sort algorithms
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template< typename T >
using PairSortCompareFunc = int (*)(const T& a, const T& b);

// -----------------------------------------------------------------
// Partition exchange sort (QuickSort)
// -----------------------------------------------------------------
template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline int partition(T* list, SORTPAIRCOMPAREFUNC& comparator, int p, int r)
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

template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline void quickSort(T* list, SORTPAIRCOMPAREFUNC& comparator, int p, int r)
{
	if (p >= r)
		return;

	int q = partition(list, comparator, p, r);

	quickSort(list, comparator, p, q - 1);
	quickSort(list, comparator, q + 1, r);
}

// -----------------------------------------------------------------
// Shell sort
// -----------------------------------------------------------------
template< typename T, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
inline void shellSort(T* list, SORTPAIRCOMPAREFUNC& comparator, int i0, int i1)
{
	const int SHELLJMP = 3; //2 or 3
	const int n = i1 - i0;

	int gap;
	for (gap = 1; gap < n; gap = gap * SHELLJMP + 1);

	for (gap = int(gap / SHELLJMP); gap > 0; gap = int(gap / SHELLJMP))
	{
		for (int i = i0; i < i1 - gap; i++)
		{
			for (int j = i; (j >= i0) && (comparator)(list[j], list[j + gap]) > 0; j -= gap)
			{
				QuickSwap(list[j], list[j + gap]);
			}
		}
	}
}

// array wrapper
template< typename T, typename STORAGE_TYPE, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
void shellSort(ArrayBase<T, STORAGE_TYPE>& arr, SORTPAIRCOMPAREFUNC& comparator, int i0 = 0, int i1 = 0)
{
	shellSort(arr.ptr(), comparator, i0, i1 == 0 ? arr.numElem()-1 : i1);
}

// array wrapper
template< typename T, typename STORAGE_TYPE, typename SORTPAIRCOMPAREFUNC = PairSortCompareFunc<T> >
void quickSort(ArrayBase<T, STORAGE_TYPE>& arr, SORTPAIRCOMPAREFUNC& comparator, int p = 0, int r = 0)
{
	quickSort(arr.ptr(), comparator, p, r == 0 ? arr.numElem() - 1 : r);
}
