//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System messageboxes
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/InterfaceManager.h"
#include "core/profiler.h"				// PPSourceLine

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

enum EEqAssertType {
	_EQASSERT_IGNORE_ALWAYS	= -1,
	_EQASSERT_BREAK			= 1,
	_EQASSERT_SKIP			= 0,	// only when debugger is not present
};


#if defined(_RETAIL) || defined(_PROFILE)

#define	ASSERT_MSG(x, msgFmt, ...)	{ }
#define	ASSERT(x)					{ }
#define ASSERT_FAIL(msgFmt, ...)	{ }

#else

IEXPORTS int _InternalAssertMsg(PPSourceLine sl, bool isSkipped, const char* expression, const char* statement, ...);

#define	ASSERT_MSG(x, msgFmt, ...) \
{ \
	static bool _ignoreAssert = false; \
	if (!(x)) { \
		const int _assertResult = _InternalAssertMsg(PP_SL, _ignoreAssert, #x, msgFmt, ##__VA_ARGS__); \
		if (_assertResult == _EQASSERT_BREAK) { _DEBUG_BREAK; } \
		else if (_assertResult == _EQASSERT_IGNORE_ALWAYS) { _ignoreAssert = true; }\
	} \
}

#define	ASSERT(x)					ASSERT_MSG(x, nullptr)
#define ASSERT_FAIL(msgFmt, ...)	ASSERT_MSG(false, "%s: " msgFmt, __func__, ##__VA_ARGS__)

#endif // _RETAIL || _PROFILE

#define assert_sizeof( type, size )						static_assert( sizeof( type ) == size, "type '" #type "' size " #size " - size mismatch" )
#define assert_sizeof_8_byte_multiple( type )			static_assert( ( sizeof( type ) &  7 ) == 0, "type '" #type "' size is not multiple of 8 bytes" )
#define assert_sizeof_16_byte_multiple( type )			static_assert( ( sizeof( type ) & 15 ) == 0, "type '" #type "' size is not multiple of 16 bytes" )
#define assert_offsetof( type, field, offset )			static_assert( offsetof( type, field ) == offset, "field '" #type "." #field "' size " #offset " - offset mismatch" )
#define assert_offsetof_8_byte_multiple( type, field )	static_assert( ( offsetof( type, field ) & 7 ) == 0, "field '" #type "." #field "' offset is not multiple of 8 bytes" )
#define assert_offsetof_16_byte_multiple( type, field )	static_assert( ( offsetof( type, field ) & 15 ) == 0 , "field '" #type "." #field "' offset is not multiple of 16 bytes" )

// test for C++ standard
assert_sizeof(char, 1);
assert_sizeof(short, 2);
assert_sizeof(int, 4);
//assert_sizeof(long, 4); // NOTE: we should really not use long type in any way.
assert_sizeof(long long, 8);
assert_sizeof(float, 4);
assert_sizeof(double, 8);

