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

#include "ppmem.h"


#include <malloc.h>
#include "DebugInterface.h"
#include "IConCommandFactory.h"
#include "utils/eqstring.h"
#include "utils/DkList.h"
#include "utils/eqthread.h"

#pragma todo("Last node priority optimization (for allocations smallest than 10 kb)")

#include <stdio.h>

using namespace Threading;

//#define PPMEM_DISABLE
#define PPMEM_EXTRA_DEBUGINFO

// debug mode disables internal allocating of ppmem_alloc_t to ease the process of finding memory errors
#ifndef _DEBUG
#	define PPMEM_INTERNAL_ALLOC // full internal memory allocation
//#	define PPMEM_USE_JEMALLOC
#else
#	define PPMEM_DISABLE
//#	define PPMEM_EXTRA_DEBUGINFO
#endif // _DEBUG

#ifdef PPMEM_USE_JEMALLOC
#include <jemalloc/jemalloc.h>

#define PPMEM_DISABLE
#endif // PPMEM_USE_JEMALLOC

#define PPMEM_ENDMARK (0x1df001ed)	// i'd fooled :D

// pages
CPPMemPage* g_pFirstPage = NULL;
static uint g_nAllocIdCounter = 0;

CEqMutex g_MemMutex;

struct ppmem_alloc_t
{
	uint	offset;
	uint	size;
	//uint	reserve;
	uint	freespace_beyond;

	void*	ptr;

#ifdef PPMEM_EXTRA_DEBUGINFO
	// extra information
	char*	src;
	int		line;
#endif // PPMEM_EXTRA_DEBUGINFO

	ppmem_alloc_t* next;

	int		id;
};

void cc_meminfo_f(DkList<EqString> *args)
{
	bool fullStats = false;

	if(CMD_ARGC > 0 && CMD_ARGV(0) == "full")
	{
		fullStats = true;
	}

	PPMemInfo( fullStats );
}

static ConCommand	ppmem_stats("ppmem_stats",cc_meminfo_f, "Memory info",0);

static ConVar		ppmem_break_on_alloc("ppmem_break_on_alloc", "-1", "Helps to catch allocation id at stack trace",0);

static ConVar		ppmem_freecheck("ppmem_freecheck", "0", "Checks memory validty before freeing",0);

CPPMemPage::CPPMemPage()
{
	m_nPageSize = DEFAULT_PAGE_SIZE;

	m_pBase = (ubyte*)malloc( DEFAULT_PAGE_SIZE );

	m_nAllocations.SetValue(0);
	m_nLowAllocated.SetValue(0);
	m_pAllocations = NULL;

	m_pNextPage = NULL;
}

CPPMemPage::CPPMemPage(uint page_size)
{
	m_nPageSize = page_size;

	m_pBase = (ubyte*)malloc( page_size );
	m_nAllocations.SetValue(0);
	m_pAllocations = NULL;

	m_nLowAllocated.SetValue(0);

	m_pNextPage = NULL;
}

CPPMemPage::~CPPMemPage()
{
	FreePage();
}

void CPPMemPage::FreePage()
{
	if(m_pBase)
		free( m_pBase );

	m_pBase = NULL;

#ifndef PPMEM_INTERNAL_ALLOC
	ppmem_alloc_t* pAlloc = m_pAllocations;

	// get a pointer, free it and proceed
	while(pAlloc)
	{
		ppmem_alloc_t* ptr = pAlloc;
		pAlloc = pAlloc->next;

		delete ptr;
	}
#endif // PPMEM_INTERNAL_ALLOC
}

