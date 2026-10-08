//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#include <tracy/TracyC.h>
#include <tracy/Tracy.hpp>

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"

#ifdef _WIN32
#define USE_CONCURRENCY_VISUALIZER
#endif

#ifdef USE_CONCURRENCY_VISUALIZER

// Concurrency Visualizer profiler on Windows platforms

#if (WINVER < _WIN32_WINNT_VISTA)
#error "WINVER is less than Vista for CV markers, please check if platform.h included correctly"
#endif

#include <cvmarkersobj.h>

using namespace Concurrency::diagnostic;

struct cvSpanHolder
{
	span* GetSpan() { return (span*)data; }
	char data[sizeof(span)];
};

struct cvEvents
{
	FixedArray<cvSpanHolder, 100>	spanStack;
	FixedArray<marker_series*, 100>	seriesStack;
	EqWString threadName;
};

static thread_local cvEvents* tlsCV_events = nullptr;

static marker_series* GetTLSMarkerSeries()
{
	static constexpr const int maxThreadName = 128;

	const uintptr_t threadId = Threading::GetCurrentThreadID();

	if (!tlsCV_events)
	{
		tlsCV_events = PPNew cvEvents();

		char threadName[maxThreadName]{ 0 };
		Threading::GetThreadName(threadId, threadName, maxThreadName);

		AnsiUnicodeConverter(tlsCV_events->threadName, threadName);
	}

	const int depth = tlsCV_events->spanStack.numElem();
	if (depth < tlsCV_events->seriesStack.numElem())
		return tlsCV_events->seriesStack[depth];

	ASSERT(!tlsCV_events->seriesStack.isFull());

	EqWString wThreadName;

	if(tlsCV_events->threadName.Length() != 0)
		wThreadName = (depth > 0) ? EqWString::Format(L"%ls - level %d", tlsCV_events->threadName.ToCString(), depth) : tlsCV_events->threadName;
	else
		wThreadName = (depth > 0) ? EqWString::Format(L"Thread %% - level %d", threadId, depth) : EqWString::Format(L"Thread %d", threadId);

	marker_series* newSeries = PPNew marker_series(wThreadName.ToCString());
	tlsCV_events->seriesStack.append(newSeries);

	return newSeries;
}

static void CVAddMarker(EqStringRef name)
{
	marker_series* series = GetTLSMarkerSeries();

	EqWString str;
	AnsiUnicodeConverter(str, name);
	span s(*series, high_importance, str);
}

static int CVBeginMarker(EqStringRef name)
{
	marker_series* series = GetTLSMarkerSeries();

	thread_local static EqWString wText;
	AnsiUnicodeConverter(wText, name);

	ASSERT(!tlsCV_events->spanStack.isFull());

	const int depth = tlsCV_events->spanStack.numElem();
	cvSpanHolder& spanHld = tlsCV_events->spanStack.append();
	new(spanHld.data) span(*series, normal_importance, wText.ToCString());

	return depth;
}

static void CVEndMarker(int depth)
{
	if (!tlsCV_events)
		return;

	ASSERT(depth == tlsCV_events->spanStack.numElem() - 1);
	tlsCV_events->spanStack.back().GetSpan()->~span();
	tlsCV_events->spanStack.popBack();
}

static void CVReleaseThreadMarkers()
{
	if (!tlsCV_events)
		return;

	ASSERT_MSG(tlsCV_events->spanStack.isEmpty(), "Still in performance measure");
	delete tlsCV_events;
	tlsCV_events = nullptr;
}

#else

// JSON profiler on systems where CV and PIX are not available

#include <unistd.h>
#include "profiler_json.h"

using JSONTraceEventStack = FixedArray<JSONTraceEvent, 100>;

static thread_local JSONTraceEventStack* tlsJSON_events = nullptr;

static uint64 GetPerfClock()
{
	timespec ts;
	clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
	return static_cast<uint64>(ts.tv_sec) * 1000000ULL + static_cast<uint64>(ts.tv_nsec) / 1000ULL;
}

static uintptr_t GetPerfCurrentThreadId()
{
	return gettid(); //syscall(SYS_gettid);
}

static EqJSONTracer s_jsonTracer;
static bool s_startTrace = false;

DECLARE_CVAR(ptrace_file, "ptrace_eq2.json", "Performance trace file name", CV_ARCHIVE);
DECLARE_CMD(ptrace_start, "Performance trace start", 0)
{
	s_startTrace = true;
}

