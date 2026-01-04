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

#ifndef PPMEM_DISABLED
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#endif

constexpr const uint PPMEM_CHECKMARK	   = MAKECHAR4('P','P','M','E');
constexpr const uint PPMEM_CHECKMARK_FREED = MAKECHAR4('F','R','E','E');
constexpr const uint PPMEM_EXTRA_MARKS = 20;

constexpr const uint PPMEM_MAX_STAT_THREADS = 128;

struct PPMemState;

struct PPAllocInfo
{
	PPMemState*		state{nullptr};

	int64			size;
	PPSourceLine	sl;

	uint			id;
	uint			checkMark;
};

// allocation map
struct PPSrcCounter
{
	uint64 allocatedCount{ 0 };
	uint64 allocatedMem{ 0 };
	uint64 counter{ 0 };
	uint64 lastTimeStamp{ 0 };
};
using PPSourceCounterMap = Map<uint64, PPSrcCounter>;
using PPSourceMap = Map<const char*, const char*>;

using PPMemStateList = FixedArray<PPMemState*, PPMEM_MAX_STAT_THREADS>;
static PPMemStateList& PPGetStateList()
{
	static FixedArray<PPMemState*, PPMEM_MAX_STAT_THREADS> s_memStates;
	return s_memStates;
}

static Threading::CEqMutex& PPGetStateMutex()
{
	static Threading::CEqMutex s_memStateMutex;
	return s_memStateMutex;
}

struct PPMemState
{
	PPMemState();

	void	OnAlloc(int64 size, PPSourceLine sl);
	void	OnFree(int64 size, PPSourceLine sl);

	PPSourceMap				sourceFileNameMap{ PPSourceLine::Empty() };
	PPSourceCounterMap		sourceCounterMap{ PPSourceLine::Empty() };
	CEqTimer				timer;

	uint64					numAllocs{ 0 };
	uint64					allocMemCounter = 0;
	uint					allocIdCounter = 0;
};

PPMemState::PPMemState()
{
	Threading::CScopedMutex m(PPGetStateMutex());
	PPGetStateList().append(this);
}

void PPMemState::OnAlloc(int64 size, PPSourceLine sl)
{
	++numAllocs;
	allocMemCounter += size;

	if (!sourceFileNameMap.find(sl.GetFileName()))
		sourceFileNameMap.insert(sl.GetFileName(), strdup(sl.GetFileName()));

	PPSrcCounter& cnt = sourceCounterMap[sl.data];
	++cnt.counter;
	++cnt.allocatedCount;
	cnt.allocatedMem += size;
	cnt.lastTimeStamp = timer.GetTimeMS();
}

void PPMemState::OnFree(int64 size, PPSourceLine sl)
{
	++numAllocs;
	allocMemCounter -= size;

	PPSrcCounter& cnt = sourceCounterMap[sl.data];
	--cnt.allocatedCount;
	cnt.allocatedMem -= size;
}

static PPMemState& PPGetState()
{
	static thread_local PPMemState* st = new PPMemState();
	return *st;
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
DECLARE_CVAR(ppmem_breakOnAlloc, "-1", "Helps to catch allocation id at stack trace", CV_UNREGISTERED);
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
	ConCommandBase::Register(&ppmem_breakOnAlloc);
#endif

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	ConCommandBase::Register(&cmd_crtdebug_break_alloc);

	_CrtSetAllocHook(EqAllocHook);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
}

void PPMemShutdown()
{
#ifndef PPMEM_DISABLED
    ConCommandBase::Unregister(&ppmem_stats);
    ConCommandBase::Unregister(&ppmem_breakOnAlloc);
#endif
#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
	ConCommandBase::Unregister(&cmd_crtdebug_break_alloc);
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)


	Threading::CScopedMutex m(PPGetStateMutex());
	for (PPMemState* st : PPGetStateList())
	{
		for (auto it = st->sourceFileNameMap.begin(); it; ++it)
			free((void*)*it);
		delete st;
	}
	PPGetStateList().clear();
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

	fprintf(statFile, "Source,Allocated,Alloc_Counter,Alloc_Last_Timestamp,Alloc_Check_Fail,Total_Mem\n");

	Threading::CScopedMutex m(PPGetStateMutex());

	PPMemSLStat allocCounter{ PPSourceLine::Empty() };
	PPSourceMap allocSourceNames{ PPSourceLine::Empty() };
	for (PPMemState* st : PPGetStateList())
	{
		for (auto it = st->sourceCounterMap.begin(); it; ++it)
		{
			SLStat& slStat = allocCounter[it.key()];
			const PPSrcCounter& srcCounter = *it;

			slStat.allocCounter = srcCounter.counter;
			slStat.lastTimeStamp = srcCounter.lastTimeStamp;
			slStat.totalMem += srcCounter.allocatedMem;
			slStat.numAllocated++;

			const PPSourceLine sl = *(PPSourceLine*)&it.key();
			const char* filename = st->sourceFileNameMap[sl.GetFileName()];
			allocSourceNames.insert(sl.GetFileName(), filename);
		}
	}

	for (auto it = allocCounter.begin(); !it.atEnd(); ++it)
	{
		const SLStat& stat = *it;
		const PPSourceLine sl = *(PPSourceLine*)&it.key();

		const char* filename = allocSourceNames[sl.GetFileName()];
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

	size_t totalUsage = 0;
	uint64 numAllocs = 0;

	Threading::CScopedMutex m(PPGetStateMutex());

	// currently allocated items
	for (PPMemState* st : PPGetStateList())
	{
		MsgInfo("State %" PRIu64 " allocactions, mem usage: %llu bytes (%.2f MB)\n", st->numAllocs, st->allocMemCounter, (st->allocMemCounter / 1024.0f) / 1024.0f);
		totalUsage += st->allocMemCounter;
		numAllocs += st->numAllocs;
	}

	MsgInfo("Total %" PRIu64 " allocactions, mem usage: %llu bytes (%.2f MB)\n", numAllocs, totalUsage, (totalUsage / 1024.0f) / 1024.0f);
#endif // !PPMEM_DISABLED
}