ppmem_alloc_t* CPPMemPage::FindBestMemLoc(uint size)
{
	CScopedMutex m(g_MemMutex);

#ifdef PPMEM_INTERNAL_ALLOC
	ppmem_alloc_t tempAlloc;
	ppmem_alloc_t* new_alloc = &tempAlloc;
#else
	ppmem_alloc_t* new_alloc = new ppmem_alloc_t;
#endif // PPMEM_INTERNAL_ALLOC

#ifdef PPMEM_EXTRA_DEBUGINFO
	// extra information
	new_alloc->src = NULL;
	new_alloc->line = 0;
#endif // PPMEM_EXTRA_DEBUGINFO

	// get a new pointer
	new_alloc->ptr = NULL;
	new_alloc->offset = 0;
	new_alloc->size = size;
	new_alloc->next = NULL;
	new_alloc->freespace_beyond = 0;

	// add a end mark for checking and put our alloc info at the end
#ifdef PPMEM_INTERNAL_ALLOC
	new_alloc->size += sizeof(int) + sizeof(ppmem_alloc_t);
#else
	new_alloc->size += sizeof(int);
#endif // PPMEM_INTERNAL_ALLOC

	// push back newest allocation
	if(!m_pAllocations)
	{
		m_nLowAllocated.SetValue(new_alloc->offset + new_alloc->size);
		new_alloc->freespace_beyond = m_nPageSize-(m_nLowAllocated.GetValue()+size);
	// add a end mark for checking and put our alloc info at the end

#ifdef PPMEM_INTERNAL_ALLOC
		ppmem_alloc_t* pAllocPut = (ppmem_alloc_t*)(m_pBase + new_alloc->offset + new_alloc->size - sizeof(ppmem_alloc_t));
		memcpy(pAllocPut, new_alloc, sizeof(ppmem_alloc_t));

		m_pAllocations = pAllocPut;

		return pAllocPut;
#else
		m_pAllocations = new_alloc;

		return new_alloc;
#endif // PPMEM_INTERNAL_ALLOC
	}
	else
	{
		ppmem_alloc_t* pAlloc = m_pAllocations;

		int i = 0;

		while(pAlloc)
		{
			bool bShouldPutFirst = ((pAlloc == m_pAllocations) && (pAlloc->offset >= new_alloc->size));

			if(bShouldPutFirst)
			{
				new_alloc->freespace_beyond = m_pAllocations->offset - new_alloc->size;
				new_alloc->offset = 0;
				new_alloc->next = m_pAllocations;

#ifdef PPMEM_INTERNAL_ALLOC
				ppmem_alloc_t* pAllocPut = (ppmem_alloc_t*)(m_pBase + new_alloc->offset + new_alloc->size - sizeof(ppmem_alloc_t));
				memcpy(pAllocPut, new_alloc, sizeof(ppmem_alloc_t));

				m_pAllocations = pAllocPut;
				return pAllocPut;
#else
				m_pAllocations = new_alloc;
				return new_alloc;
#endif // PPMEM_INTERNAL_ALLOC


			}

			if(pAlloc->freespace_beyond >= new_alloc->size)
			{
				// if this is last node
				if(!pAlloc->next)
				{
					// add last
					new_alloc->freespace_beyond = pAlloc->freespace_beyond - new_alloc->size;
					pAlloc->freespace_beyond = 0;

					new_alloc->offset = pAlloc->offset + pAlloc->size;

					// only this sets up low allocated
					m_nLowAllocated.SetValue(new_alloc->offset + new_alloc->size);

#ifdef PPMEM_INTERNAL_ALLOC
					ppmem_alloc_t* pAllocPut = (ppmem_alloc_t*)(m_pBase + new_alloc->offset + new_alloc->size - sizeof(ppmem_alloc_t));
					memcpy(pAllocPut, new_alloc, sizeof(ppmem_alloc_t));

					pAlloc->next = pAllocPut;
					return pAllocPut;
#else
					pAlloc->next = new_alloc;
					return new_alloc;
#endif	// PPMEM_INTERNAL_ALLOC


				}
				else if(pAlloc->offset + pAlloc->size + new_alloc->size <= pAlloc->next->offset)
				{
					// insertion
					new_alloc->next = pAlloc->next;

					new_alloc->freespace_beyond = pAlloc->freespace_beyond - new_alloc->size;

					pAlloc->freespace_beyond = 0;

					new_alloc->offset = pAlloc->offset + pAlloc->size;

#ifdef PPMEM_INTERNAL_ALLOC
					ppmem_alloc_t* pAllocPut = (ppmem_alloc_t*)(m_pBase + new_alloc->offset + new_alloc->size - sizeof(ppmem_alloc_t));
					memcpy(pAllocPut, new_alloc, sizeof(ppmem_alloc_t));

					pAlloc->next = pAllocPut;
					return pAllocPut;
#else
					pAlloc->next = new_alloc;
					return new_alloc;
#endif	// PPMEM_INTERNAL_ALLOC
				}
			}

			i++;
			pAlloc = pAlloc->next;
		}
	}

	//delete new_alloc;

	// nothing allocated, return base
	return NULL;
}

