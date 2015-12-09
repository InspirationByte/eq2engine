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
//
//		How memory is put:
//		 __________________________________            ____________________________
//		|----data----|endmark|ppmem_alloc_t|~~~FREE~~~|-data-|endmark|ppmem_alloc_t|
//		|____________|_4byte_|_____________|~~~SPACE~~|______|_4byte_|_____________|...
//
//		ppmem_alloc_t only put if PPMEM_INTERNAL_ALLOC is on.
//
//	[07/06/14] - fixed possible page deallocation bug
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef PPMEM_H
#define PPMEM_H

#include <string.h>

#include "Platform.h"
#include "InterfaceManager.h"

#include "utils/eqthread.h"

#define DEFAULT_PAGE_SIZE	(32*1024*1024) // 32 megabyte

struct ppmem_alloc_t;

// memory page class
class CPPMemPage
{
public:
	CPPMemPage();
	CPPMemPage(uint page_size);

	~CPPMemPage();

	void			FreePage();

	// allocates memory in that page
	void*			Alloc(uint size, const char* pszFileName = NULL, int nLine = 0);

	// frees memory
	bool			Free(void* ptr);

	// resizes allocation or finds new one
	void*			ReAlloc(void* ptr, uint newsize, const char* pszFileName = NULL, int nLine = 0);

	ubyte*			GetBase();

	uint			GetPageSize();
	uint			GetUsage();

	uint			PrintAllocMap( bool fullStats );
	uint			CheckPage();

	CPPMemPage*		m_pNextPage;

protected:
	ppmem_alloc_t*	FindBestMemLoc(uint size);
	void			RemoveAlloc( ppmem_alloc_t* cur, ppmem_alloc_t* prev );

	ubyte*			m_pBase;

	uint			m_nPageSize;

	ppmem_alloc_t*	m_pAllocations;

	Threading::CEqInterlockedInteger	m_nLowAllocated;
	Threading::CEqInterlockedInteger	m_nAllocations;
};

IEXPORTS void	PPMemInfo( bool fullStats = true );

IEXPORTS void*	PPDAlloc( uint size, const char* pszFileName, int nLine );
IEXPORTS void*	PPDReAlloc( void* ptr, uint size, const char* pszFileName, int nLine );

IEXPORTS void	PPFree( void* ptr );

#define			PPAlloc(size)						PPDAlloc(size, __FILE__, __LINE__)
#define			PPAllocStructArray(type, count)		(type*)	PPDAlloc(count*sizeof(type), __FILE__, __LINE__)
#define			PPReAlloc(ptr, size)				PPDReAlloc(ptr, size, __FILE__, __LINE__)

// special macro to control your class allocations
// or if you want fast allocator
#define PPMEM_MANAGED_OBJECT()					\
	static void* operator new (size_t size)		\
	{											\
		return PPAlloc( size );					\
	}											\
	static void* operator new [] (size_t size)	\
	{											\
		return PPAlloc( size );					\
	}											\
	void operator delete (void *p)				\
	{											\
		PPFree(p);								\
	}											\
	void operator delete[] (void *p)			\
	{											\
		PPFree(p);								\
	}

#endif // PPMEM_H
