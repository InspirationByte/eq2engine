//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

#include "core/ppmem.h"

#include "core/DebugInterface.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "utils/strtools.h"
#include "utils/eqthread.h"

#include <stdio.h>
#include <malloc.h>
#include <unordered_map>

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
#define pp_internal_malloc(s)	_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#else
#define pp_internal_malloc(s)	malloc(s)
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

using namespace Threading;

//#define PPMEM_DISABLE
#define PPMEM_EXTRA_DEBUGINFO
#define PPMEM_CHECKMARK			(0x1df001ed)	// i'd fooled :D
#define PPMEM_DEBUG_TAG_MAX		32

#ifdef EQ_DEBUG
#define PPMEM_DEBUG_TAGS
#endif // EQ_DEBUG

struct ppallocinfo_t
{
#ifdef PPMEM_DEBUG_TAGS
	// extra visibility via CRT debug output
	char			tag[PPMEM_DEBUG_TAG_MAX];
#endif // PPMEM_DEBUG_TAGS

#ifdef PPMEM_EXTRA_DEBUGINFO
	PPSourceLine	sl;
#endif // PPMEM_EXTRA_DEBUGINFO

	uint			id;
	size_t			size;

	uint			checkMark;
};

DECLARE_CONCOMMAND_FN(ppmemstats)
{
	bool fullStats = false;

	if(CMD_ARGC > 0 && CMD_ARGV(0) == "full")
		fullStats = true;

	PPMemInfo( fullStats );
}

typedef std::unordered_map<void*,ppallocinfo_t*>::iterator allocIterator_t;

// allocation map
static std::unordered_map<void*,ppallocinfo_t*>	s_allocPointerMap;
static uint								s_allocIdCounter = 0;
CEqMutex			g_allocMemMutex;

bool g_enablePPMem = false;

static ConCommand	ppmem_stats("ppmem_stats",CONCOMMAND_FN(ppmemstats), "Memory info",CV_UNREGISTERED);
static ConVar		ppmem_break_on_alloc("ppmem_break_on_alloc", "-1", "Helps to catch allocation id at stack trace",CV_UNREGISTERED);

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

DECLARE_CMD(crtdebug_break_alloc, "Sets allocation ID to catch allocation", CV_UNREGISTERED)
{
	if(CMD_ARGC == 0)
	{
		Msg("now: %d\n", _crtBreakAlloc);
		return;
	}

	// don't print any message to console
	_crtBreakAlloc = atoi(CMD_ARGV(0).ToCString());
}

size_t _crtBreakAllocSize = -1;

int EqAllocHook( int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber)
{
	bool cond = (_crtBreakAlloc == requestNumber && _crtBreakAllocSize == size);

	return cond ? FALSE : TRUE;
}

#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

void PPMemInit()
{
	int idxEnablePpmem = g_cmdLine->FindArgument("-memdebug");

	g_enablePPMem = (idxEnablePpmem != -1);

	if(g_enablePPMem)
	{
		g_consoleCommands->RegisterCommand(&ppmem_stats);
		g_consoleCommands->RegisterCommand(&ppmem_break_on_alloc);
	}

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->RegisterCommand(&cmd_crtdebug_break_alloc);

	_CrtSetAllocHook(EqAllocHook);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

	//PPMemShutdown();
}

void PPMemShutdown()
{
	if(!g_enablePPMem)
		return;

    g_consoleCommands->UnregisterCommand(&ppmem_stats);
    g_consoleCommands->UnregisterCommand(&ppmem_break_on_alloc);

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->UnregisterCommand(&cmd_crtdebug_break_alloc);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
}

