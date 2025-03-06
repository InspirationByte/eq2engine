//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#if !defined(_RETAIL)
#define PROFILE_ENABLE
#endif

struct ProfEventWrp
{
public:
	ProfEventWrp(const char* name);
	~ProfEventWrp();
private:
	int eventId{ -1 };
};

#ifdef PROFILE_ENABLE

IEXPORTS void ProfAddMarker(const char* text);
IEXPORTS int ProfBeginMarker(const char* text);
IEXPORTS void ProfEndMarker(int eventId);
IEXPORTS void ProfReleaseCurrentThreadMarkers();

#define PROF_EVENT(name)				ProfEventWrp _profEvt(name)
#define PROF_EVENT_F()					ProfEventWrp _profEvt(__func__)
#define PROF_MARKER(name)				ProfAddMarker(name)
#define PROF_RELEASE_THREAD_MARKERS()	ProfReleaseCurrentThreadMarkers()

inline ProfEventWrp::ProfEventWrp(const char* name)	{ eventId = ProfBeginMarker(name); }
inline ProfEventWrp::~ProfEventWrp()				{ ProfEndMarker(eventId); }

#else

#define PROF_EVENT(name)
#define PROF_EVENT_F()
#define PROF_MARKER(name)
#define PROF_RELEASE_THREAD_MARKERS()

inline ProfEventWrp::ProfEventWrp(const char* name) {};
inline ProfEventWrp::~ProfEventWrp() = default;

#endif // PROFILE_ENABLE