// allocates memory in that page
void* CPPMemPage::Alloc(uint size, const char* pszFileName, int nLine)
{
	ppmem_alloc_t* new_alloc = FindBestMemLoc( size );

	if(!new_alloc)
		return NULL;

	// extra information
#ifdef PPMEM_EXTRA_DEBUGINFO
	new_alloc->src = (char*)pszFileName;
	new_alloc->line = nLine;
#endif // PPMEM_EXTRA_DEBUGINFO

	/*
	new_alloc->reserve = reserve;

	int res_max_point = new_alloc->reserve + (new_alloc->offset + new_alloc->size);

	if(res_max_point > m_nPageSize)
		new_alloc->reserve -= res_max_point - m_nPageSize;
		*/

	// get a new pointer
	new_alloc->ptr = (m_pBase + new_alloc->offset);

#ifdef _DEBUG
	// zero the memory
	memset(new_alloc->ptr,0, new_alloc->size - sizeof(int));
#endif

#ifdef PPMEM_INTERNAL_ALLOC
	int* endmark_ptr = (int*)((ubyte*)new_alloc->ptr + new_alloc->size - (sizeof(int) + sizeof(ppmem_alloc_t)));
#else
	int* endmark_ptr = (int*)((ubyte*)new_alloc->ptr + new_alloc->size - sizeof(int));
#endif // PPMEM_INTERNAL_ALLOC

	*endmark_ptr = PPMEM_ENDMARK;

	// increment allocation count
	new_alloc->id = g_nAllocIdCounter++;
	m_nAllocations.Increment();

	if(ppmem_break_on_alloc.GetInt() != -1 && new_alloc->id == ppmem_break_on_alloc.GetInt())
	{
		PPMemInfo();
		ASSERTMSG(false, "Debug break for c_catch_alloc. Look at the stack");
	}

	// TODO: User assert

	return new_alloc->ptr;
}

#define CHECK_ALLOC_INRANGE(alloc, ptr) ((uint64)pAlloc->ptr >= (uint64)ptr && (uint64)ptr <= ((uint64)pAlloc->ptr + pAlloc->size))

// frees memory
bool CPPMemPage::Free(void* ptr)
{
	if(ptr == NULL)
		return true;

	CScopedMutex m(g_MemMutex);

	ppmem_alloc_t* pAlloc = m_pAllocations;
	ppmem_alloc_t* pPrev = NULL;

	if(ppmem_freecheck.GetBool())
	{
		uint invalid_allocs = CheckPage();
		if(invalid_allocs)
		{
			MsgError("PPMem detected out of range error (%u)\n", invalid_allocs);
			PPMemInfo();
			ASSERT(!"Memory error, break\n");
		}
	}

	// get a pointer, free it and proceed
	while(pAlloc)
	{
		if (CHECK_ALLOC_INRANGE(pAlloc, ptr))
		{
			if (pAlloc->ptr != ptr)
				ASSERTMSG(false, "CPPMemPage::Free - pointer is valid, but not exactly as it was allocated");

			RemoveAlloc(pAlloc, pPrev);

			// we successfully deallocated memory part
			return true;
		}

		pPrev = pAlloc;
		pAlloc = pAlloc->next;
	}

	return false;
}

void CPPMemPage::RemoveAlloc( ppmem_alloc_t* cur, ppmem_alloc_t* prev )
{
	//Msg("PPFree: unlink ptr=%p\n", cur->ptr);

	// assign valid pointer before deletion
	if(cur == m_pAllocations)
	{
		m_pAllocations = cur->next;
	}
	else if(prev != NULL)
	{
		prev->next = cur->next;

		if(prev->next)
			prev->next->freespace_beyond += cur->size;
	}

	m_nAllocations.Decrement();

	if(!cur->next)
	{
		m_nLowAllocated.Sub(cur->size);
		if(prev)
			prev->freespace_beyond = m_nPageSize-m_nLowAllocated.GetValue();
	}

	if(m_nAllocations.GetValue() == 0)
	{
		// if this is root
		if( cur == m_pAllocations )
			m_pAllocations = NULL;

		m_nLowAllocated.SetValue(0);
	}

#ifndef PPMEM_INTERNAL_ALLOC
	// dealloc
	delete cur;
#endif // PPMEM_INTERNAL_ALLOC
}

