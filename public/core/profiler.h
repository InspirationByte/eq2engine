//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// source-line contailer
struct PPSourceLine
{
	uint64 data{ 0 };

	static PPSourceLine Empty();
	static PPSourceLine Make(const char* filename, int line);

	const char* GetFileName() const;
	int			GetLine() const;
};

// Source-line value constructor helper
template<typename T>
struct PPSLValueCtor
{
	T x;
	PPSLValueCtor<T>(const PPSourceLine& sl) : x() {}
};

#define PP_SL			PPSourceLine::Make(__FILE__, __LINE__)
#define	PPNew			new(PP_SL)
#define	PPNewSL(sl)		new(sl)