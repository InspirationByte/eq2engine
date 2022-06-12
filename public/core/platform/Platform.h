//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//---------------------------------------------
// Platform definitions
//---------------------------------------------

#include "platformdefs.h"

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
#	define _WIN32_WINDOWS 0x0500
#  ifndef WINVER
#     define WINVER 0x0500
#  endif
#include <Windows.h>
#ifdef FreeModule
#undef FreeModule
#endif
#ifdef FreeModule
#undef FreeModule
#endif

#elif defined(PLAT_LINUX) || defined(PLAT_ANDROID)
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

#endif // PLAT_WIN

//------------------------------------------------------------------------------------------------

// define window type
#ifdef PLAT_SDL
struct SDL_Window;
struct SDL_Cursor;

#define EQWNDHANDLE		SDL_Window*		
#define EQCURSOR		SDL_Cursor*		
#elif PLAT_WIN
typedef HWND			EQWNDHANDLE;
typedef HICON			EQCURSOR;
#else
typedef void*			EQWNDHANDLE;
typedef void*			EQCURSOR;
#endif // PLAT_SDL

IEXPORTS void Platform_Sleep(uint32 nMilliseconds);
