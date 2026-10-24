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

constexpr const uint PPMEM_IDENT		   = MAKECHAR4('P','P','I','D');
constexpr const uint PPMEM_IDENT_FREED	   = MAKECHAR4('P','P','F','R');
constexpr const uint PPMEM_CHECKMARK	   = MAKECHAR4('P','P','M','E');
constexpr const uint PPMEM_CHECKMARK_FREED = MAKECHAR4('F','R','E','E');
constexpr const uint PPMEM_HEAD_MARKS = 8;
constexpr const uint PPMEM_EXTRA_MARKS = 20;

constexpr const uint PPMEM_MAX_STAT_THREADS = 128;

struct PPMemState;

struct PPAllocInfo
{
	// identifies tracked block, placed far from user data so underflow damages check marks first
	uint			ident;
	uint			id;
	PPMemState*		state{nullptr};

	int64			size : 48;
	int64			alignment : 16;
	PPSourceLine	sl;

	uint			checkMarks[PPMEM_HEAD_MARKS];
};

// header size keeps user pointer aligned for any power of two alignment up to its size
static_assert(sizeof(PPAllocInfo) == 64, "PPAllocInfo size must be 64 bytes");

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

struct PPMemStateList : public FixedArray<PPMemState*, PPMEM_MAX_STAT_THREADS> {};

