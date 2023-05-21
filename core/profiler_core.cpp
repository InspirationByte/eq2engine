//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#if (WINVER < _WIN32_WINNT_VISTA)
#error "WTF"
#endif
#include <cvmarkersobj.h>
#endif
#include "core/core_common.h"

#include "core/ConCommand.h"
#include "core/ConVar.h"

#ifdef _WIN32

using namespace Concurrency::diagnostic;

struct cvSpanHolder
{
	span* GetSpan() { return (span*)data; }
	char data[sizeof(span)];
};

struct cvEvents
{
	Array<marker_series*> series{ PP_SL };
	Array<cvSpanHolder> pushedEvents{ PP_SL };
	EqWString threadName;
};

static thread_local cvEvents* tlsCV_events = nullptr;

static marker_series* GetTLSMarkerSeries()
{
	static constexpr const int maxThreadName = 128;
	static constexpr const int maxSeriesDepth = 100;

	const uintptr_t threadId = Threading::GetCurrentThreadID();

	if (!tlsCV_events)
	{
		tlsCV_events = PPNew cvEvents();

		char threadName[maxThreadName]{ 0 };
		Threading::GetThreadName(threadId, threadName, maxThreadName);

		tlsCV_events->threadName = threadName;
	}

	const int depth = tlsCV_events->pushedEvents.numElem();
	if (depth < tlsCV_events->series.numElem())
		return tlsCV_events->series[depth];

	ASSERT(depth < maxSeriesDepth);

	EqWString wThreadName;

	if(tlsCV_events->threadName.Length() != 0)
		wThreadName = (depth > 0) ? EqWString::Format(L"%ls - level %d", tlsCV_events->threadName.ToCString(), depth) : tlsCV_events->threadName;
	else
		wThreadName = (depth > 0) ? EqWString::Format(L"Thread %% - level %d", threadId, depth) : EqWString::Format(L"Thread %d", threadId);

	marker_series* newSeries = PPNew marker_series(wThreadName.ToCString());
	tlsCV_events->series.append(newSeries);

	return newSeries;
}

#else

#include <sys/syscall.h>
#include "profiler_json.h"

static thread_local Array<CVTraceEvent>* tlsCV_events = nullptr;

static uint64 GetPerfClock()
{
	timespec ts;
	clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
	return static_cast<uint64>(ts.tv_sec) * 1000000ULL + static_cast<uint64>(ts.tv_nsec) / 1000ULL;
}

static uintptr_t GetPerfCurrentThreadId()
{
	return syscall(SYS_gettid);
}

static Threading::CEqMutex s_traceMutex;
static EqCVTracerJSON s_jsonTracer;
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

#endif // _WIN32

IEXPORTS void ProfAddMarker(const char* text)
{
#ifdef _WIN32
	marker_series* series = GetTLSMarkerSeries();
	span s(*series, high_importance, EqWString(text));
#else

#endif // _WIN32
}

IEXPORTS int ProfBeginMarker(const char* text)
{
	int eventId = 0;

#ifdef _WIN32
	marker_series* series = GetTLSMarkerSeries();

	eventId = tlsCV_events->pushedEvents.numElem();
	cvSpanHolder& spanHld = tlsCV_events->pushedEvents.append();
	EqWString wText(text);
	new(spanHld.data) span(*series, normal_importance, wText.ToCString());
#else
	if(!s_jsonTracer.IsCapturing())
	{
		if(s_startTrace)
		{
			Threading::CScopedMutex m(s_traceMutex);
			s_jsonTracer.Start(ptrace_file.GetString());
		}
		else
		{
			if(tlsCV_events)
				tlsCV_events->clear(true);
			return eventId;
		}
	}
	else if(!s_startTrace)
	{
		Threading::CScopedMutex m(s_traceMutex);
		s_jsonTracer.Stop();
		return eventId;
	}

	if(!tlsCV_events)
		tlsCV_events = PPNew Array<CVTraceEvent>(PP_SL);

	CVTraceEvent& evt = tlsCV_events->append();
	evt.name = text;
	evt.pid = 1000;
	evt.threadId = GetPerfCurrentThreadId();
	evt.timeStamp = GetPerfClock();
	evt.type = EVT_DURATION_BEGIN;

	// need to ensure that events are strictly monotonic per thread
	if(tlsCV_events->numElem() && tlsCV_events->back().timeStamp >= evt.timeStamp)
		evt.timeStamp = tlsCV_events->back().timeStamp + 1;

#endif // _WIN32

	return eventId;
}

IEXPORTS void ProfEndMarker(int eventId)
{
	if (eventId < 0)
		return;

#ifdef _WIN32
	ASSERT(tlsCV_events->pushedEvents.numElem()-1 == eventId);
	tlsCV_events->pushedEvents.back().GetSpan()->~span();
	tlsCV_events->pushedEvents.popBack();
#else
	if(!s_jsonTracer.IsCapturing())
	{
		if(tlsCV_events)
			tlsCV_events->clear(true);
		return;
	}

	const CVTraceEvent startEvt = tlsCV_events->popBack();
	ASSERT_MSG(startEvt.type == EVT_DURATION_BEGIN, "profiler begin event type is invalid");

	const int64 duration = max<int64>(static_cast<int64>(GetPerfClock()) - startEvt.timeStamp, 2);

	CVTraceEvent writeEvt;
	writeEvt.name = startEvt.name;
	writeEvt.threadId = startEvt.threadId;
	writeEvt.pid = startEvt.pid;
	writeEvt.timeStamp = startEvt.timeStamp;
	writeEvt.duration = duration;
	writeEvt.type = EVT_DURATION_BEGIN_END;

	s_jsonTracer.WriteEvent(writeEvt);
#endif // _WIN32
}

IEXPORTS void ProfReleaseCurrentThreadMarkers()
{
#ifdef _WIN32
	if (!tlsCV_events)
		return;

	ASSERT_MSG(tlsCV_events->pushedEvents.numElem() == 0, "Still in performance measure");
	delete tlsCV_events;
	tlsCV_events = nullptr;
#else

#endif
}