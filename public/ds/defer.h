//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Go-style defer keyword
// Execute code when program leaves the current scope
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// Usage: defer{ statements; };

#ifndef defer

struct EMPTY_BASES defer_dummy {};
template <class F> struct EMPTY_BASES deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return { f }; }
#define DEFER_(LINE)	zz_defer##LINE
#define DEFER(LINE)		DEFER_(LINE)
#define defer auto		DEFER(__LINE__) = defer_dummy{} *[&]()

#endif // defer