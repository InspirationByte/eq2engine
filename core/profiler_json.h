//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IVirtualStream;
using IFile = IVirtualStream;

enum ETraceEvtType
{
	EVT_UNKNOWN = 0,

	EVT_THREAD_NAME,
	EVT_DURATION_BEGIN,
	EVT_DURATION_END,
	EVT_DURATION_BEGIN_END,
};

struct CVTraceEvent
{
	EqString 		name;
	uintptr_t 		threadId{ 0 };
	uint64 			pid{ 0 };
	ETraceEvtType 	type{ EVT_UNKNOWN };
	uint64			timeStamp{ 0 };
	int64			duration{ 0 };	// microseconds
	uint64			id{ 0 };
};

class EqCVTracerJSON
{
public:
	bool 			Start(const char* fileName);
	void			Stop();

	void			WriteEvent(const CVTraceEvent& evt);

	bool			IsCapturing() const { return Atomic::Load(m_captureInProgress); }
	uint64			AllocEventId();
private:	
	void			FlushTempBuffer();
	void			EventToString(EqString& out, const CVTraceEvent& evt);

	EqString			m_batchPrefix;
	Array<CVTraceEvent>	m_tmpBuffer{ PP_SL };
	Set<uintptr_t>		m_threadMaskData{ PP_SL };
	IVirtualStreamPtr	m_outFile{ nullptr };
	uint64				m_eventId{ 0 };
	int					m_captureInProgress{ 0 };
};