// Printing the statistics and tracked memory usage
void PPMemInfo( bool fullStats )
{
	if(!g_enablePPMem)
		return;

	CScopedMutex m(g_allocMemMutex);

	size_t totalUsage = 0;
	size_t numErrors = 0;

	for(allocIterator_t iterator = s_allocPointerMap.begin(); iterator != s_allocPointerMap.end(); iterator++)
	{
		ppallocinfo_t* alloc = iterator->second;
		void* curPtr = iterator->first;

		totalUsage += alloc->size;
	
		if(fullStats)
		{

#ifdef PPMEM_DEBUG_TAGS

#	ifdef PPMEM_EXTRA_DEBUGINFO
			MsgInfo("alloc '%s' id=%d, src='%s:%d', ptr=%p, size=%d\n", alloc->tag, alloc->id, alloc->src, alloc->line, curPtr, alloc->size);
#	else
			MsgInfo("alloc '%s' id=%d, ptr=%p, size=%d\n", alloc->tag, alloc->id, curPtr, alloc->size);
#	endif

#else

#	ifdef PPMEM_EXTRA_DEBUGINFO
			MsgInfo("alloc id=%d, src='%s:%d', ptr=%p, size=%d\n", alloc->id, alloc->sl.GetFileName(), alloc->sl.GetLine(), curPtr, alloc->size);
#	else
			MsgInfo("alloc id=%d, ptr=%p, size=%d\n", alloc->id, curPtr, alloc->size);
#	endif

#endif // PPMEM_DEBUG_TAGS



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

	CScopedMutex m(g_allocMemMutex);

	allocIterator_t it = s_allocPointerMap.find(ptr);

	if(it != s_allocPointerMap.end())
	{
		isValidInputPtr = true;
		return it->second;
	}
	else // try find the valid allocation in range
	{
		isValidInputPtr = false;

		for(allocIterator_t iterator = s_allocPointerMap.begin(); iterator != s_allocPointerMap.end(); iterator++)
		{
			if(iterator->second == NULL)
				continue;

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
void* PPDAlloc(size_t size, const PPSourceLine& sl, const char* debugTAG)
{
#ifdef PPMEM_DISABLE
	void* mem = pp_internal_malloc(size);
	ASSERT_MSG(mem, "No mem left");
	return mem;
#else

	if (!g_enablePPMem)
	{
		void* mem = pp_internal_malloc(size);
		ASSERT_MSG(mem, "No mem left");
		return mem;
	}

	// allocate more to store extra information of this
	ppallocinfo_t* alloc = (ppallocinfo_t*)pp_internal_malloc(sizeof(ppallocinfo_t) + size + sizeof(uint));
	ASSERT_MSG(alloc, "No mem left");

	alloc->sl = sl;
	alloc->size = size;
	alloc->id = s_allocIdCounter++;

#ifdef PPMEM_DEBUG_TAGS
	// extra visibility via CRT debug output
	if(!debugTAG)
		strncpy(alloc->tag, EqString::Format("ppalloc_%d", s_allocIdCounter).ToCString(), PPMEM_DEBUG_TAG_MAX);
	else
		strncpy(alloc->tag, debugTAG, PPMEM_DEBUG_TAG_MAX);
#endif // PPMEM_DEBUG_TAGS

	// actual pointer address
	void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);

	uint* checkMark = (uint*)((ubyte*)actualPtr + size);

	// begin and end mark for checking
	alloc->checkMark = PPMEM_CHECKMARK;
	*checkMark = PPMEM_CHECKMARK;

	g_allocMemMutex.Lock();
	s_allocPointerMap[actualPtr] = alloc;	// store pointer in global map
	g_allocMemMutex.Unlock();

	if( ppmem_break_on_alloc.GetInt() != -1)
		ASSERT_MSG(alloc->id == (uint)ppmem_break_on_alloc.GetInt(), EqString::Format("PPDAlloc: Break on allocation id=%d", alloc->id).ToCString());

	return actualPtr;
#endif // PPMEM_DISABLE
}

// reallocates memory block
void* PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl, const char* debugTAG )
{
#ifdef PPMEM_DISABLE
	return realloc(ptr, size);
#else
	if(!g_enablePPMem)
		return realloc(ptr, size);

	bool isValid = false;
	ppallocinfo_t* alloc = FindAllocation(ptr, isValid);

	if(alloc)
	{
		int ptrDiff = (ubyte*)ptr - ((ubyte*)alloc);
		ASSERT_MSG(isValid, EqString::Format("PPDReAlloc: Given pointer is invalid but allocation was found in the range.\nOffset is %d bytes.", ptrDiff).ToCString());

		void* oldPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);

		g_allocMemMutex.Lock();
		s_allocPointerMap.erase(oldPtr);
		g_allocMemMutex.Unlock();

		alloc = (ppallocinfo_t*)realloc(alloc, sizeof(ppallocinfo_t) + size + sizeof(uint));

		ASSERT_MSG(alloc != nullptr, "PPDReAlloc: NULL pointer after realloc!");

		// set new size
		alloc->size = size;
		alloc->sl = sl;

		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);

		uint* checkMark = (uint*)((ubyte*)actualPtr + size);

		// store pointer in global map
		g_allocMemMutex.Lock();
		s_allocPointerMap[actualPtr] = alloc;
		g_allocMemMutex.Unlock();

		// reset end mark for checking
		*checkMark = PPMEM_CHECKMARK;

		return actualPtr;
	}
	else
		return PPDAlloc(size, sl, debugTAG);
#endif // PPMEM_DISABLE
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLE
	free(ptr);
#else
	if (!g_enablePPMem)
	{
		free(ptr);
		return;
	}
		

	if(ptr == nullptr)
		return;

	bool isValid = false;
	ppallocinfo_t* alloc = FindAllocation(ptr, isValid);

	ASSERT_MSG(alloc != nullptr, "PPFree ERROR: pointer is not valid or it's been already freed!");

	if(alloc)
	{
		int ptrDiff = (ubyte*)ptr - ((ubyte*)alloc);
		ASSERT_MSG(isValid, EqString::Format("PPFree: Given pointer is invalid but allocation was found in the range.\nOffset is %d bytes.", ptrDiff).ToCString());

		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		uint* checkMark = (uint*)((ubyte*)actualPtr + alloc->size);

		ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged before)");
		ASSERT_MSG(*checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged after)");

		free(alloc);

		g_allocMemMutex.Lock();
		s_allocPointerMap.erase(actualPtr);
		g_allocMemMutex.Unlock();
	}
#endif // PPMEM_DISABLE
}

void* operator new(size_t size, PPSourceLine sl)
{
	return PPDAlloc(size, sl);
}

void* operator new[](size_t size, PPSourceLine sl)
{
	return PPDAlloc(size, sl);
}

void operator delete(void* ptr, PPSourceLine sl)
{
	PPFree(ptr);
}

void operator delete[](void* ptr, PPSourceLine sl)
{
	PPFree(ptr);
}