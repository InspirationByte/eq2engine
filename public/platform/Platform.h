//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef PLATFORM_H
#define PLATFORM_H

#include "dktypes.h"

//---------------------------------------------
// Platform definitions
//---------------------------------------------

#ifdef EQ_USE_SDL
#	define PLAT_SDL 1
#	include "SDL.h"
#	include "SDL_syswm.h"
#endif // EQ_USE_SDL

#ifdef _WIN32

#define PLAT_WIN 1

#ifdef _WIN64
#	define PLAT_WIN64 1
#else
#	define PLAT_WIN32 1
#endif // _WIN64

#else

#define PLAT_POSIX 1

#ifdef __APPLE__
#	define PLAT_OSX 1
#elif LINUX
#	define PLAT_LINUX 1
#elif __ANDROID__
#	define PLAT_ANDROID 1
#endif // __APPLE__

// TODO: android and ios
#endif // _WIN32

#if _MSC_VER >= 1400
// To make MSVC 2005 happy
#pragma warning (disable: 4996)
#  define assume(x) __assume(x)
#  define no_alias __declspec(noalias)
#else
#  define assume(x)
#  define no_alias
#endif

// Include some standard files
#ifdef PLAT_WIN

#   define WIN32_LEAN_AND_MEAN
#	undef _WIN32_WINNT
#	undef _WIN32_WINDOWS
#	define _WIN32_WINNT 0x0500
#	define _WIN32_WINDOWS 0x0500
#  ifndef WINVER
#     define WINVER 0x0500
#  endif
#  include <windows.h>

#elif defined(LINUX)


#  include <stdio.h>

#elif defined(__APPLE__)
#  include <X11/keysym.h>
#endif // _WIN32

//#include <limits.h>
#include <float.h>

#ifdef _MSC_VER
// Remove some warnings of warning level 4.
#pragma warning(disable : 4514) // warning C4514: 'acosl' : unreferenced inline function has been removed
#pragma warning(disable : 4100) // warning C4100: 'hwnd' : unreferenced formal parameter
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4512) // warning C4512: 'InFileRIFF' : assignment operator could not be generated
#pragma warning(disable : 4611) // warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable : 4706) // warning C4706: assignment within conditional expression
#pragma warning(disable : 4710) // warning C4710: function 'x' not inlined
#pragma warning(disable : 4702) // warning C4702: unreachable code
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4239) // nonstandard extension used : 'argument' ( conversion from class Vector to class Vector& )
#pragma warning(disable : 4097) // typedef-name 'BaseClass' used as synonym for class-name 'CFlexCycler::CBaseFlex'
#pragma warning(disable : 4324) // Padding was added at the end of a structure
#pragma warning(disable : 4244) // type conversion warning.
#pragma warning(disable : 4305)	// truncation from 'const double ' to 'float '
#pragma warning(disable : 4786)	// Disable warnings about long symbol names

#if _MSC_VER >= 1300
#pragma warning(disable : 4511)	// Disable warnings about private copy constructors
#endif // _MSC_VER

#endif // _MSC_VER

//------------------------------------------------------------------------------------------------

// Define some useful macros
#define MCHAR2(a, b) (a | (b << 8))
#define MCHAR4(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

#define _STR_(x) #x
#define DEFINE_STR(x) "#define " #x " " _STR_(x) "\n"

#define elementsOf(x)					(sizeof(x) / sizeof(x[0]))
#define offsetOf(structure,member)		(size_t)&(((structure *)0)->member)
#define elementSizeOf(structure,member)	sizeof(((structure *)0)->member)

#ifdef PLAT_WIN // maybe GCC?

#define forceinline __forceinline
#define _ALIGNED(x) __declspec(align(x))

#define ALIGNED_TYPE(s, a) typedef s _ALIGNED(a)

#else

#define forceinline inline
#define _ALIGNED(x) __attribute__ ((aligned(x)))

#define ALIGNED_TYPE(s, a) typedef struct s _ALIGNED(a)

#endif

// Abstract class identifier
#define abstract_class class

// Pure specifier
//#define PURE =0

//---------------------------------------------------------------------------------------------
// Message utilities
//---------------------------------------------------------------------------------------------

typedef void (*PREERRORMESSAGECALLBACK)( void );

void SetPreErrorCallback(PREERRORMESSAGECALLBACK callback);

void CrashMsg(const char* fmt, ...);
void ErrorMsg(const char* fmt, ...);
void WarningMsg(const char* fmt, ...);
void InfoMsg(const char* fmt, ...);

#ifndef _DKLAUNCHER_
// Heap checking
void __CheckHeap();
#endif

//---------------------------------------------------------------------------------------------
// Timer
//---------------------------------------------------------------------------------------------

// Platform QueryPerformanceCounter initializer
void	Platform_InitTime();

// returns current time since application is running
float	Platform_GetCurrentTime();

// sleeps the execution thread and let other processes to run for a specified amount of time.
void	Platform_Sleep(uint32 nMilliseconds);

//------------------------------------------------------------------------------------------------

#ifdef PLAT_WIN

#ifdef _MSC_VER
// Ensure proper handling of for-scope
#  if (_MSC_VER <= 1200)
#    define for if(0); else for
#  else
#    pragma conform(forScope, on)
#    pragma warning(disable: 4258)
#  endif
#endif

#else

#define stricmp(a, b) strcasecmp(a, b)

#endif // LINUX

//------------------------------------------------------------------------------------------------

void			outputDebugString(const char *str);
extern void		_InternalAssert(const char *file, int line, const char *statement);
extern void		AssertLogMsg(const char *fmt,...);

#define			ASSERT(b) if (!(b)) _InternalAssert(__FILE__, __LINE__, #b)
#define			ASSERTMSG(b, msg) if (!(b)) _InternalAssert(__FILE__, __LINE__, msg)

void			AssertValidReadPtr( void* ptr, int count = 1 );
void			AssertValidWritePtr( void* ptr, int count = 1 );
void			AssertValidStringPtr( const char* ptr, int maxchar = 0xFFFFFF  );
void			AssertValidWStringPtr( const wchar_t* ptr, int maxchar = 0xFFFFFF );
void			AssertValidReadWritePtr( void* ptr, int count = 1 );

//------------------------------------------------------------------------------------------------

#ifdef PLAT_WIN
// Alloca defined for this platform
#define  stackalloc( _size ) _alloca( _size )
#define  stackfree( _p )   0

#if _MSC_VER < 1900
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif // vsnprintf

#elif LINUX
// Alloca defined for this platform
#define  stackalloc( _size ) alloca( _size )
#define  stackfree( _p )   0
#endif

//------------------------------------------------------------------------------------------------

// define window type
#ifdef PLAT_SDL
typedef SDL_Window*		EQWNDHANDLE;	// better use void
typedef SDL_Cursor*		EQCURSOR;
#elif PLAT_WIN
typedef HWND			EQWNDHANDLE;
typedef HICON			EQCURSOR;
#else
typedef void*			EQWNDHANDLE;
typedef void*			EQCURSOR;
#endif // PLAT_SDL

#endif // PLATFORM_H
