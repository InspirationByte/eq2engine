//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
#include "core/IFileSystem.h"

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
#define pp_internal_malloc(s)	_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#else
#define pp_internal_malloc(s)	malloc(s)
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

using namespace Threading;

constexpr const uint PPMEM_CHECKMARK	   = MAKECHAR4('P','P','M','E');
constexpr const uint PPMEM_CHECKMARK_FREED = MAKECHAR4('E','M','T','Y');
constexpr const uint PPMEM_EXTRA_MARKS = 20;

struct ppallocinfo_t
{
	ppallocinfo_t*	next{nullptr};
	ppallocinfo_t*	prev{nullptr};

	size_t			size;
	PPSourceLine	sl;

	uint			id;
	uint			checkMark;
};

// allocation map
struct ppsrc_counter_t
{
	uint64 count{ 0 };
	uint64 lastTimeStamp{ 0 };
};
using source_counter_map = Map<uint64, ppsrc_counter_t>;
using source_map = Map<const char*, const char*>;

struct ppmem_state_t
{
	source_map sourceFileNameMap{PPSourceLine::Empty()};
	source_counter_map sourceCounterMap{ PPSourceLine::Empty() };
	CEqTimer timer;

	ppallocinfo_t* first{ nullptr };
	ppallocinfo_t* last{ nullptr };
	uint64 numAllocs{ 0 };

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

constexpr EqStringRef s_ppmemFullStatsCmd = "full";

DECLARE_CMD(ppmem_stats, "Memory info", CV_UNREGISTERED)
{
	bool fullStats = false;

	if (CMD_ARGC > 0 && CMD_ARGV(0) == s_ppmemFullStatsCmd)
		fullStats = true;

	PPMemInfo(fullStats);
}
DECLARE_CVAR(ppmem_break_on_alloc, "-1", "Helps to catch allocation id at stack trace", CV_UNREGISTERED);
DECLARE_CVAR(ppmem_stats_rate, "0", "Shows allocation rate statistics for each source line", CV_UNREGISTERED);
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
	ConCommandBase::Register(&ppmem_stats);
	ConCommandBase::Register(&ppmem_break_on_alloc);
	ConCommandBase::Register(&ppmem_stats_rate);
#endif

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	ConCommandBase::Register(&cmd_crtdebug_break_alloc);

	_CrtSetAllocHook(EqAllocHook);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

	//PPMemShutdown();
}

void PPMemShutdown()
{
#ifndef PPMEM_DISABLED
    ConCommandBase::Unregister(&ppmem_stats);
    ConCommandBase::Unregister(&ppmem_break_on_alloc);
	ConCommandBase::Unregister(&ppmem_stats_rate);
#endif
#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	ConCommandBase::Unregister(&cmd_crtdebug_break_alloc);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
}

struct SLStat
{
	uint64 totalMem{ 0 };
	uint64 numAllocated{ 0 };
	uint64 allocCounter{ 0 };
	uint64 lastTimeStamp{ 0 };
	uint64 checkFailed{ 0 };
};

using PPMemSLStat = Map<uint64, SLStat>;

