//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) - a C++ memory allocation debugger
//				designed to detect memory leaks and allocation errors
//
//				PPMEM performs array outrange check in your allocations
//				to perform easy debugging (NOTE : there is only 4 bytes last for
//				checking, only for cycles). If engine crashes it will show in console
//				allocation that was out of range, so it could help in debugging.
//				For checking page call 'PrintAllocMap()' and all info will be print to
//				standard console output, or for checking whole space use 'PPMemInfo()'
//				that is attached to 'ppmem_stats' console command
//
//////////////////////////////////////////////////////////////////////////////////

#include "ppmem.h"


#include <malloc.h>
#include <map>
#include "DebugInterface.h"
#include "IConCommandFactory.h"
#include "utils/strtools.h"
#include "utils/eqthread.h"

#include <stdio.h>

using namespace Threading;

//#define PPMEM_DISABLE
#define PPMEM_EXTRA_DEBUGINFO
#define PPMEM_CHECKMARK			(0x1df001ed)	// i'd fooled :D

struct ppallocinfo_t
{
	ppallocinfo_t()
	{
	}

#ifdef PPMEM_EXTRA_DEBUGINFO
	const char*	src;
	int			line;
#endif // PPMEM_EXTRA_DEBUGINFO

	uint		id;
	uint		size;

	uint		checkMark;
};

void cc_meminfo_f(DkList<EqString> *args)
{
	bool fullStats = false;

	if(CMD_ARGC > 0 && CMD_ARGV(0) == "full")
		fullStats = true;

	PPMemInfo( fullStats );
}

typedef std::map<void*,ppallocinfo_t*>::iterator allocIterator_t;

// allocation map
static std::map<void*,ppallocinfo_t*>	s_allocPointerMap;
static uint								s_allocIdCounter = 0;
static CEqMutex							s_allocMemMutex;

static ConCommand	ppmem_stats("ppmem_stats",cc_meminfo_f, "Memory info",CV_UNREGISTERED);
static ConVar		ppmem_break_on_alloc("ppmem_break_on_alloc", "-1", "Helps to catch allocation id at stack trace",CV_UNREGISTERED);

void PPMemInit()
{
    g_sysConsole->RegisterCommand(&ppmem_stats);
    g_sysConsole->RegisterCommand(&ppmem_break_on_alloc);

	//PPMemShutdown();
}

void PPMemShutdown()
{
    g_sysConsole->UnregisterCommand(&ppmem_stats);
    g_sysConsole->UnregisterCommand(&ppmem_break_on_alloc);


}

// Printing the statistics and tracked memory usage
void PPMemInfo( bool fullStats )
{
	CScopedMutex m(s_allocMemMutex);

	uint totalUsage = 0;
	uint numErrors = 0;

	for(allocIterator_t iterator = s_allocPointerMap.begin(); iterator != s_allocPointerMap.end(); iterator++)
	{
		ppallocinfo_t* alloc = iterator->second;
		void* curPtr = iterator->first;

		totalUsage += alloc->size;
	
		if(fullStats)
		{
#ifdef PPMEM_EXTRA_DEBUGINFO
			MsgInfo("alloc id=%d, src='%s' (%d), ptr=%p, size=%d\n", alloc->id, alloc->src, alloc->line, curPtr, alloc->size);
#else
			MsgInfo("alloc id=%d, ptr=%p, size=%d\n", alloc->id, curPtr, alloc->size);
#endif

			uint* checkMark = (uint*)((ubyte*)curPtr + alloc->size);

			if(alloc->checkMark != PPMEM_CHECKMARK || *checkMark != PPMEM_CHECKMARK)
			{
				MsgInfo(" ^^^ outranged ^^^\n");
				numErrors++;
			}
		}
		else
		{
			uint* checkMark = (uint*)((ubyte*)curPtr + alloc->size);

			if(alloc->checkMark != PPMEM_CHECKMARK || *checkMark != PPMEM_CHECKMARK)
				numErrors++;
		}
	}

	uint allocCount = s_allocPointerMap.size();

	MsgInfo("--- of %u allocactions, total usage: %.2f MB\n", allocCount, (totalUsage / 1024.0f) / 1024.0f);

	if(numErrors > 0)
		MsgWarning("%d allocations has overflow/underflow happened in runtime. Please print full stats to console\n", numErrors);
}

