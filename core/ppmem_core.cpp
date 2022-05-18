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

#define NO_PPMEM_OP
#include "core/ppmem.h"

#include "ds/Map.h"

#include "core/DebugInterface.h"
#include "core/IConsoleCommands.h"

#include "utils/eqthread.h"

#include <malloc.h>

//#define PPMEM_DISABLE

#if defined(_RETAIL) || defined(PLAT_ANDROID)
#define PPMEM_DISABLE
#endif

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
#define pp_internal_malloc(s)	_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#else
#define pp_internal_malloc(s)	malloc(s)
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

using namespace Threading;

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

	size_t			size;

#ifdef PPMEM_EXTRA_DEBUGINFO
	PPSourceLine	sl;
#endif // PPMEM_EXTRA_DEBUGINFO

	uint			id;
	uint			checkMark;
};

DECLARE_CONCOMMAND_FN(ppmemstats)
{
	bool fullStats = false;

	if(CMD_ARGC > 0 && CMD_ARGV(0) == "full")
		fullStats = true;

	PPMemInfo( fullStats );
}

// allocation map
using pointer_map = Map<const void*, ppallocinfo_t*>;
using source_map = Map<const char*, const char*>;

struct ppmem_state_t
{
	source_map sourceFileNameMap{PPSourceLine::Empty()};
	pointer_map allocPointerMap{ PPSourceLine::Empty() };
	uint allocIdCounter = 0;
	CEqMutex allocMemMutex;
};

ppmem_state_t& PPGetState()
{
	static ppmem_state_t st;
	return st;
}

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
	g_consoleCommands->RegisterCommand(&ppmem_stats);
	g_consoleCommands->RegisterCommand(&ppmem_break_on_alloc);

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->RegisterCommand(&cmd_crtdebug_break_alloc);

	_CrtSetAllocHook(EqAllocHook);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

	//PPMemShutdown();
}

void PPMemShutdown()
{
    g_consoleCommands->UnregisterCommand(&ppmem_stats);
    g_consoleCommands->UnregisterCommand(&ppmem_break_on_alloc);

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->UnregisterCommand(&cmd_crtdebug_break_alloc);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
}

// Printing the statistics and tracked memory usage
void PPMemInfo( bool fullStats )
{
	ppmem_state_t& st = PPGetState();

	CScopedMutex m(st.allocMemMutex);

	size_t totalUsage = 0;
	size_t numErrors = 0;

	Map<uint64, int> allocCounter{ PPSourceLine::Empty() };

	for(auto it = st.allocPointerMap.begin(); it != st.allocPointerMap.end(); ++it)
	{
		ppallocinfo_t* alloc = it.value();
		const void* curPtr = it.key();

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
			MsgInfo("alloc id=%d, src='%s:%d', ptr=%p, size=%d\n", alloc->id, st.sourceFileNameMap[alloc->sl.GetFileName()], alloc->sl.GetLine(), curPtr, alloc->size);
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

#ifdef PPMEM_EXTRA_DEBUGINFO
		if (!allocCounter.count(alloc->sl.data))
			allocCounter[alloc->sl.data] = 0;
		allocCounter[alloc->sl.data]++;
#endif // PPMEM_EXTRA_DEBUGINFO
	}

#ifdef PPMEM_EXTRA_DEBUGINFO
	for (auto it = allocCounter.begin(); it != allocCounter.end(); ++it)
	{
		const PPSourceLine sl = *(PPSourceLine*)&it.key();
		MsgInfo("'%s:%d' count: %d\n", st.sourceFileNameMap[sl.GetFileName()], sl.GetLine(), it.value());
	}
#endif // PPMEM_EXTRA_DEBUGINFO

	uint allocCount = st.allocPointerMap.size();

	MsgInfo("--- of %u allocactions, total usage: %.2f MB\n", allocCount, (totalUsage / 1024.0f) / 1024.0f);

	if(numErrors > 0)
		MsgWarning("%d allocations has overflow/underflow happened in runtime. Please print full stats to console\n", numErrors);
}

IEXPORTS size_t	PPMemGetUsage()
{
#ifdef PPMEM_DISABLE
	return 0;
#else
	ppmem_state_t& st = PPGetState();
	CScopedMutex m(st.allocMemMutex);

	size_t totalUsage = 0;

	Map<uint64, int> allocCounter{ PPSourceLine::Empty() };

	for (auto it = st.allocPointerMap.begin(); it != st.allocPointerMap.end(); ++it)
	{
		const ppallocinfo_t* alloc = it.value();
		totalUsage += alloc->size;
	}
	return totalUsage;
#endif
}

