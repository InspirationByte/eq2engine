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

struct ProfEventWrp
{
public:
	ProfEventWrp(const char* name);
	~ProfEventWrp();
private:
	int eventId{ -1 };
};

#ifdef PROFILE_ENABLE

#define PP_SL PPSourceLine::Make(__FILE__, __LINE__)

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

#define PP_SL PPSourceLine::Empty()

#define PROF_EVENT(name)
#define PROF_EVENT_F()
#define PROF_MARKER(name)
#define PROF_RELEASE_THREAD_MARKERS()

inline ProfEventWrp::ProfEventWrp(const char* name) {};
inline ProfEventWrp::~ProfEventWrp() = default;

#endif // PROFILE_ENABLE