// resizes allocation or finds new one
void* CPPMemPage::ReAlloc(void* ptr, uint newsize, const char* pszFileName, int nLine)
{
#pragma todo("better reallocation by extending allocation size if possible")

	ppmem_alloc_t* pAlloc = m_pAllocations;
	ppmem_alloc_t* pPrev = NULL;

	bool bIsOk = false;

	if(ptr)
	{
		// get a pointer, free it and proceed
		while(pAlloc)
		{
			if(pAlloc->ptr == ptr)
			{
				bIsOk = true;

				g_MemMutex.Lock();
				// pointer needs to be relocated
				RemoveAlloc( pAlloc, pPrev );

				g_MemMutex.Unlock();
				break;
			}

			g_MemMutex.Lock();

			pPrev = pAlloc;
			pAlloc = pAlloc->next;

			g_MemMutex.Unlock();
		}

		if(!bIsOk)
			return NULL;
	}

	// not found placement by old ptr, find new one
	ppmem_alloc_t* new_alloc = FindBestMemLoc( newsize );

	if(!new_alloc)
		return NULL;

#ifdef PPMEM_EXTRA_DEBUGINFO
	// extra information
	new_alloc->src = (char*)pszFileName;
	new_alloc->line = nLine;
#endif // PPMEM_EXTRA_DEBUGINFO

	// get a new pointer
	new_alloc->ptr = (m_pBase + new_alloc->offset);

	/*

	Don't do this.

	if(ptr)
	{
		// copy the old memory to new
		int copySize = pAlloc->size - (sizeof(int) + sizeof(ppmem_alloc_t));

		if(copySize > newsize)
			copySize = newsize;

		memcpy(new_alloc->ptr, pAlloc->ptr, copySize);
	}*/

	// debug purpose only
	//memset(new_alloc->ptr,0, new_alloc->size);

	// increment allocation count
	new_alloc->id = g_nAllocIdCounter++;
	m_nAllocations.Increment();

#ifdef PPMEM_INTERNAL_ALLOC
	int* endmark_ptr = (int*)((ubyte*)new_alloc->ptr + new_alloc->size - (sizeof(int) + sizeof(ppmem_alloc_t)));
#else
	int* endmark_ptr = (int*)((ubyte*)new_alloc->ptr + new_alloc->size - sizeof(int));
#endif // PPMEM_INTERNAL_ALLOC

	*endmark_ptr = PPMEM_ENDMARK;

	return new_alloc->ptr;
}

uint CPPMemPage::PrintAllocMap( bool fullStats )
{
	ppmem_alloc_t* pAlloc = m_pAllocations;

	Msg("Alloc counter: %d\n", m_nAllocations.GetValue());

	uint memUsage = 0;
	// get a pointer, free it and proceed
	while(pAlloc)
	{
		if(fullStats)
		{
#ifdef PPMEM_EXTRA_DEBUGINFO
			MsgInfo("alloc id=%d, src='%s' (%d), ptr=%p, size=%d, fsb=%d\n", pAlloc->id, pAlloc->src, pAlloc->line, pAlloc->ptr, pAlloc->size, pAlloc->freespace_beyond);
#else
			MsgInfo("alloc id=%d, ptr=%p, size=%d, fsb=%d\n", pAlloc->id, pAlloc->ptr, pAlloc->size, pAlloc->freespace_beyond);
#endif
		}

#ifdef PPMEM_INTERNAL_ALLOC
		int* endmark_ptr = (int*)((ubyte*)pAlloc->ptr + pAlloc->size - (sizeof(int) + sizeof(ppmem_alloc_t)));
#else
		int* endmark_ptr = (int*)((ubyte*)pAlloc->ptr + pAlloc->size - sizeof(int));
#endif // PPMEM_INTERNAL_ALLOC

		if(fullStats)
		{
			if(*endmark_ptr != PPMEM_ENDMARK)
				MsgInfo(" ^^^ somewhere outranged ^^^\n");
		}
		else
		{
			if(*endmark_ptr != PPMEM_ENDMARK)
				MsgInfo("Memory page has error. Please print full stats to console\n");
		}

		memUsage += pAlloc->size;

		pAlloc = pAlloc->next;
	}

	MsgInfo("--- of %u allocs, page usage: %.2f MB, page size: %.2f MB\n", m_nAllocations.GetValue(), (memUsage / 1024.0f) / 1024.0f, (m_nPageSize / 1024.0f)/1024.0f);

	return memUsage;
}

