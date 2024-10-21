//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) -  memory allocation tracker
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ppsourceline.h"

IEXPORTS void	PPMemInfo( bool fullStats = true );
IEXPORTS size_t	PPMemGetUsage();

IEXPORTS void*	PPDAlloc( size_t size, const PPSourceLine& sl );
IEXPORTS void*	PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl );
IEXPORTS void	PPDCheck( void* ptr );

IEXPORTS void	PPFree( void* ptr );

#define	PPAlloc(size)						PPDAlloc(size, PP_SL)
#define	PPAllocStructArray(type, count)		reinterpret_cast<type*>(PPDAlloc(count*sizeof(type), PP_SL))
#define	PPReAlloc(ptr, size)				PPDReAlloc(ptr, size, PP_SL)

#define	PPAllocStructArrayRef(type, count)	ArrayRef(PPAllocStructArray(type, count), count)
#define	PPFreeArrayRef(ref)					PPFree(ref.ptr()), ref = nullptr

// Source-line value constructor helper
template<typename T> struct PPSLValueCtor
{
	T x;
	PPSLValueCtor<T>(const PPSourceLine& sl) : x() {}
};

// Source-line placement new wrapper for default constructor
template<typename T> void PPSLPlacementNew(void* item, const PPSourceLine& sl) { new(item) PPSLValueCtor<T>(sl); }

#ifdef PPMEM_DISABLED

#define PP_SL			PPSourceLine::Empty()
#define PPNew			new
#define PPNewSL(sl)		new

#else

#define PP_SL			PPSourceLine::Make(__FILE__, __LINE__)
#define PPNew			new(PP_SL)
#define PPNewSL(sl)		new(sl)

#endif // PPMEM_DISABLED

#define	PPNewArrayRef(type, count)	ArrayRef(PPNew type[count], count)
#define PPDeleteArrayRef(ref)		delete [] ref.ptr(), ref = nullptr

#if defined(__clang__)
#define PPNOEXCEPT noexcept
#else
#define PPNOEXCEPT
#endif

#if !defined(NO_PPMEM_OP) && !defined(PPMEM_DISABLED)

void* operator new(size_t size);
void* operator new(size_t size, size_t alignment);
void* operator new[](size_t size);

void operator delete(void* ptr) PPNOEXCEPT;
void operator delete(void* ptr, size_t alignment) PPNOEXCEPT;
void operator delete[](void* ptr) PPNOEXCEPT;

void* operator new(size_t size, PPSourceLine sl) PPNOEXCEPT;
void* operator new[](size_t size, PPSourceLine sl) PPNOEXCEPT;

void operator delete(void* ptr, PPSourceLine sl) PPNOEXCEPT;
void operator delete[](void* ptr, PPSourceLine sl) PPNOEXCEPT;

#endif // #if !NO_PPMEM_OP && !PPMEM_DISABLED