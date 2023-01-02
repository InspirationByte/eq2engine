//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

#include "core/core_common.h"

#include "core/ppmem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IConsoleCommands.h"

#ifdef _RETAIL
#define PPMEM_DISABLED
#endif

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
#define pp_internal_malloc(s)	_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#else
#define pp_internal_malloc(s)	malloc(s)
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

using namespace Threading;

#define PPMEM_EXTRA_DEBUGINFO
#define PPMEM_CHECKMARK			(0x1df001ed)	// i'd fooled :D

struct ppallocinfo_t
{
	ppallocinfo_t*	next{nullptr};
	ppallocinfo_t*	prev{nullptr};

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
struct ppmem_src_counter_t
{
	uint64 count{ 0 };
	uint64 lastTime{ 0 };
};
using source_counter_map = Map<uint64, ppmem_src_counter_t>;
using source_map = Map<const char*, const char*>;

struct ppmem_state_t
{
#ifdef PPMEM_EXTRA_DEBUGINFO
	source_map sourceFileNameMap{PPSourceLine::Empty()};
	source_counter_map sourceCounterMap{ PPSourceLine::Empty() };
	CEqTimer timer;
#endif

	ppallocinfo_t* first{ nullptr };
	ppallocinfo_t* last{ nullptr };
	size_t numAllocs{ 0 };

	uint allocIdCounter = 0;
	uint64 allocMemCounter = 0;
	CEqMutex allocMemMutex;
};

static ppmem_state_t& PPGetState()
{
	static ppmem_state_t st;
	return st;
}

#ifndef PPMEM_DISABLED
static ConCommand	ppmem_stats("ppmem_stats",CONCOMMAND_FN(ppmemstats), "Memory info",CV_UNREGISTERED);
static ConVar		ppmem_break_on_alloc("ppmem_break_on_alloc", "-1", "Helps to catch allocation id at stack trace",CV_UNREGISTERED);
#endif

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
#ifndef PPMEM_DISABLED
	g_consoleCommands->RegisterCommand(&ppmem_stats);
	g_consoleCommands->RegisterCommand(&ppmem_break_on_alloc);
#endif

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->RegisterCommand(&cmd_crtdebug_break_alloc);

	_CrtSetAllocHook(EqAllocHook);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

	//PPMemShutdown();
}

void PPMemShutdown()
{
#ifndef PPMEM_DISABLED
    g_consoleCommands->UnregisterCommand(&ppmem_stats);
    g_consoleCommands->UnregisterCommand(&ppmem_break_on_alloc);
#endif
#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	g_consoleCommands->UnregisterCommand(&cmd_crtdebug_break_alloc);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
}

// Printing the statistics and tracked memory usage
void PPMemInfo(bool fullStats)
{
	ppmem_state_t& st = PPGetState();

	CScopedMutex m(st.allocMemMutex);

	size_t totalUsage = 0;
	size_t numErrors = 0;

	struct SLStat_t
	{
		size_t totalMem{ 0 };
		uint numAlloc{ 0 };
	};

	Map<uint64, SLStat_t> allocCounter{ PPSourceLine::Empty() };

	if (fullStats)
		MsgInfo("--- currently allocated memory ---\n");

	// currently allocated items
	for(ppallocinfo_t* alloc = st.first; alloc != nullptr; alloc = alloc->next)
	{
		const void* curPtr = alloc + 1;

		totalUsage += alloc->size;
	
		if(fullStats)
		{
#ifdef PPMEM_EXTRA_DEBUGINFO
			const char* filename = st.sourceFileNameMap[alloc->sl.GetFileName()];
			const int fileLine = alloc->sl.GetLine();
			MsgInfo("alloc id=%d, src='%s:%d', ptr=%p, size=%d\n", alloc->id, filename, fileLine, curPtr, alloc->size);
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

#ifdef PPMEM_EXTRA_DEBUGINFO
		SLStat_t& slStat = allocCounter[alloc->sl.data];
		slStat.totalMem += alloc->size;
		slStat.numAlloc++;
#endif // PPMEM_EXTRA_DEBUGINFO
	}

#ifdef PPMEM_EXTRA_DEBUGINFO
	MsgInfo("--- allocations groupped by file-line ---\n");

	// currently allocated items groupped by file:line
	{
		Array<uint64> sortedList{ PPSourceLine::Empty() };
		sortedList.resize(allocCounter.size());
		for (auto it = allocCounter.begin(); it != allocCounter.end(); ++it)
			sortedList.append(it.key());

		sortedList.sort([&allocCounter](uint64 a, uint64 b) {
			return (int64)allocCounter[b].numAlloc - (int64)allocCounter[a].numAlloc;
		});

		for (int i = 0; i < sortedList.numElem(); ++i)
		{
			const uint64 key = sortedList[i];
			const SLStat_t& stat = allocCounter[key];
			const PPSourceLine sl = *(PPSourceLine*)&key;

			const char* filename = st.sourceFileNameMap[sl.GetFileName()];
			const int fileLine = sl.GetLine();

			MsgInfo("'%s:%d' count: %d, size: %.2f KB\n", st.sourceFileNameMap[sl.GetFileName()], sl.GetLine(), stat.numAlloc, (stat.totalMem / 1024.0f));
		}
	}

	MsgInfo("--- allocation rate statistics ---\n");

	// (re)allocation rate stats
	{
		Array<uint64> sortedList{ PPSourceLine::Empty() };
		sortedList.resize(st.sourceCounterMap.size());
		for (auto it = st.sourceCounterMap.begin(); it != st.sourceCounterMap.end(); ++it)
			sortedList.append(it.key());

		sortedList.sort([&st](uint64 a, uint64 b) {
			return (int64)st.sourceCounterMap[b].lastTime - (int64)st.sourceCounterMap[a].lastTime;
		});

		for (int i = 0; i < sortedList.numElem(); ++i)
		{
			const uint64 key = sortedList[i];
			const PPSourceLine sl = *(PPSourceLine*)&key;

			const char* filename = st.sourceFileNameMap[sl.GetFileName()];
			const int fileLine = sl.GetLine();

			MsgInfo("'%s:%d' counter: %u\n", st.sourceFileNameMap[sl.GetFileName()], sl.GetLine(), st.sourceCounterMap[key].count);
		}
	}
#endif // PPMEM_EXTRA_DEBUGINFO

	MsgInfo("Total %u allocactions, mem usage: %.2f MB\n", st.numAllocs, (totalUsage / 1024.0f) / 1024.0f);

	if(numErrors > 0)
		MsgWarning("%d allocations has overflow/underflow happened in runtime. Please print full stats to console\n", numErrors);
}

IEXPORTS size_t	PPMemGetUsage()
{
#ifdef PPMEM_DISABLE
	return 0;
#else
	ppmem_state_t& st = PPGetState();
	return st.allocMemCounter;
#endif
}

// allocated debuggable memory block
void* PPDAlloc(size_t size, const PPSourceLine& sl)
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
	}

	// insert to linked list tail
	{
		CScopedMutex m(st.allocMemMutex);
		++st.numAllocs;
		st.allocMemCounter += alloc->size;

		if (st.last != nullptr)
			st.last->next = alloc;
		else
			st.first = alloc;

		alloc->prev = st.last;
		alloc->next = nullptr;
		st.last = alloc;

#ifdef PPMEM_EXTRA_DEBUGINFO
		if (!st.sourceFileNameMap.count(sl.GetFileName()))
			st.sourceFileNameMap[sl.GetFileName()] = strdup(sl.GetFileName());

		ppmem_src_counter_t& cnt = st.sourceCounterMap[sl.data];
		++cnt.count;
		cnt.lastTime = st.timer.GetTimeMS();
#endif
	}

	if( ppmem_break_on_alloc.GetInt() != -1)
		ASSERT_MSG(alloc->id == (uint)ppmem_break_on_alloc.GetInt(), "PPDAlloc: Break on allocation id=%d", alloc->id);

	return actualPtr;
