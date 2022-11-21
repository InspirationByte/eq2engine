//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/InterfaceManager.h"

#ifndef _WIN32

#include <signal.h>
#define _DEBUG_BREAK raise(SIGINT)

#else

#if _MSC_VER >= 1400
#define _DEBUG_BREAK __debugbreak()
#else
#define _DEBUG_BREAK _asm int 0x03
#endif

#endif

#if defined(_RETAIL) || defined(_PROFILE)

#define	ASSERT_MSG(x, msgFmt, ...)
#define	ASSERT(x)
#define ASSERT_FAIL(msgFmt, ...)

#else

#define _EQASSERT_IGNORE_ALWAYS		-1
#define _EQASSERT_BREAK				1
#define _EQASSERT_SKIP				0	// only when debugger is not present

IEXPORTS int _InternalAssertMsg(PPSourceLine sl, const char* statement, ...);

#define	ASSERT_MSG(x, msgFmt, ...) \
{ \
	static bool ignoreAssert = false; \
	if (!(x) && !ignoreAssert) { \
		const int result = _InternalAssertMsg(PP_SL, msgFmt, ##__VA_ARGS__); \
		if (result == _EQASSERT_BREAK) { _DEBUG_BREAK; } \
		else if (result == _EQASSERT_IGNORE_ALWAYS) { ignoreAssert = true; }\
	} \
}

#define	ASSERT(x)					ASSERT_MSG(x, #x)
#define ASSERT_FAIL(msgFmt, ...)	ASSERT_MSG(false, msgFmt, ##__VA_ARGS__)

#endif // _RETAIL || _PROFILE

#if !defined( __TYPEINFOGEN__ ) && !defined( _lint ) && defined(_WIN32)	// pcLint has problems with assert_offsetof()

template<bool> struct compile_time_assert_failed;
template<> struct compile_time_assert_failed<true> {};
template<int x> struct compile_time_assert_test {};
#define compile_time_assert_join2( a, b )	a##b
#define compile_time_assert_join( a, b )	compile_time_assert_join2(a,b)
#define compile_time_assert( x )			typedef compile_time_assert_test<sizeof(compile_time_assert_failed<(bool)(x)>)> compile_time_assert_join(compile_time_assert_typedef_, __LINE__)

#define assert_sizeof( type, size )						compile_time_assert( sizeof( type ) == size )
#define assert_sizeof_8_byte_multiple( type )			compile_time_assert( ( sizeof( type ) &  7 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )			compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_offsetof( type, field, offset )			compile_time_assert( offsetof( type, field ) == offset )
#define assert_offsetof_8_byte_multiple( type, field )	compile_time_assert( ( offsetof( type, field ) & 7 ) == 0 )
#define assert_offsetof_16_byte_multiple( type, field )	compile_time_assert( ( offsetof( type, field ) & 15 ) == 0 )

#else

#define compile_time_assert( x )
#define assert_sizeof( type, size )
#define assert_sizeof_8_byte_multiple( type )
#define assert_sizeof_16_byte_multiple( type )
#define assert_offsetof( type, field, offset )
#define assert_offsetof_8_byte_multiple( type, field )
#define assert_offsetof_16_byte_multiple( type, field )

#endif

// test for C++ standard
assert_sizeof(char, 1);
assert_sizeof(short, 2);
assert_sizeof(int, 4);
assert_sizeof(long, 4);
assert_sizeof(long long, 8);
assert_sizeof(float, 4);
assert_sizeof(double, 8);
