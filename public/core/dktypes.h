//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium type definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef DKTYPES_H
#define DKTYPES_H

#include <stddef.h>
#include <string.h>

// disable stupid deprecate warning
#pragma warning(disable : 4996)

#define INLINE		__inline

#ifdef __GNUC__
#define __forceinline __attribute__((always_inline))
#endif // __GNUC__

#define FORCEINLINE __forceinline

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

#ifdef _MSC_VER // maybe GCC?

#define forceinline __forceinline
#define _ALIGNED(x) __declspec(align(x))

#define ALIGNED_TYPE(s, a) typedef s _ALIGNED(a)

#else

#define forceinline inline
#define _ALIGNED(x) __attribute__ ((aligned(x)))

#define ALIGNED_TYPE(s, a) typedef struct s _ALIGNED(a)

#endif

class CEqException
{
	static const int ERROR_BUFFER_LENGTH = 2048;
public:
	CEqException( const char* text = "" )
	{
		strncpy( s_szError, text, ERROR_BUFFER_LENGTH );
		s_szError[ERROR_BUFFER_LENGTH-1] = 0;
	}

	const char*	GetErrorString() const
	{
		return s_szError;
	}

protected:

	int	GetErrorBufferSize()
	{
		return ERROR_BUFFER_LENGTH;
	}

private:
	char s_szError[ERROR_BUFFER_LENGTH];
};

// quick swap function
template< class T >
inline void QuickSwap( T &a, T &b )
{
	T c = a;
	a = b;
	b = c;
}

#endif // DKTYPES_H
