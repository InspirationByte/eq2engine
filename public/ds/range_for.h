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
struct RangeForMixin 
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

	RangeFor(int from, int to) : from(from), to(to) {}

	bool AtEnd() const { return from >= to; }
	int operator*() { return from; }
	void operator++() { ++to; }
};