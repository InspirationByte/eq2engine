//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/IEqParallelJobs.h"

#include "profiler_json.h"

static Threading::CEqSignal s_jsonTracerWriteCompleted;
static Threading::CEqMutex s_jsonTracerMutex;
using namespace Threading;

static constexpr const int bufferThreshold = 1000;

bool EqCVTracerJSON::Start(const char* fileName)
{
	if(Atomic::Load(m_captureInProgress) != 0)
		return false;

	CScopedMutex m(s_jsonTracerMutex);

    IFilePtr file = g_fileSystem->Open(fileName, "wb", SP_ROOT);

	if(!file)
		return false;

	Msg("----- PERF TRACE START -----\n");
	Msg("output file: %s\n", fileName);

	{
		file->Print("{\"traceEvents\":[");
		s_jsonTracerWriteCompleted.Raise();

		m_outFile = file;
		m_batchPrefix.Empty();
		m_threadMaskData.clear(true);
		m_tmpBuffer.reserve(bufferThreshold);

		Atomic::Store(m_captureInProgress, 1);
	}

	return true;
}

void EqCVTracerJSON::Stop()
{
	if(Atomic::Load(m_captureInProgress) == 0)
		return;
	Atomic::Store(m_captureInProgress, 0);

	CScopedMutex m(s_jsonTracerMutex);
	FlushTempBuffer();

	s_jsonTracerWriteCompleted.Wait();
	m_outFile->Print("]}");
    m_outFile = nullptr;

    m_batchPrefix.Empty();
    m_threadMaskData.clear(true);
	m_eventId = 0;

    Msg("----- PERF TRACE COMPLETED -----\n");
}

void EqCVTracerJSON::WriteEvent(const CVTraceEvent& evt)
{
	if(Atomic::Load(m_captureInProgress) == 0)
		return;

	{
		CScopedMutex m(s_jsonTracerMutex);
		if(m_tmpBuffer.numElem() > bufferThreshold)
			FlushTempBuffer();

		m_tmpBuffer.append(evt);

		if(m_threadMaskData.contains(evt.threadId))
			return;
		m_threadMaskData.insert(evt.threadId);
	}

	char threadName[64];
	Threading::GetThreadName(evt.threadId, threadName, sizeof(threadName));
	if(*threadName == 0)
		return;

	// need to put thread name event
	{
		CScopedMutex m(s_jsonTracerMutex);
		CVTraceEvent& metaDataEvt = m_tmpBuffer.append();
		
		metaDataEvt.type = EVT_THREAD_NAME;
		metaDataEvt.pid = evt.pid;
		metaDataEvt.threadId = evt.threadId;
		metaDataEvt.name = threadName;
	}
}

uint64 EqCVTracerJSON::AllocEventId()
{
	return Atomic::Increment(m_eventId);
}

void EqCVTracerJSON::FlushTempBuffer()
{
	if(Atomic::Load(m_captureInProgress) == 0)
		return;

	s_jsonTracerWriteCompleted.Clear();

	bool initialStart = false;
	if(m_batchPrefix.Length() == 0)
	{
		initialStart = true;
		m_batchPrefix = ",";
	}

	Array<CVTraceEvent>* writeBuffer = PPNew Array<CVTraceEvent>(PP_SL);
	writeBuffer->swap(m_tmpBuffer);
	g_parallelJobs->AddJob(JOB_TYPE_ANY, [_initialStart = initialStart, this, writeBuffer](void*, int) {
		CScopedMutex m(s_jsonTracerMutex);

		bool initialStart = _initialStart;

		EqString str;
		for(int i = 0; i < writeBuffer->numElem(); ++i)
		{
			if(!initialStart)
				m_outFile->Write(m_batchPrefix.GetData(), 1, m_batchPrefix.Length());
			else
				initialStart = false;

			EventToString(str, writeBuffer->ptr()[i]);
			m_outFile->Write(str.GetData(), 1, str.Length());
		}

		s_jsonTracerWriteCompleted.Raise();

		delete writeBuffer;
	});
	g_parallelJobs->Submit();

	m_tmpBuffer.reserve(bufferThreshold);
}

void EqCVTracerJSON::EventToString(EqString& out, const CVTraceEvent& evt)
{
	EqString evtName = evt.name;
	EqString evtArgs;
	char ph;
	switch(evt.type)
	{
		case EVT_DURATION_BEGIN:
			ph = 'B';
			break;
		case EVT_DURATION_END:
			ph = 'E';
			break;
		case EVT_DURATION_BEGIN_END:
			ph = 'X';
			break;
		case EVT_THREAD_NAME:
			evtArgs = EqString::Format("\"name\": \"%s\"", evt.name.ToCString());
			evtName = "thread_name";
			ph = 'M';
			break;
		default:
			ASSERT_FAIL("incorrect event type %d", evt.type);
	}

	out = EqString::Format(
		"{"
		"\"cat\":\"e2\","
		"\"ph\":\"%c\","
		"\"id\":\"%" PRIu64 "\","
		"\"pid\":%" PRIu64 "," 
		"\"tid\":%" PRIu64 ","
		"\"ts\":%" PRIu64 ","
		"\"dur\":%" PRId64 ","
		"\"name\":\"%s\","
		"\"args\":{%s}"
		"}\n", 
		ph, 
		evt.id, 
		evt.pid, 
		evt.threadId,
		evt.timeStamp, 
		(evt.type == EVT_DURATION_BEGIN_END) ? evt.duration : 0,
		evtName.ToCString(), 
		evtArgs.ToCString()
	);
}