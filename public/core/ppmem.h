//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) -  memory allocation tracker
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct PPSourceLine;

IEXPORTS void	PPMemInfo( bool fullStats = true );
IEXPORTS size_t	PPMemGetUsage();

IEXPORTS void*	PPDAlloc( size_t size, const PPSourceLine& sl );
IEXPORTS void*	PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl );

IEXPORTS void	PPFree( void* ptr );

#define	PPAlloc(size)									PPDAlloc(size, PP_SL)
#define	PPAllocStructArray(type, count)					(type*)	PPDAlloc(count*sizeof(type), PP_SL)
#define	PPReAlloc(ptr, size)							PPDReAlloc(ptr, size, PP_SL)

#ifdef PPMEM_DISABLED

#define	PPNew			new
#define	PPNewSL(sl)		new

#else

#define	PPNew			new(PP_SL)
#define	PPNewSL(sl)		new(sl)

#endif // PPMEM_DISABLED

#ifdef __clang__
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