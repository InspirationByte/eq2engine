//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqParallelJobs.h"
#include "core/IDkCore.h"
#include "core/IEqCPUServices.h"

using namespace Threading;

EXPORTED_INTERFACE(IEqParallelJobManager, CEqParallelJobManager);


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
	const int numThreadsToSpawn = max(4, g_cpuCaps->GetCPUCount());
	m_jobMng = PPNew CEqJobManager("e2CoreJobMng", numThreadsToSpawn, 8192);

	MsgInfo("*Parallel jobs threads: %d\n", numThreadsToSpawn);

	return true;
}

void CEqParallelJobManager::Shutdown()
{
	m_jobMng->Wait();
	SAFE_DELETE(m_jobMng);
}

// adds the job to the queue
void CEqParallelJobManager::AddJob(IParallelJob* job)
{
	m_jobMng->AddJob(job);
}

// adds the job
void CEqParallelJobManager::AddJob(EQ_JOB_FUNC func, void* args, int count /*= 1*/)
{
	ASSERT(count > 0);

	FunctionParallelJob* job = PPNew FunctionParallelJob("PJob", func, args, count);
	job->m_deleteJob = true;

	m_jobMng->AddJob(job);
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobManager::Submit()
{
	m_jobMng->Submit();
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
