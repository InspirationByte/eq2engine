//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) - a C++ memory allocation library
//				designed to detect memory leaks and allocation errors
//
//				The PPMem uses 'pages' that allocates once. Default page size is 32mb,
//				but you can specify needed size by yourself.
//				You can use automatic page allocator that can be called by PPDAlloc,
//				but also you can create memory page manually using CPPMemPage class
//				and use that for your own procedures.
//
//				Also, PPMEM performs array outrange check in your allocations
//				to perform easy debugging (NOTE : there is only 4 bytes last for
//				checking, only for cycles). If engine crashes it will show in console
//				allocation that was out of range, so it could help in debugging.
//				For checking page call 'PrintAllocMap()' and all info will be print to
//				standard console output, or for checking whole space use 'PPMemInfo()'
//				that is attached to 'ppmem_stats' console command
//
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
	uint64 data;

	static PPSourceLine Make(const char* filename, int line);

	const char* GetFileName() const;
	int			GetLine() const;
};

#define PP_SL	PPSourceLine::Make(__FILE__, __LINE__)
#define			PPNew	new(PP_SL)

#define			PPAlloc(size)									PPDAlloc(size, PP_SL)
#define			PPAllocStructArray(type, count)					(type*)	PPDAlloc(count*sizeof(type), PP_SL)
#define			PPReAlloc(ptr, size)							PPDReAlloc(ptr, size, PP_SL)

#define			PPAllocTAG(size, tagSTR)						PPDAlloc(size, PP_SL, tagSTR)
#define			PPAllocStructArrayTAG(type, count, tagSTR)		(type*)	PPDAlloc(count*sizeof(type), PP_SL, tagSTR)
#define			PPReAllocTAG(ptr, size, tagSTR)					PPDReAlloc(ptr, size, PP_SL, tagSTR)

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

#endif // PPMEM_H
