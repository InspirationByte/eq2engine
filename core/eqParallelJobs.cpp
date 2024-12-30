//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"
#include "eqParallelJobs.h"
#include "core/IDkCore.h"
#include "core/IEqCPUServices.h"

using namespace Threading;

EXPORTED_INTERFACE(IEqParallelJobManager, CEqParallelJobManager);

DECLARE_CVAR(job_threads, "-1", nullptr, CV_CHEAT | CV_UNREGISTERED);
DECLARE_CVAR(job_debugProcessAffinity, "-1", nullptr, CV_CHEAT | CV_UNREGISTERED);

CEqParallelJobManager::CEqParallelJobManager()
{
	// required by mobile port
	g_eqCore->RegisterInterface(this);
}

CEqParallelJobManager::~CEqParallelJobManager()
{
	g_eqCore->UnregisterInterface<CEqParallelJobManager>();
}

// creates new job thread
bool CEqParallelJobManager::Init()
{
#ifndef _RETAIL
	g_consoleCommands->RegisterCommand(&job_threads);
	g_consoleCommands->RegisterCommand(&job_debugProcessAffinity);
#endif

#ifdef _WIN32
	if (job_debugProcessAffinity.GetInt() != -1)
	{
		SetCurrentProcessAffinity(1 << job_debugProcessAffinity.GetInt());
	}
#endif

	constexpr int jobQueueSize = 8192;

	const int numThreadsToSpawn = job_threads.GetInt() == -1 ? max(4, g_cpuCaps->GetCPUCount()) : job_threads.GetInt();
	m_jobMng = PPNew CEqJobManager("e2CoreJobMng", numThreadsToSpawn, jobQueueSize);

	MsgInfo("* Job threads: %d\n", m_jobMng->GetJobThreadsCount());

	return true;
}

void CEqParallelJobManager::Shutdown()
{
#ifndef _RETAIL
	g_consoleCommands->UnregisterCommand(&job_threads);
	g_consoleCommands->UnregisterCommand(&job_debugProcessAffinity);
#endif
	SAFE_DELETE(m_jobMng);
}

// adds the job to the queue
void CEqParallelJobManager::AddJob(IParallelJob* job)
{
	job->InitJob();
	m_jobMng->StartJob(job);
}

// adds the job
void CEqParallelJobManager::AddJob(EQ_JOB_FUNC func, void* args, int count /*= 1*/)
{
	ASSERT(count > 0);

	FunctionJob* job = PPNew FunctionJob("PJob", func, args, count);
	job->DeleteOnFinish();
	m_jobMng->InitStartJob(job);
}

bool CEqParallelJobManager::AllJobsCompleted() const
{
	return m_jobMng->AllJobsCompleted();
}

// wait for completion
void CEqParallelJobManager::Wait(int waitTimeout)
{
	m_jobMng->Wait(waitTimeout);
}
