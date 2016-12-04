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

IEXPORTS void	PPMemInfo( bool fullStats = true );

IEXPORTS void*	PPDAlloc( uint size, const char* pszFileName, int nLine, const char* debugTAG = nullptr );
IEXPORTS void*	PPDReAlloc( void* ptr, uint size, const char* pszFileName, int nLine, const char* debugTAG = nullptr );

IEXPORTS void	PPFree( void* ptr );

#define			PPAlloc(size)						PPDAlloc(size, __FILE__, __LINE__)
#define			PPAllocStructArray(type, count)		(type*)	PPDAlloc(count*sizeof(type), __FILE__, __LINE__)
#define			PPReAlloc(ptr, size)				PPDReAlloc(ptr, size, __FILE__, __LINE__)

#define			PPAllocTAG(size, tagSTR)						PPDAlloc(size, __FILE__, __LINE__, tagSTR)
#define			PPAllocStructArrayTAG(type, count, tagSTR)		(type*)	PPDAlloc(count*sizeof(type), __FILE__, __LINE__, tagSTR)
#define			PPReAllocTAG(ptr, size, tagSTR)					PPDReAlloc(ptr, size, __FILE__, __LINE__, tagSTR)

// special macro to control your class allocations
// or if you want fast allocator

#define PPMEM_MANAGED_OBJECT_TAG( tagStr )		\
	static void* operator new (size_t size)		\
	{											\
		return PPAllocTAG( size, tagStr );		\
	}											\
	static void* operator new [] (size_t size)	\
	{											\
		return PPAllocTAG( size, tagStr );		\
	}											\
	void operator delete (void *p)				\
	{											\
		PPFree(p);								\
	}											\
	void operator delete[] (void *p)			\
	{											\
		PPFree(p);								\
	}

#define PPMEM_MANAGED_OBJECT()	PPMEM_MANAGED_OBJECT_TAG(nullptr)

#endif // PPMEM_H
