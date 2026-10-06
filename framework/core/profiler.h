//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/stringref.h"

#if !defined(_RETAIL)
#define PROFILE_ENABLE
#endif

#ifdef PROFILE_ENABLE

IEXPORTS void ProfAddMarker(EqStringRef file, int line, EqStringRef name);
IEXPORTS int ProfBeginMarker(EqStringRef file, int line, EqStringRef name);
IEXPORTS void ProfEndMarker(int depth);

IEXPORTS void ProfBeginFrameMark(EqStringRef name);
IEXPORTS void ProfEndFrameMark(EqStringRef name);

IEXPORTS void ProfReleaseCurrentThreadMarkers();

#define PROF_EVENT(name)				ProfEventWrp _profEvt(__FILE__, __LINE__, name)
#define PROF_EVENT_F()					ProfEventWrp _profEvt(__FILE__, __LINE__, __func__)	
#define PROF_MARKER(name)				ProfAddMarker(__FILE__, __LINE__, name)
#define PROF_RELEASE_THREAD_MARKERS()	ProfReleaseCurrentThreadMarkers()

#define PROF_FRAME_BEGIN(name)			ProfBeginFrameMark(name)
#define PROF_FRAME_END(name)			ProfEndFrameMark(name)

struct ProfEventWrp
{
public:
	ProfEventWrp(EqStringRef file, int line, EqStringRef name);
	~ProfEventWrp();
private:
	int depth{-1};
};

inline ProfEventWrp::ProfEventWrp(EqStringRef file, int line, EqStringRef name)
{
	depth = ProfBeginMarker(file, line, name);
}

inline ProfEventWrp::~ProfEventWrp()
{
	ProfEndMarker(depth);
}

#else

#define PROF_EVENT(name)
#define PROF_EVENT_F()
#define PROF_MARKER(name)
#define PROF_RELEASE_THREAD_MARKERS()

#define PROF_FRAME_MARK
#define PROF_FRAME_BEGIN(name)
#define PROF_FRAME_END(name)

#endif // PROFILE_ENABLE