uint CPPMemPage::CheckPage()
{
	ppmem_alloc_t* pAlloc = m_pAllocations;

	uint numInvalid = 0;

	// get a pointer, free it and proceed
	while(pAlloc)
	{
#ifdef PPMEM_INTERNAL_ALLOC
		int* endmark_ptr = (int*)((ubyte*)pAlloc->ptr + pAlloc->size - (sizeof(int) + sizeof(ppmem_alloc_t)));
#else
		int* endmark_ptr = (int*)((ubyte*)pAlloc->ptr + pAlloc->size - sizeof(int));
#endif // PPMEM_INTERNAL_ALLOC

		// also check page
		if ((pAlloc->ptr < m_pBase) ||
			pAlloc->ptr > m_pBase+m_nPageSize)
		{
			numInvalid++;
		}

		if(*endmark_ptr != PPMEM_ENDMARK)
			numInvalid++;

		pAlloc = pAlloc->next;
	}

	return numInvalid;
}

ubyte* CPPMemPage::GetBase()
{
	return m_pBase;
}

uint CPPMemPage::GetPageSize()
{
	return m_nPageSize;
}

uint CPPMemPage::GetUsage()
{
	return m_nLowAllocated.GetValue();
}

void PPMemInit()
{
#ifdef PPMEM_DISABLE

#	ifdef PPMEM_USE_JEMALLOC
	je_init();
#	else
	MsgInfo("PPMEM is disabled\n");
#	endif

#endif // PPMEM_DISABLE
	//PPMemShutdown();
}

void PPMemShutdown()
{
#ifdef PPMEM_USE_JEMALLOC
	je_uninit();

	if(!g_pFirstPage)
		return;

	CPPMemPage* pPage = g_pFirstPage;

	// free pages
	while(pPage)
	{
		CPPMemPage* pRemoval = pPage;

		pPage = pPage->m_pNextPage;
		delete pRemoval;
	}

	g_pFirstPage = NULL;
#endif // PPMEM_USE_JEMALLOC
}

void jeStatsPrinterFn(void *ioPtr,const char * str)
{
	MsgInfo( str );
}

void PPMemInfo( bool fullStats )
{
	// no any useful information in je_malloc_stats_print

#ifndef PPMEM_USE_JEMALLOC
	if(!g_pFirstPage)
	{
		return;
	}

	CPPMemPage* pPage = g_pFirstPage;

	uint memUsage = 0;
	int nPage = 0;

	MsgInfo("--- PPMEM Memory usage information\n");

	// find placement in page
	while(pPage)
	{
		MsgInfo("--- PPMEM Page #%d\n", nPage);

		memUsage += pPage->PrintAllocMap( fullStats );

		int nOutranged = pPage->CheckPage();
		if(nOutranged)
			MsgInfo("Page was outranged in %d allocations!\n", nOutranged);

		nPage++;

		pPage = pPage->m_pNextPage;
	}

	Msg("*** Total usage: %.2f MB\n", (memUsage / 1024.0f)/1024.0f);
	Msg("*** Pages allocated: %d\n", nPage);

	Msg("\n Use 'ppmem_break_on_alloc <allocation id>' variable to trace stack at next app. launch\n");
#endif // PPMEM_USE_JEMALLOC
}

CPPMemPage* AllocPage(uint req_size)
{
	uint size = req_size;

	if(size < DEFAULT_PAGE_SIZE)
		size = DEFAULT_PAGE_SIZE;

	//printf("Page allocated with size %d\n", size);

	return new CPPMemPage( size );
}

