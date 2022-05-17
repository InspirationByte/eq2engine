//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) -  memory allocation tracker
//////////////////////////////////////////////////////////////////////////////////

#ifndef PPMEM_H
#define PPMEM_H

#include <string.h>

#include "platform/Platform.h"
#include "InterfaceManager.h"

struct PPSourceLine;

IEXPORTS void	PPMemInfo( bool fullStats = true );

IEXPORTS void*	PPDAlloc( size_t size, const PPSourceLine& sl, const char* debugTAG = nullptr );
IEXPORTS void*	PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl, const char* debugTAG = nullptr );

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

#define PP_SL			PPSourceLine::Make(__FILE__, __LINE__)
#define	PPNew			new(PP_SL)
#define	PPNewSL(sl)		new(sl)

#define	PPAlloc(size)									PPDAlloc(size, PP_SL)
#define	PPAllocStructArray(type, count)					(type*)	PPDAlloc(count*sizeof(type), PP_SL)
#define	PPReAlloc(ptr, size)							PPDReAlloc(ptr, size, PP_SL)

#define	PPAllocTAG(size, tagSTR)						PPDAlloc(size, PP_SL, tagSTR)
#define	PPAllocStructArrayTAG(type, count, tagSTR)		(type*)	PPDAlloc(count*sizeof(type), PP_SL, tagSTR)
#define	PPReAllocTAG(ptr, size, tagSTR)					PPDReAlloc(ptr, size, PP_SL, tagSTR)

void* operator new(size_t size);
void* operator new(size_t size, size_t alignment);
void* operator new[](size_t size);

void operator delete(void* ptr);
void operator delete(void* ptr, size_t alignment);
void operator delete[](void* ptr);

void* operator new(size_t size, PPSourceLine sl);
void* operator new[](size_t size, PPSourceLine sl);

void operator delete(void* ptr, PPSourceLine sl);
void operator delete[](void* ptr, PPSourceLine sl);

// Source-line value constructor helper
template<typename T>
struct PPSLValueCtor
{
	T x;
	PPSLValueCtor<T>(const PPSourceLine& sl) : x() {}
};

#endif // PPMEM_H