static void PPMemPlotAllocStatsCSV()
{
	FILE* statFile = fopen("logs/ppmemstat.csv", "w");
	if (!statFile)
	{
		MsgError("Failed to write ppmemstat.csv");
		return;
	}

	PPMemSLStat allocCounter{ PPSourceLine::Empty() };
	ppmem_state_t& st = PPGetState();
	CScopedMutex m(st.allocMemMutex);
	for (ppallocinfo_t* alloc = st.first; alloc != nullptr; alloc = alloc->next)
	{
		SLStat& slStat = allocCounter[alloc->sl.data];
		ppsrc_counter_t& srcCounter = st.sourceCounterMap[alloc->sl.data];

		slStat.allocCounter = srcCounter.count;
		slStat.lastTimeStamp = srcCounter.lastTimeStamp;
		slStat.totalMem += alloc->size;
		slStat.numAllocated++;

		const void* curPtr = alloc + 1;
		uint* checkMark = (uint*)((ubyte*)curPtr + alloc->size);
		if (alloc->checkMark != PPMEM_CHECKMARK || *checkMark != PPMEM_CHECKMARK)
			++slStat.checkFailed;
	}

	fprintf(statFile, "Source,Allocated,Alloc_Counter,Alloc_Last_Timestamp,Alloc_Check_Fail,Total_Mem\n");
	for (auto it = allocCounter.begin(); !it.atEnd(); ++it)
	{
		const SLStat& stat = *it;
		const PPSourceLine sl = *(PPSourceLine*)&it.key();

		const char* filename = st.sourceFileNameMap[sl.GetFileName()];
		const int fileLine = sl.GetLine();

		fprintf(statFile, "\"%s:%d\",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n", filename, fileLine, stat.numAllocated, stat.allocCounter, stat.lastTimeStamp, stat.checkFailed, stat.totalMem);
	}

	fclose(statFile);
}

// Printing the statistics and tracked memory usage
void PPMemInfo(bool saveStatFile)
{
#if !defined(PPMEM_DISABLED)
	if (saveStatFile)
	{
		PPMemPlotAllocStatsCSV();
		return;
	}

	ppmem_state_t& st = PPGetState();
	size_t totalUsage = 0;

	// currently allocated items
	CScopedMutex m(st.allocMemMutex);
	for(ppallocinfo_t* alloc = st.first; alloc != nullptr; alloc = alloc->next)
	{
		const void* curPtr = alloc + 1;
		totalUsage += alloc->size;
	}

	MsgInfo("Total %" PRIu64 " allocactions, mem usage: %.2f MB\n", st.numAllocs, (totalUsage / 1024.0f) / 1024.0f);
#endif // !PPMEM_DISABLED
}

IEXPORTS size_t	PPMemGetUsage()
{
#ifdef PPMEM_DISABLED
	return 0;
#else
	ppmem_state_t& st = PPGetState();
	return st.allocMemCounter;
#endif
}

