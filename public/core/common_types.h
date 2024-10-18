//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
#define __forceinline							__attribute__((always_inline))
#define ATTRIB_FORMAT_PRINTF(fmtpos, argspos)	__attribute__((format(printf, fmtpos, argspos)))
#define PRINTF_FMT_CHECK(fmt, ...)
#else
#define ATTRIB_FORMAT_PRINTF(fmtpos, argspos)
#define PRINTF_FMT_CHECK(fmt, ...)				(void)sizeof(printf(fmt, ##__VA_ARGS__)),
#endif // __GNUC__

#ifdef PLAT_ANDROID

typedef __builtin_va_list	va_list;
#ifndef va_start
#	define va_start(v,l)		__builtin_va_start(v,l)
#endif

#ifndef va_end
#	define va_end(v)			__builtin_va_end(v)
#endif

#ifndef va_arg
#	define va_arg(v,l)			__builtin_va_arg(v,l)
#endif

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L || defined(__GXX_EXPERIMENTAL_CXX0X__)

#	ifndef va_copy
#		define va_copy(d,s)		__builtin_va_copy(d,s)
#	endif

#endif

#ifndef __va_copy
#	define __va_copy(d,s)		__builtin_va_copy(d,s)
#endif

typedef __builtin_va_list	__gnuc_va_list;
typedef __gnuc_va_list		va_list;
typedef va_list				__va_list;

#endif // PLAT_ANDROID

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

typedef ptrdiff_t 		intptr;

#ifdef _WIN32
typedef   signed __int64  	int64;
typedef unsigned __int64 	uint64;
#else
typedef   signed long long  int64;
typedef unsigned long long 	uint64;
#endif

#define COM_CHAR_MIN	(-COM_CHAR_MAX - 1)
#define COM_CHAR_MAX 	0x7f
#define COM_UCHAR_MAX 	0xff

#define COM_SHRT_MIN	(-COM_SHRT_MAX - 1)
#define COM_SHRT_MAX 	0x7fff
#define COM_USHRT_MAX 	0xffff

#define COM_INT_MIN		(-INT_MAX - 1)
#define COM_INT_MAX 	0x7fffffff
#define COM_UINT_MAX 	0xffffffff

#define COM_FLT_MIN		FLT_MIN
#define COM_FLT_MAX		FLT_MAX

// quick swap function
template< class T >
inline void QuickSwap( T &a, T &b )
{
	T c = a;
	a = b;
	b = c;
}

template <class T, class T2 = T, class TR = T>
static constexpr TR min(T x, T2 y) { return static_cast<TR>((x < y) ? x : y); }

template <class T, class T2 = T, class TR = T>
static constexpr TR max(T x, T2 y) { return static_cast<TR>((x > y) ? x : y); }

// portable format specifiers for int64 & uint64
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

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#ifdef _MSC_VER
// see https://devblogs.microsoft.com/cppblog/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3/
#define EMPTY_BASES __declspec(empty_bases)
#else
#define EMPTY_BASES
#endif

// Define some useful macros
#define MAKECHAR4(a, b, c, d)			(a | (b << 8) | (c << 16) | (d << 24))

#define _SEMICOLON_REQ( expr )			do expr while(0)
#define _STR_(x) #x
#define DEFINE_STR(x)					"#define " #x " " _STR_(x) "\n"

#define elementsOf(x)					(sizeof(x) / sizeof(x[0]))
#define offsetOf(structure,member)		(uintptr_t)&(((structure *)0)->member)
#define elementSizeOf(structure,member)	sizeof(((structure *)0)->member)

#define SAFE_DELETE(p)			_SEMICOLON_REQ( if (p) { delete (p); (p) = nullptr; } )
#define SAFE_DELETE_ARRAY(p)	_SEMICOLON_REQ( if (p) { delete[] (p); (p) = nullptr; } )

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