ppallocinfo_t* FindAllocation( void* ptr, bool& isValidInputPtr )
{
	if(ptr == nullptr)
	{
		isValidInputPtr = false;
		return NULL;
	}

	CScopedMutex m(s_allocMemMutex);

	if(s_allocPointerMap.count(ptr) > 0)
	{
		isValidInputPtr = true;
		return s_allocPointerMap[ptr];
	}
	else // try find the valid allocation in range
	{
		isValidInputPtr = false;

		for(allocIterator_t iterator = s_allocPointerMap.begin(); iterator != s_allocPointerMap.end(); iterator++)
		{
			ppallocinfo_t* curAlloc = iterator->second;
			void* curPtr = iterator->first;

			// if it's in range
			if(ptr >= curPtr && ptr <= (ubyte*)curPtr + curAlloc->size )
				return curAlloc; // return this allocation
		}
	}

	return nullptr;
}

// allocated debuggable memory block
void* PPDAlloc(uint size, const char* pszFileName, int nLine)
{
#ifdef PPMEM_DISABLE
	return malloc(size);
#else
	// allocate more to store extra information of this
	ppallocinfo_t* alloc = (ppallocinfo_t*)malloc(sizeof(ppallocinfo_t) + size + sizeof(uint));

	alloc->src = pszFileName;
	alloc->line = nLine;

	alloc->size = size;
	alloc->id = s_allocIdCounter++;

	// actual pointer address
	void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);

	uint* checkMark = (uint*)((ubyte*)actualPtr + size);

	// begin and end mark for checking
	alloc->checkMark = PPMEM_CHECKMARK;
	*checkMark = PPMEM_CHECKMARK;

	s_allocMemMutex.Lock();

	// store pointer in global map
	s_allocPointerMap[actualPtr] = alloc;

	s_allocMemMutex.Unlock();

	if( ppmem_break_on_alloc.GetInt() != -1)
		ASSERTMSG(alloc->id == (uint)ppmem_break_on_alloc.GetInt(), varargs("PPDAlloc: Break on allocation id=%d", alloc->id));

	return actualPtr;
#endif // PPMEM_DISABLE
}

// reallocates memory block
void* PPDReAlloc( void* ptr, uint size, const char* pszFileName, int nLine )
{
#ifdef PPMEM_DISABLE
	return realloc(ptr, size);
#else
	bool isValid = false;
	ppallocinfo_t* alloc = FindAllocation(ptr, isValid);

	if(alloc)
	{
		int ptrDiff = (ubyte*)ptr - ((ubyte*)alloc);
		ASSERTMSG(isValid, varargs("PPDReAlloc: Given pointer is invalid but allocation was found in the range.\nOffset is %d bytes.", ptrDiff));

		void* oldPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		s_allocPointerMap.erase(oldPtr);

		alloc = (ppallocinfo_t*)realloc(alloc, sizeof(ppallocinfo_t) + size + sizeof(uint));

		ASSERTMSG(alloc != nullptr, "PPDReAlloc: NULL pointer after realloc!");

		// set new size
		alloc->size = size;

		alloc->src = pszFileName;
		alloc->line = nLine;

		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);

		uint* checkMark = (uint*)((ubyte*)actualPtr + size);

		// store pointer in global map
		s_allocPointerMap[actualPtr] = alloc;

		// reset end mark for checking
		*checkMark = PPMEM_CHECKMARK;

		return actualPtr;
	}
	else
		return PPDAlloc(size, pszFileName, nLine);
#endif // PPMEM_DISABLE
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLE
	return free(ptr);
#else
	if(ptr == nullptr)
		return;

	bool isValid = false;
	ppallocinfo_t* alloc = FindAllocation(ptr, isValid);

	ASSERTMSG(alloc != nullptr, "PPFree ERROR: pointer is not valid or it's been already freed!");

	if(alloc)
	{
		int ptrDiff = (ubyte*)ptr - ((ubyte*)alloc);
		ASSERTMSG(isValid, varargs("PPFree: Given pointer is invalid but allocation was found in the range.\nOffset is %d bytes.", ptrDiff));

		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		uint* checkMark = (uint*)((ubyte*)actualPtr + alloc->size);

		ASSERTMSG(alloc->checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged before)");
		ASSERTMSG(*checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged after)");

		free(alloc);

		s_allocPointerMap.erase(actualPtr);
	}
#endif // PPMEM_DISABLE
}