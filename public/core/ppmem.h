//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

// source-line contailer
struct PPSourceLine
{
	uint64 data{ 0 };

	static PPSourceLine Empty();
	static PPSourceLine Make(const char* filename, int line);

	const char* GetFileName() const;
	int			GetLine() const;
};

// Source-line value constructor helper
template<typename T>
struct PPSLValueCtor
{
	T x;
	PPSLValueCtor<T>(const PPSourceLine& sl) : x() {}
};

#define PP_SL			PPSourceLine::Make(__FILE__, __LINE__)
#define	PPNew			new(PP_SL)
#define	PPNewSL(sl)		new(sl)

#define	PPAlloc(size)									PPDAlloc(size, PP_SL)
#define	PPAllocStructArray(type, count)					(type*)	PPDAlloc(count*sizeof(type), PP_SL)
#define	PPReAlloc(ptr, size)							PPDReAlloc(ptr, size, PP_SL)

#ifdef __clang__
#define PPNOEXCEPT noexcept
#else
#define PPNOEXCEPT
#endif

#ifndef NO_PPMEM_OP

void* operator new(size_t size);
void* operator new(size_t size, size_t alignment);
void* operator new[](size_t size);

void operator delete(void* ptr) PPNOEXCEPT;
void operator delete(void* ptr, size_t alignment) PPNOEXCEPT;
void operator delete[](void* ptr) PPNOEXCEPT;

void* operator new(size_t size, PPSourceLine sl);
void* operator new[](size_t size, PPSourceLine sl);

void operator delete(void* ptr, PPSourceLine sl) PPNOEXCEPT;
void operator delete[](void* ptr, PPSourceLine sl) PPNOEXCEPT;

#endif