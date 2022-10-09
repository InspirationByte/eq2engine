//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

#endif // _WIN32

IEXPORTS void ProfAddMarker(const char* text)
{
#ifdef _WIN32
	marker_series* series = GetTLSMarkerSeries();
	span s(*series, high_importance, EqWString(text));
#endif // _WIN32
}

IEXPORTS int ProfBeginMarker(const char* text)
{
	int eventId = -1;

#ifdef _WIN32
	marker_series* series = GetTLSMarkerSeries();

	eventId = tlsCV_events->pushedEvents.numElem();
	cvSpanHolder& spanHld = tlsCV_events->pushedEvents.append();
	EqWString wText(text);
	new(spanHld.data) span(*series, normal_importance, wText.ToCString());
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
#endif
}