// allocated debuggable memory block
void* PPDAlloc(size_t size, const PPSourceLine& sl)
{
#ifdef PPMEM_DISABLED
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

	// alignof(ppallocinfo_t);
	// sizeof(ppallocinfo_t);
	// sizeof(RawItem<ppallocinfo_t, alignof(ppallocinfo_t)>);

	ppmem_state_t& st = PPGetState();
	// allocate more to store extra information of this
	ppallocinfo_t* alloc = (ppallocinfo_t*)pp_internal_malloc(sizeof(ppallocinfo_t) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	ASSERT_MSG(alloc, "alloc: no mem left");

	// actual pointer address
	void* actualPtr = alloc + 1;
	{
		alloc->sl = sl;
		alloc->size = size;
		alloc->id = st.allocIdCounter++;

		alloc->checkMark = PPMEM_CHECKMARK;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + size);
		for (int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK;
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

		if (!st.sourceFileNameMap.count(sl.GetFileName()))
			st.sourceFileNameMap[sl.GetFileName()] = strdup(sl.GetFileName());

		ppsrc_counter_t& cnt = st.sourceCounterMap[sl.data];
		++cnt.count;
		cnt.lastTimeStamp = st.timer.GetTimeMS();
	}

	if( ppmem_break_on_alloc.GetInt() != -1)
		ASSERT_MSG(alloc->id == (uint)ppmem_break_on_alloc.GetInt(), "PPDAlloc: Break on allocation id=%d", alloc->id);

	return actualPtr;
#endif // PPMEM_DISABLED
}

static int PPMemCmpTailCheckmarks(const ubyte* x)
{
	const uint checkmark = PPMEM_CHECKMARK;
	const ubyte* y = (ubyte*)&checkmark;

	int diff = 0;
	for (size_t i = 0; i < PPMEM_EXTRA_MARKS * sizeof(uint); i++) 
		diff += (x[i] != y[i & 3]);

	return diff;
}


// reallocates memory block
void* PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl )
{
#ifdef PPMEM_DISABLED
	void* mem = realloc(ptr, size);
	ASSERT_MSG(mem, "No mem left");
	return mem;
#else
	ppmem_state_t& st = PPGetState();

	ppallocinfo_t* r_alloc = (ppallocinfo_t*)ptr - 1;
	if (ptr == nullptr || r_alloc->checkMark != PPMEM_CHECKMARK)
	{
		return PPDAlloc(size, sl);
	}

	{
		// actual pointer address
		void* actualPtr = r_alloc + 1;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + r_alloc->size);

		const int diff = PPMemCmpTailCheckmarks((ubyte*)tailCheckMark);
		ASSERT_MSG(r_alloc->checkMark == PPMEM_CHECKMARK, "buffer underflow of %s:%d, investigate with ASAN", r_alloc->sl.GetFileName(), r_alloc->sl.GetLine());
		ASSERT_MSG(diff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", diff, r_alloc->sl.GetFileName(), r_alloc->sl.GetLine());
	}

	// remove from linked list first
	// as realloc might change the pointer
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

	ppallocinfo_t* alloc = (ppallocinfo_t*)realloc((void*)r_alloc, sizeof(ppallocinfo_t) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	ASSERT_MSG(alloc, "realloc: no mem left!");

	// actual pointer address
	void* actualPtr = alloc + 1;
	{
		alloc->size = size;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + size);
		for(int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK;
	}

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

		ppsrc_counter_t& cnt = st.sourceCounterMap[sl.data];
		++cnt.count;
		cnt.lastTimeStamp = st.timer.GetTimeMS();
	}

	return actualPtr;
#endif // PPMEM_DISABLED
}

IEXPORTS void PPDCheck(void* ptr)
{
#ifndef PPMEM_DISABLED
	ppmem_state_t& st = PPGetState();

	ppallocinfo_t* alloc = (ppallocinfo_t*)ptr - 1;
	if (ptr == nullptr || alloc->checkMark != PPMEM_CHECKMARK)
		return;

	// actual pointer address
	void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
	uint* tailCheckMark = (uint*)((ubyte*)actualPtr + alloc->size);

	const int diff = PPMemCmpTailCheckmarks((ubyte*)tailCheckMark);
	ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "buffer underflow of %s:%d, investigate with ASAN", alloc->sl.GetFileName(), alloc->sl.GetLine());
	ASSERT_MSG(diff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", diff, alloc->sl.GetFileName(), alloc->sl.GetLine());

	// try restoring memory region so app will try to crash if somehow it uses it
	//alloc->checkMark = PPMEM_CHECKMARK;
	//for (int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
	//	*tailCheckMark++ = PPMEM_CHECKMARK;
#endif
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLED
	free(ptr);
#else

	if(ptr == nullptr)
		return;

	ppmem_state_t& st = PPGetState();

	ppallocinfo_t* alloc = (ppallocinfo_t*)ptr - 1;
	if(alloc->checkMark != PPMEM_CHECKMARK)
	{
		free(ptr);
		return;
	}

	{
		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(ppallocinfo_t);
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + alloc->size);

		const int diff = PPMemCmpTailCheckmarks((ubyte*)tailCheckMark);
		ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "buffer underflow of %s:%d, investigate with ASAN", alloc->sl.GetFileName(), alloc->sl.GetLine());
		ASSERT_MSG(diff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", diff, alloc->sl.GetFileName(), alloc->sl.GetLine());

		// set check marks to indicate freed mem regions
		alloc->checkMark = PPMEM_CHECKMARK_FREED;
		for (int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK_FREED;
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

	free((void*)alloc);
#endif // PPMEM_DISABLED
}