IEXPORTS size_t	PPMemGetUsage()
{
#ifdef PPMEM_DISABLED
	return 0;
#else
	uint64 memCounterTotal = 0;
	for (PPMemState* st : PPGetStateList())
		memCounterTotal += st->allocMemCounter;

	return memCounterTotal;
#endif
}

#if defined(CRT_DEBUG_ENABLED) && defined(_WIN32)
#define PPInternalMalloc(s)	_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#else
#define PPInternalMalloc(s)	malloc(s)
#endif // defined(CRT_DEBUG_ENABLED) && defined(_WIN32)

// allocated debuggable memory block
void* PPDAlloc(size_t size, const PPSourceLine& sl)
{
#ifdef PPMEM_DISABLED
	void* mem = PPInternalMalloc(size);
	ASSERT_MSG(mem, "No mem left");
	return mem;
#else

	if (sl.data == 0) 
	{
		void* mem = PPInternalMalloc(size);
		ASSERT_MSG(mem, "alloc: no mem left");
		return mem;
	}

	// allocate more to store extra information of this
	PPAllocInfo* alloc = (PPAllocInfo*)PPInternalMalloc(sizeof(PPAllocInfo) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	ASSERT_MSG(alloc, "alloc: no mem left");

	PPMemState& st = PPGetState();

	// actual pointer address
	void* actualPtr = alloc + 1;
	{
		alloc->sl = sl;
		alloc->size = size;
		alloc->id = Atomic::Increment(st.allocIdCounter);
		alloc->state = &st;

		alloc->checkMark = PPMEM_CHECKMARK;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + size);
		for (int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK;
	}

	st.OnAlloc(alloc->size, sl);

	if( ppmem_breakOnAlloc.GetInt() != -1 && alloc->id == (uint)ppmem_breakOnAlloc.GetInt())
		ASSERT_FAIL("PPDAlloc: Break on allocation id=%d", alloc->id);

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
	ASSERT_MSG(mem, "alloc: no mem left");
	return mem;
#else
	PPAllocInfo* r_alloc = (PPAllocInfo*)ptr - 1;
	if (ptr == nullptr || r_alloc->checkMark != PPMEM_CHECKMARK)
	{
		return PPDAlloc(size, sl);
	}

	PPMemState& r_st = *r_alloc->state;
	{
		// actual pointer address
		void* actualPtr = r_alloc + 1;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + r_alloc->size);

		const int diff = PPMemCmpTailCheckmarks((ubyte*)tailCheckMark);
		ASSERT_MSG(r_alloc->checkMark == PPMEM_CHECKMARK, "buffer underflow of %s:%d, investigate with ASAN", r_alloc->sl.GetFileName(), r_alloc->sl.GetLine());
		ASSERT_MSG(diff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", diff, r_alloc->sl.GetFileName(), r_alloc->sl.GetLine());
	}

	// decrement alloc counters
	// as realloc might change the pointer
	r_st.OnFree(r_alloc->size, r_alloc->sl);

	PPAllocInfo* alloc = (PPAllocInfo*)realloc((void*)r_alloc, sizeof(PPAllocInfo) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	ASSERT_MSG(alloc, "realloc: no mem left!");

	PPMemState& st = PPGetState();

	// actual pointer address
	void* actualPtr = alloc + 1;
	{
		alloc->id = Atomic::Increment(st.allocIdCounter);
		alloc->state = &st;
		alloc->size = size;
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + size);
		for(int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK;
	}

	st.OnAlloc(alloc->size, sl);

	return actualPtr;
#endif // PPMEM_DISABLED
}

IEXPORTS void PPDCheck(void* ptr)
{
#ifndef PPMEM_DISABLED
	PPAllocInfo* alloc = (PPAllocInfo*)ptr - 1;
	if (ptr == nullptr || alloc->checkMark != PPMEM_CHECKMARK)
		return;

	// actual pointer address
	void* actualPtr = ((ubyte*)alloc) + sizeof(PPAllocInfo);
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

	PPAllocInfo* alloc = (PPAllocInfo*)ptr - 1;
	if(alloc->checkMark != PPMEM_CHECKMARK)
	{
		free(ptr);
		return;
	}

	if (PPGetStateList().isEmpty())
	{
		free((void*)alloc);
		return;
	}

	PPMemState& st = *alloc->state;

	{
		// actual pointer address
		void* actualPtr = ((ubyte*)alloc) + sizeof(PPAllocInfo);
		uint* tailCheckMark = (uint*)((ubyte*)actualPtr + alloc->size);

		const int diff = PPMemCmpTailCheckmarks((ubyte*)tailCheckMark);
		ASSERT_MSG(alloc->checkMark == PPMEM_CHECKMARK, "buffer underflow of %s:%d, investigate with ASAN", alloc->sl.GetFileName(), alloc->sl.GetLine());
		ASSERT_MSG(diff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", diff, alloc->sl.GetFileName(), alloc->sl.GetLine());

		// set check marks to indicate freed mem regions
		alloc->checkMark = PPMEM_CHECKMARK_FREED;
		for (int i = 0; i < PPMEM_EXTRA_MARKS; ++i)
			*tailCheckMark++ = PPMEM_CHECKMARK_FREED;
	}

	st.OnFree(alloc->size, alloc->sl);

	free((void*)alloc);
#endif // PPMEM_DISABLED
}
