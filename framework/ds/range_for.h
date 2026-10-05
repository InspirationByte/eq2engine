#pragma once

/*
Usage:

struct RangeFor : RangeForMixin<RangeFor>
{
	bool AtEnd() const	{ return currentItem >= count; }
	int operator*()		{ return currentItem; }
	void operator++()	{ increment(); }
};
*/

struct _EndMarker {};

template<typename IT>
struct EMPTY_BASES RangeForMixin
{
	bool operator==(_EndMarker) { return static_cast<IT*>(this)->AtEnd(); }
	bool operator!=(_EndMarker) { return !static_cast<IT*>(this)->AtEnd(); }

	friend IT			begin(const IT& it) { return it; }
	friend _EndMarker	end(const IT& it) { return {}; }
};

struct RangeFor : RangeForMixin<RangeFor>
{
	int from;
	int to;
	int step;

	RangeFor(int from, int to)
		: from(from)
		, to(to)
		, step(from <= to ? 1 : -1)
	{
	}

	bool AtEnd() const { return from == to + step; }
	int operator*() const { return from; }
	void operator++() { from += step; }
};