#endif // PPMEM_DISABLE
}

// reallocates memory block
void* PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl )
{
#ifdef PPMEM_DISABLE
	void* mem = realloc(ptr, size);
	ASSERT_MSG(mem, "No mem left");
	return mem;
#else
	void* retPtr = nullptr;

	{
		ppmem_state_t& st = PPGetState();

		const ppallocinfo_t* r_alloc = (ppallocinfo_t*)ptr - 1;
		if (ptr == nullptr || r_alloc->checkMark != PPMEM_CHECKMARK)
		{
			return PPDAlloc(size, sl);
		}

		// remove from linked list first
		{
			CScopedMutex m(st.allocMemMutex);
			st.allocMemCounter -= r_alloc->size;

			if (r_alloc->prev == nullptr)
				st.first = r_alloc->next;
			else
				r_alloc->prev->next = r_alloc->next;

			if (r_alloc->next == nullptr)
				st.last = r_alloc->prev;
			else
				r_alloc->next->prev = r_alloc->prev;
		}

		ppallocinfo_t* alloc = (ppallocinfo_t*)realloc((void*)r_alloc, sizeof(ppallocinfo_t) + size + sizeof(uint));
		ASSERT_MSG(alloc, "realloc: no mem left!");

		// actual pointer address
		retPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		uint* checkMark = (uint*)((ubyte*)retPtr + size);

		// insert to linked list tail
		{
			CScopedMutex m(st.allocMemMutex);
			st.allocMemCounter += alloc->size;

			if (st.last != nullptr)
				st.last->next = alloc;
			else
				st.first = alloc;

			alloc->prev = st.last;
			alloc->next = nullptr;
			st.last = alloc;

#ifdef PPMEM_EXTRA_DEBUGINFO
			ppmem_src_counter_t& cnt = st.sourceCounterMap[sl.data];
			++cnt.count;
			cnt.lastTime = st.timer.GetTimeMS();
#endif
		}

		{
			alloc->sl = sl;
			alloc->size = size;
			alloc->id = st.allocIdCounter++;

			alloc->checkMark = PPMEM_CHECKMARK;
			*checkMark = PPMEM_CHECKMARK;
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

		const ppallocinfo_t* alloc = (ppallocinfo_t*)ptr - 1;
		if(alloc->checkMark != PPMEM_CHECKMARK)
		{
			free(ptr);
			return;
		}

		// remove from linked list
		{
			CScopedMutex m(st.allocMemMutex);
			--st.numAllocs;
			st.allocMemCounter -= alloc->size;

			if (alloc->prev == nullptr)
				st.first = alloc->next;
			else
				alloc->prev->next = alloc->next;

			if (alloc->next == nullptr)
				st.last = alloc->prev;
			else
				alloc->next->prev = alloc->prev;
		}

		// actual pointer address
		const void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		const uint* checkMark = (uint*)((ubyte*)actualPtr + alloc->size);

		ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged before)");
		ASSERT_MSG(*checkMark == PPMEM_CHECKMARK, "PPCheck: memory is invalid (was outranged after)");

		free((void*)alloc);
	}
#endif // PPMEM_DISABLE
}