void* PPDAlloc(uint size, const char* pszFileName, int nLine)
{
#ifdef PPMEM_DISABLE

#	ifdef PPMEM_USE_JEMALLOC
		return je_malloc(size);
#	else
		return malloc(size);
#	endif // PPMEM_USE_JEMALLOC

#else
	
	//if(size >= g_maxSizeAlloc)
	//{
	//	ASSERT(!"PPAlloc break: trying to allocate over 2GB of memory! Programmer, please check the code for unitialized variables!");
	//}

	int required_size = size + sizeof(int);

#ifdef PPMEM_INTERNAL_ALLOC
	required_size += sizeof(ppmem_alloc_t);
#endif

	if(!g_pFirstPage)
	{
		CScopedMutex m(g_MemMutex);
		g_pFirstPage = AllocPage( required_size );
	}

	CPPMemPage* pPage = g_pFirstPage;

	CScopedMutex m(g_MemMutex);

	// find placement in page
	while(pPage)
	{
		if(pPage->GetUsage() + size < pPage->GetPageSize())
		{
			void* pPtr = pPage->Alloc( size, pszFileName, nLine );

			if(pPtr)
				return pPtr;
		}

		if(!pPage->m_pNextPage)
			pPage->m_pNextPage = AllocPage( required_size );

		pPage = pPage->m_pNextPage;
	}

	return NULL;
#endif // PPMEM_DISABLE
}

void* PPDReAlloc( void* ptr, uint size, const char* pszFileName, int nLine )
{
#ifdef PPMEM_DISABLE

#	ifdef PPMEM_USE_JEMALLOC
		return je_realloc(ptr, size);
#	else
		return realloc(ptr, size);
#	endif // PPMEM_USE_JEMALLOC

#else

	int required_size = size + sizeof(int);

#ifdef PPMEM_INTERNAL_ALLOC
	required_size += sizeof(ppmem_alloc_t);
#endif

	if(!g_pFirstPage)
	{
		CScopedMutex m(g_MemMutex);
		g_pFirstPage = AllocPage( required_size );
	}

	if(!ptr)
	{
		//Msg("PPDReAlloc: early alloc %s at %d\n", pszFileName, nLine);
		return PPDAlloc( size, pszFileName, nLine );
	}

	CPPMemPage* pPage = g_pFirstPage;

	void* pPtr = NULL;

	// find placement in page
	while(pPage)
	{
		CScopedMutex m(g_MemMutex);

		// is in range of this page?
		if( (ptr >= pPage->GetBase()) && (ptr <= pPage->GetBase()+pPage->GetPageSize()) )
		{
			pPtr = pPage->ReAlloc( ptr, size, pszFileName, nLine );

			//Msg("PPDReAlloc: try realloc %s at %d\n", pszFileName, nLine);

			// found and reallocated
			if(pPtr)
				break;
		}

		pPage = pPage->m_pNextPage;
	}

	// if we're failed to reallocate in existing pages, create in standard way
	if(!pPtr)
	{
		//Msg("PPDReAlloc: late alloc %s at %d\n", pszFileName, nLine);
		return PPDAlloc( size, pszFileName, nLine );
	}

	return pPtr;
#endif // PPMEM_DISABLE
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLE

#	ifdef PPMEM_USE_JEMALLOC
		return je_free(ptr);
#	else
		return free(ptr);
#	endif // PPMEM_USE_JEMALLOC

#else
	if(ptr == NULL)
		return;

	CPPMemPage* pPage = g_pFirstPage;

	CPPMemPage* datPage = NULL;

	// find placement in page
	while( pPage )
	{
		if(ptr >= pPage->GetBase() && ptr <= pPage->GetBase()+ pPage->GetPageSize())
		{
			datPage = pPage;
			break;
		}

		pPage = pPage->m_pNextPage;
	}

	if(datPage)
	{
		if(!datPage->Free( ptr ))
			ASSERTMSG(false, "PPFree: got invalid address for free");
	}
	else
		ASSERTMSG(false, "PPFree: bad alloc or not ppmem allocation");

#endif // PPMEM_DISABLE
}