static PPMemStateList* s_memStateList;
static PPMemStateList& PPGetStateList()
{
	static PPMemStateList s_memStates;
	s_memStateList = &s_memStates;
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

#ifdef _WIN32

#if defined(CRT_DEBUG_ENABLED)
#	define PPInternalMalloc(s)				_malloc_dbg(s, _NORMAL_BLOCK, pszFileName, nLine)
#	define PPInternalReAlloc(p, s)			_realloc_dbg(p, s, _NORMAL_BLOCK, pszFileName, nLine)
#	define PPInternalAlignedMalloc(s, a)	_aligned_malloc_dbg(s, a, pszFileName, nLine)
#else
#	define PPInternalMalloc(s)				malloc(s)
#	define PPInternalReAlloc(p, s)			realloc(p, s)
#	define PPInternalAlignedMalloc(s, a)	_aligned_malloc(s, a)
#endif

#define PPInternalFree			free
#define PPInternalAlignedFree	_aligned_free

#else

#define PPInternalMalloc(s)		malloc(s)
#define PPInternalReAlloc(p, s)	realloc(p, s)

void* PPInternalAlignedMalloc(size_t size, size_t alignment)
{ 
	void* _block = malloc(size);
	if(!_block)
		return nullptr;
	posix_memalign(&_block, alignment, size); 
	return _block; 
}

#define PPInternalFree			free
#define PPInternalAlignedFree	free

#endif // _WIN32

// returns number of damaged bytes in check marks
static int PPMemCmpCheckmarks(const ubyte* x, int numMarks)
{
	const uint checkmark = PPMEM_CHECKMARK;
	const ubyte* y = (ubyte*)&checkmark;

	int diff = 0;
	for (size_t i = 0; i < numMarks * sizeof(uint); i++)
		diff += (x[i] != y[i & 3]);

	return diff;
}

static int PPMemCmpTailCheckmarks(const ubyte* x)
{
	return PPMemCmpCheckmarks(x, PPMEM_EXTRA_MARKS);
}

static int PPMemCmpHeadCheckmarks(const PPAllocInfo* alloc)
{
	return PPMemCmpCheckmarks(reinterpret_cast<const ubyte*>(alloc->checkMarks), PPMEM_HEAD_MARKS);
}

static void PPMemSetCheckmarks(uint* marks, int numMarks, uint value)
{
	for (int i = 0; i < numMarks; ++i)
		marks[i] = value;
}

static void PPMemCheckBlock(const PPAllocInfo* alloc)
{
	const uint* tailCheckMark = reinterpret_cast<const uint*>(reinterpret_cast<const ubyte*>(alloc + 1) + alloc->size);

	const int headDiff = PPMemCmpHeadCheckmarks(alloc);
	const int tailDiff = PPMemCmpTailCheckmarks(reinterpret_cast<const ubyte*>(tailCheckMark));
	ASSERT_MSG(headDiff == 0, "buffer underflow by %d bytes of %s:%d, investigate with ASAN", headDiff, alloc->sl.GetFileName(), alloc->sl.GetLine());
	ASSERT_MSG(tailDiff == 0, "buffer overflow by %d bytes of %s:%d, investigate with ASAN", tailDiff, alloc->sl.GetFileName(), alloc->sl.GetLine());
}

IEXPORTS void PPDCheck(void* ptr)
{
#ifndef PPMEM_DISABLED
	PPAllocInfo* alloc = (PPAllocInfo*)ptr - 1;
	if (ptr == nullptr || alloc->ident != PPMEM_IDENT)
		return;

	PPMemCheckBlock(alloc);
#endif
}

#ifndef PPMEM_DISABLED
static void* PPInitAllocInternal(void* alloc, size_t size, size_t alignment, const PPSourceLine& sl)
{
	if (!alloc)
	{
		ASSERT_FAIL("alloc: no mem left");
		return nullptr;
	}

	void* userPtr = reinterpret_cast<ubyte*>(alloc) + max(sizeof(PPAllocInfo), alignment);
	PPAllocInfo* allocInfo = reinterpret_cast<PPAllocInfo*>(userPtr) - 1;

	PPMemState& st = PPGetState();

	// actual pointer address
	{
		allocInfo->sl = sl;
		allocInfo->size = size;
		allocInfo->alignment = alignment;
		allocInfo->id = Atomic::Increment(st.allocIdCounter);
		allocInfo->state = &st;

		allocInfo->ident = PPMEM_IDENT;
		PPMemSetCheckmarks(allocInfo->checkMarks, PPMEM_HEAD_MARKS, PPMEM_CHECKMARK);
		PPMemSetCheckmarks((uint*)((ubyte*)userPtr + size), PPMEM_EXTRA_MARKS, PPMEM_CHECKMARK);
	}

	st.OnAlloc(allocInfo->size, sl);

	if( ppmem_breakOnAlloc.GetInt() != -1 && allocInfo->id == (uint)ppmem_breakOnAlloc.GetInt())
		ASSERT_FAIL("PPDAlloc: Break on allocation id=%d", allocInfo->id);

	//TracyAlloc(userPtr, size);

	return userPtr;
}
#endif // !PPMEM_DISABLED

// allocated debuggable memory block
void* PPDAlloc(size_t size, const PPSourceLine& sl)
{
#ifdef PPMEM_DISABLED
	void* mem = PPInternalMalloc(size);
	ASSERT_MSG(mem, "No mem left");
	//TracyAlloc(mem, size);
	return mem;
#else

	if (sl.data == 0) 
	{
		void* mem = PPInternalMalloc(size);
		ASSERT_MSG(mem, "alloc: no mem left");
		//TracyAlloc(mem, size);
		return mem;
	}

	void* alloc = PPInternalMalloc(sizeof(PPAllocInfo) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	return PPInitAllocInternal(alloc, size, 0, sl);
#endif // PPMEM_DISABLED
}

void* PPDAlignedAlloc(size_t size, size_t alignment, const PPSourceLine& sl)
{
#ifdef PPMEM_DISABLED
	void* mem = PPInternalMalloc(size);
	ASSERT_MSG(mem, "No mem left");
	//TracyAlloc(mem, size);
	return mem;
#else

	if (sl.data == 0) 
	{
		void* mem = PPInternalAlignedMalloc(size, alignment);
		ASSERT_MSG(mem, "alloc: no mem left");
		//TracyAlloc(mem, size);	
		return mem;
	}

	const size_t alignedAllocInfoSize = max(sizeof(PPAllocInfo), alignment);

	void* alloc = PPInternalAlignedMalloc(alignedAllocInfoSize + size + sizeof(uint) * PPMEM_EXTRA_MARKS, alignment);
	return PPInitAllocInternal(alloc, size, alignment, sl);
#endif // PPMEM_DISABLED
}

// reallocates memory block
void* PPDReAlloc( void* ptr, size_t size, const PPSourceLine& sl )
{
#ifdef PPMEM_DISABLED
	//if(ptr) TracyFree(ptr);

	void* mem = realloc(ptr, size);
	ASSERT_MSG(mem, "alloc: no mem left");

	//TracyAlloc(mem, size);

	return mem;
#else
	if (ptr == nullptr)
	{
		return PPDAlloc(size, sl);
	}

	// tracking is a property of the block chosen when it was allocated, reallocation keeps it:
	// untracked block stays untracked (its size is unknown to copy into tracked one),
	// tracked block stays tracked even when source line is not provided
	PPAllocInfo* r_alloc = (PPAllocInfo*)ptr - 1;
	if (r_alloc->ident != PPMEM_IDENT)
	{
		void* mem = PPInternalReAlloc(ptr, size);
		ASSERT_MSG(mem, "realloc: no mem left!");
		return mem;
	}

	ASSERT_MSG(r_alloc->alignment == 0, "ReAlloc used on aligned allocation - not supported yet");

	PPMemState& r_st = *r_alloc->state;
	PPMemCheckBlock(r_alloc);

	// keep allocation attributed to its original place if caller has no source line
	const PPSourceLine newSl = sl.data ? sl : r_alloc->sl;

	//TracyFree(ptr);

	// decrement allocInfo counters
	// as realloc might change the pointer
	r_st.OnFree(r_alloc->size, r_alloc->sl);

	PPAllocInfo* alloc = (PPAllocInfo*)PPInternalReAlloc((void*)r_alloc, sizeof(PPAllocInfo) + size + sizeof(uint) * PPMEM_EXTRA_MARKS);
	if (!alloc)
	{
		// old block is still valid
		ASSERT_FAIL("realloc: no mem left!");
		r_st.OnAlloc(r_alloc->size, r_alloc->sl);
		return nullptr;
	}

	PPMemState& st = PPGetState();

	// actual pointer address
	void* newUserPtr = alloc + 1;
	{
		alloc->id = Atomic::Increment(st.allocIdCounter);
		alloc->state = &st;
		alloc->size = size;
		alloc->sl = newSl;

		// restore head marks in case underflow was reported
		PPMemSetCheckmarks(alloc->checkMarks, PPMEM_HEAD_MARKS, PPMEM_CHECKMARK);
		PPMemSetCheckmarks((uint*)((ubyte*)newUserPtr + size), PPMEM_EXTRA_MARKS, PPMEM_CHECKMARK);
	}

	st.OnAlloc(alloc->size, alloc->sl);
	//TracyAlloc(newUserPtr, size);

	return newUserPtr;
#endif // PPMEM_DISABLED
}

static void PPFreeAllocInternal(PPAllocInfo* alloc)
{
	PPMemState& st = *alloc->state;

	PPMemCheckBlock(alloc);

	// set check marks to indicate freed mem regions
	alloc->ident = PPMEM_IDENT_FREED;
	PPMemSetCheckmarks(alloc->checkMarks, PPMEM_HEAD_MARKS, PPMEM_CHECKMARK_FREED);
	PPMemSetCheckmarks((uint*)((ubyte*)(alloc + 1) + alloc->size), PPMEM_EXTRA_MARKS, PPMEM_CHECKMARK_FREED);

	st.OnFree(alloc->size, alloc->sl);
}

void PPFree(void* ptr)
{
#ifdef PPMEM_DISABLED
	//TracyFree(ptr);
	PPInternalFree(ptr);
#else

	if(ptr == nullptr)
		return;

	//TracyFree(ptr);

	PPAllocInfo* allocInfo = reinterpret_cast<PPAllocInfo*>(ptr) - 1;
	if(allocInfo->ident != PPMEM_IDENT)
	{
		PPInternalFree(ptr);
		return;
	}

	ASSERT_MSG(allocInfo->alignment == 0, "Misuse of Free on aligned allocation");
	void* alloc = reinterpret_cast<ubyte*>(ptr) - max(sizeof(PPAllocInfo), size_t(allocInfo->alignment));

	if(!PPGetStateList().isEmpty())
		PPFreeAllocInternal(allocInfo);

	PPInternalFree(allocInfo);
#endif // PPMEM_DISABLED
}

void PPAlignedFree(void* ptr)
{
#ifdef PPMEM_DISABLED
	//TracyFree(ptr);
	PPInternalAlignedFree(ptr);
#else

	if(ptr == nullptr)
		return;

	//TracyFree(ptr);

	PPAllocInfo* allocInfo = (PPAllocInfo*)ptr - 1;
	if(allocInfo->ident != PPMEM_IDENT)
	{
		PPInternalAlignedFree(ptr);
		return;
	}

	ASSERT_MSG(allocInfo->alignment != 0, "Misuse of Free on unaligned allocation");

	void* alloc = reinterpret_cast<ubyte*>(ptr) - max(sizeof(PPAllocInfo), size_t(allocInfo->alignment));
	if(!PPGetStateList().isEmpty())
		PPFreeAllocInternal(allocInfo);

	PPInternalAlignedFree(alloc);
#endif // PPMEM_DISABLED
}