// allocated debuggable memory block
void* PPDAlloc(size_t size, const PPSourceLine& sl, const char* debugTAG)
{
#ifdef PPMEM_DISABLE
	void* mem = pp_internal_malloc(size);
	ASSERT_MSG(mem, "No mem left");
	return mem;
#else

	if (sl.data == 0) 
	{
		void* mem = pp_internal_malloc(size);
		ASSERT_MSG(mem, "No mem left");
		return mem;
	}

	ppmem_state_t& st = PPGetState();
	// allocate more to store extra information of this
	ppallocinfo_t* alloc = (ppallocinfo_t*)pp_internal_malloc(sizeof(ppallocinfo_t) + size + sizeof(uint));
	ASSERT_MSG(alloc, "alloc: no mem left");

	// actual pointer address
	void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
	uint* checkMark = (uint*)((ubyte*)actualPtr + size);

	{
		alloc->sl = sl;
		alloc->size = size;
		alloc->id = st.allocIdCounter++;

		alloc->checkMark = PPMEM_CHECKMARK;
		*checkMark = PPMEM_CHECKMARK;

#ifdef PPMEM_DEBUG_TAGS
		// extra visibility via CRT debug output
		if (!debugTAG)
			strncpy(alloc->tag, EqString::Format("ppalloc_%d", s_allocIdCounter).ToCString(), PPMEM_DEBUG_TAG_MAX);
		else
			strncpy(alloc->tag, debugTAG, PPMEM_DEBUG_TAG_MAX);
#endif // PPMEM_DEBUG_TAGS
	}

	if(!st.sourceFileNameMap.count(sl.GetFileName()))
		st.sourceFileNameMap[sl.GetFileName()] = strdup(sl.GetFileName());

	st.allocMemMutex.Lock();
	st.allocPointerMap[actualPtr] = alloc;	// store pointer in global map
	st.allocMemMutex.Unlock();

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
	void* retPtr = nullptr;

	{
		ppmem_state_t& st = PPGetState();
		CScopedMutex m(st.allocMemMutex);

		const auto it = st.allocPointerMap.find(ptr);

		if (it == st.allocPointerMap.end())
		{
			return PPDAlloc(size, sl, debugTAG);
		}

		ppallocinfo_t* alloc = (ppallocinfo_t*)realloc(it.value(), sizeof(ppallocinfo_t) + size + sizeof(uint));
		ASSERT_MSG(alloc, "realloc: no mem left!");

		// actual pointer address
		retPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		uint* checkMark = (uint*)((ubyte*)retPtr + size);

		st.allocPointerMap[retPtr] = alloc;
		if(retPtr != ptr)
			st.allocPointerMap.remove(it);

		{
			alloc->sl = sl;
			alloc->size = size;
			alloc->id = st.allocIdCounter++;

			alloc->checkMark = PPMEM_CHECKMARK;
			*checkMark = PPMEM_CHECKMARK;

#ifdef PPMEM_DEBUG_TAGS
			// extra visibility via CRT debug output
			if (!debugTAG)
				strncpy(alloc->tag, EqString::Format("ppalloc_%d", s_allocIdCounter).ToCString(), PPMEM_DEBUG_TAG_MAX);
			else
				strncpy(alloc->tag, debugTAG, PPMEM_DEBUG_TAG_MAX);
#endif // PPMEM_DEBUG_TAGS
		}
	}


	return retPtr;
#endif // PPMEM_DISABLE
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLE
	free(ptr);
#else

	if(ptr == nullptr)
		return;

	{
		ppmem_state_t& st = PPGetState();
		CScopedMutex m(st.allocMemMutex);

		const auto it = st.allocPointerMap.find(ptr);

		if (it == st.allocPointerMap.end())
		{
			free(ptr);
			return;
		}

		const ppallocinfo_t* alloc = it.value();

		// actual pointer address
		const void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		const uint* checkMark = (uint*)((ubyte*)actualPtr + alloc->size);

		ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged before)");
		ASSERT_MSG(*checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged after)");

		free((void*)alloc);
		st.allocPointerMap.remove(it);
	}
#endif // PPMEM_DISABLE
}
