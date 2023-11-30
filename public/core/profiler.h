//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// source-line contailer
struct PPSourceLine
{
	uint64 data{ 0 };

	static PPSourceLine Empty();
	static PPSourceLine Make(const char* filename, int line);

	const char* GetFileName() const;
	int			GetLine() const;
};

// Source-line value constructor helper
template<typename T>
struct PPSLValueCtor
{
	T x;
	PPSLValueCtor<T>(const PPSourceLine& sl) : x() {}
};

// Source-line placement new wrapper for default constructor
template<typename T>
void PPSLPlacementNew(void* item, const PPSourceLine& sl) { new(item) PPSLValueCtor<T>(sl); }

#define PP_SL			PPSourceLine::Make(__FILE__, __LINE__)

#ifdef PROFILE_ENABLE

#define PROF_EVENT(name)				ProfEventWrp _profEvt(name)
#define PROF_EVENT_F()					ProfEventWrp _profEvt(__func__)
#define PROF_MARKER(name)				ProfAddMarker(name)
#define PROF_RELEASE_THREAD_MARKERS()	ProfReleaseCurrentThreadMarkers()

#else

#define PROF_EVENT(name)
#define PROF_MARKER(name)
#define PROF_RELEASE_THREAD_MARKERS()

#endif // PROFILE_ENABLE

IEXPORTS void ProfAddMarker(const char* text);
IEXPORTS int ProfBeginMarker(const char* text);
IEXPORTS void ProfEndMarker(int eventId);
IEXPORTS void ProfReleaseCurrentThreadMarkers();

struct ProfEventWrp
{
public:
	ProfEventWrp(const char* name) { eventId = ProfBeginMarker(name); }
	~ProfEventWrp() { ProfEndMarker(eventId); }
private:
	int eventId{ -1 };
};