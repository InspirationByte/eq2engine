//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RIFF reader utility class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct SpiralIterator
{
	static int GetIterations(int radius);

	SpiralIterator(const IVector2D& cell, int numIter) 
		: cell(cell), numIter(numIter) {}
	bool AtEnd() const;

	void operator++();
	IVector2D operator*() const;

	int numIter{ 0 };
	int dir{ 0 };
	int p{ 0 };
	int q{ 0 };
	IVector2D cell{ 0 };
};

inline int SpiralIterator::GetIterations(int radius)
{
	int n = 0;
	for (int i = 1; i <= radius; i++) // and we have triangular number
		n += i;
	return n * 8 + 1;
}

inline bool SpiralIterator::AtEnd() const
{
	return numIter <= 0;
}

inline IVector2D SpiralIterator::operator*() const
{
	return cell + IVector2D(p, q);
}

inline void SpiralIterator::operator++()
{
	numIter--;

	if (dir == 0)
		dir = (++p + q == 1) ? 1 : dir;
	else if (dir == 1)
		dir = (p == ++q) ? 2 : dir;
	else if (dir == 2)
		dir = (--p + q == 0) ? 3 : dir;
	else
		dir = (p == --q) ? 0 : dir;
}