DECLARE_CMD(ptrace_stop, "Performance trace start", 0)
{
	s_startTrace = false;
}

static void CVAddMarker(EqStringRef name)
{
}

static int CVBeginMarker(EqStringRef name)
{
	if (!s_jsonTracer.IsCapturing())
	{
		if (s_startTrace)
		{
			s_jsonTracer.Start(ptrace_file.GetString());
		}
		else
		{
			if (tlsJSON_events)
				tlsJSON_events->clear(true);
			return -1;
		}
	}
	else if (!s_startTrace)
	{
		s_jsonTracer.Stop();
		return -1;
	}

	if (!tlsJSON_events)
		tlsJSON_events = PPNew JSONTraceEventStack();

	const int depth = tlsJSON_events->numElem();

	ASSERT(!tlsJSON_events->isFull());

	JSONTraceEvent& evt = tlsJSON_events->append();
	evt.name = name;
	evt.pid = 1000;
	evt.threadId = GetPerfCurrentThreadId();
	evt.timeStamp = GetPerfClock();
	evt.type = EVT_DURATION_BEGIN;

	// need to ensure that events are strictly monotonic per thread
	if (tlsJSON_events->numElem() && tlsJSON_events->back().timeStamp >= evt.timeStamp)
		evt.timeStamp = tlsJSON_events->back().timeStamp + 1;

	return depth;
}

static void CVEndMarker(int depth)
{
	if (!s_jsonTracer.IsCapturing())
	{
		if (tlsJSON_events)
			tlsJSON_events->clear(true);
		return;
	}

	ASSERT(depth == tlsJSON_events->numElem() - 1);

	JSONTraceEvent writeEvt = tlsJSON_events->popBack();
	ASSERT_MSG(writeEvt.type == EVT_DURATION_BEGIN, "profiler begin event type is invalid");

	writeEvt.id = tlsJSON_events->numElem();
	writeEvt.duration = max<int64>(static_cast<int64>(GetPerfClock()) - writeEvt.timeStamp, 1);
	writeEvt.type = EVT_DURATION_BEGIN_END;

	s_jsonTracer.WriteEvent(writeEvt);
}

static void CVReleaseThreadMarkers()
{
	if (!s_jsonTracer.IsCapturing())
	{
		if (tlsJSON_events)
			tlsJSON_events->clear(true);
	}
}

#endif // _WIN32

#ifdef TRACY_ENABLE

struct TracyZoneStack
{
	FixedArray<tracy::ScopedZone, 100> stack;
};

static thread_local TracyZoneStack tlsTracy_events;

#endif

IEXPORTS void ProfAddMarker(EqStringRef file, int line, EqStringRef name)
{
	CVAddMarker(name);
}

IEXPORTS int ProfBeginMarker(EqStringRef file, int line, EqStringRef name)
{
	int depth = CVBeginMarker(name);

#ifdef TRACY_ENABLE
	depth = tlsTracy_events.stack.numElem();
	//uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, int32_t depth, bool is_active = true
	tlsTracy_events.stack.appendEmplace((uint32_t)TracyLine, file.ToCString(), (size_t)file.Length(), TracyFunction, strlen(TracyFunction), name.ToCString(), (size_t)name.Length(), TRACY_CALLSTACK, true);
#endif

	return depth;
}
		
IEXPORTS void ProfEndMarker(int depth)
{
	if (depth < 0)
		return;

#ifdef TRACY_ENABLE
	ASSERT(depth == tlsTracy_events.stack.numElem() - 1);

	tlsTracy_events.stack.removeIndex(tlsTracy_events.stack.numElem()-1);
#endif

	CVEndMarker(depth);
}

IEXPORTS void ProfBeginFrameMark(EqStringRef name)
{
#ifdef TRACY_ENABLE
	tracy::Profiler::SendFrameMark(nullptr);
	tracy::Profiler::SendFrameMark(name, tracy::QueueType::FrameMarkMsgStart);
#endif
}

IEXPORTS void ProfEndFrameMark(EqStringRef name)
{
#ifdef TRACY_ENABLE
	tracy::Profiler::SendFrameMark(name, tracy::QueueType::FrameMarkMsgEnd);
#endif
}

IEXPORTS void ProfReleaseCurrentThreadMarkers()
{
	CVReleaseThreadMarkers();
}