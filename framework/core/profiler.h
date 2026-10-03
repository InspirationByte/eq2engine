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
#include <tracy/Tracy.hpp>

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

#define PROF_EVENT(name)				ZoneTransientN(___tracy_scoped_zone, name, true); ProfEventWrp _profEvt(name)
#define PROF_EVENT_F()					ZoneScoped; ProfEventWrp _profEvt(__func__)	
#define PROF_MARKER(name)				ZoneText(name); ProfAddMarker(name)
#define PROF_RELEASE_THREAD_MARKERS()	ProfReleaseCurrentThreadMarkers()

#define PROF_FRAME_MARK					FrameMark; ProfAddMarker("FRAME")

inline ProfEventWrp::ProfEventWrp(const char* name)	{ eventId = ProfBeginMarker(name); }
inline ProfEventWrp::~ProfEventWrp()				{ ProfEndMarker(eventId); }

#else

#define PROF_EVENT(name)
#define PROF_EVENT_F()
#define PROF_MARKER(name)
#define PROF_RELEASE_THREAD_MARKERS()

#define PROF_FRAME_MARK

inline ProfEventWrp::ProfEventWrp(const char* name) {};
inline ProfEventWrp::~ProfEventWrp() = default;

#endif // PROFILE_ENABLE