//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium type definitions
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stddef.h>
#include "ds/align.h"

// disable stupid deprecate warning
#pragma warning(disable : 4996)

#undef min
#undef max

#define INLINE			__inline
#define FORCEINLINE		__forceinline

#ifdef __GNUC__
#define __forceinline __attribute__((always_inline))
#endif // __GNUC__

// classname of the main application window
#define Equilibrium_WINDOW_CLASSNAME "Equilibrium_9826C328_598D_4C2E_85D4_0FF8E0310366"

// Define some sized types
typedef unsigned char	uint8;
typedef   signed char	int8;

typedef unsigned short	uint16;
typedef   signed short  int16;

typedef unsigned int	uint32;
typedef   signed int	int32;

typedef unsigned char	ubyte;
typedef unsigned short	ushort;
typedef unsigned int	uint;

typedef ptrdiff_t intptr;

#ifdef _WIN32
typedef   signed __int64  int64;
typedef unsigned __int64 uint64;
#else
typedef   signed long long  int64;
typedef unsigned long long uint64;
#endif

// quick swap function
template< class T >
inline void QuickSwap( T &a, T &b )
{
	T c = a;
	a = b;
	b = c;
}

template <class T, class T2 = T, class TR = T>
static constexpr TR min(T x, T2 y) { return (TR)((x < y) ? x : y); }

template <class T, class T2 = T, class TR = T>
static constexpr TR max(T x, T2 y) { return (TR)((x > y) ? x : y); }

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64   "I64d"
#define PRIu64   "I64u"
#else
#define PRId64   "lld"
#define PRIu64   "llu"
#endif
#endif

//------------------------------------------------------------------------------------------------

// Define some useful macros
#define MCHAR2(a, b)					(a | (b << 8))
#define MCHAR4(a, b, c, d)				(a | (b << 8) | (c << 16) | (d << 24))

#define _STR_(x) #x
#define DEFINE_STR(x)					"#define " #x " " _STR_(x) "\n"

#define elementsOf(x)					(sizeof(x) / sizeof(x[0]))
#define offsetOf(structure,member)		(uintptr_t)&(((structure *)0)->member)
#define elementSizeOf(structure,member)	sizeof(((structure *)0)->member)

#define SAFE_DELETE(p)			{ if (p) { delete (p); (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p)	{ if (p) { delete[] (p); (p) = nullptr; } }

//---------------------------------------------------------------------------------------------
// FIXMEs / TODOs / NOTE macros
//---------------------------------------------------------------------------------------------
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "

#define NOTE( x )  message( x )
#define FILE_LINE  message( __FILE__LINE__ )

#define TODO( x )  message( __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  TODO :   " #x "\n" \
	" -------------------------------------------------\n" )
#define FIXME( x )  message(  __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  FIXME :  " #x "\n" \
	" -------------------------------------------------\n" )
#define todo( x )  message( __FILE__LINE__" TODO :   " #x "\n" ) 
#define fixme( x )  message( __FILE__LINE__" FIXME:   " #x "\n" ) 