//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqParallelJobs.h"
#include "core/IDkCore.h"

using namespace Threading;

EXPORTED_INTERFACE(IEqParallelJobManager, CEqParallelJobManager);

CEqJobThread::CEqJobThread(CEqParallelJobManager* owner, int jobTypeId) 
	: m_owner(owner), 
	m_curJob(nullptr), 
	m_threadJobTypeId(jobTypeId)
{

}

int CEqJobThread::Run()
{
	// thread will find job by himself
	while( m_owner->AssignFreeJob( this ) )
	{
		ParallelJob* job = const_cast<ParallelJob*>(m_curJob);
		
		job->flags |= JOB_FLAG_CURRENT;

		// execute
		int iter = 0;
		job->numIterOrig = job->numIter;
		while(job->numIter-- > 0)
		{
			job->func(job->arguments, iter );
			iter++;
		}

		job->flags |= JOB_FLAG_EXECUTED;
		job->flags &= ~JOB_FLAG_CURRENT;

		if (job->onComplete)
		{
			m_owner->AddCompleted(job);
		}
		else if (job->flags & JOB_FLAG_DELETE)
		{
			delete job;
		}

		m_curJob = nullptr;
	}

	return 0;
}

bool CEqJobThread::TryAssignJob( ParallelJob* job )
{
	if( m_curJob )
		return false;

	if(job->threadId != 0)
		return false;

	// job only for specific thread?
	if (job->typeId != JOB_TYPE_ANY && m_threadJobTypeId != JOB_TYPE_ANY)
	{
		if (job->typeId != m_threadJobTypeId)
			return false;
	}

	job->threadId = GetThreadID();
	m_curJob = job;

	return true;
}

const ParallelJob* CEqJobThread::GetCurrentJob() const
{
	return const_cast<ParallelJob*>(m_curJob);
}

//-------------------------------------------------------------------------------------------

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
bool CEqParallelJobManager::Init(ArrayCRef<ParallelJobThreadDesc> jobTypes)
{
	int numThreadsSpawned = 0;

	for (const ParallelJobThreadDesc& jobType : jobTypes)
	{
		for (int j = 0; j < jobType.numThreads; j++)
		{
			CEqJobThread* jobThread = PPNew CEqJobThread(this, jobType.jobTypeId);
			m_jobThreads.append(jobThread);

			jobThread->StartWorkerThread(EqString::Format("eqWorker_%d_%d", jobType.jobTypeId, j).ToCString());
			numThreadsSpawned++;
		}
	}

	MsgInfo("*Parallel jobs threads: %d\n", numThreadsSpawned);

	m_mainThreadId = Threading::GetCurrentThreadID();

	return true;
}

void CEqParallelJobManager::Shutdown()
{
	Wait();

	for (CEqJobThread* jobThread : m_jobThreads)
		delete jobThread;

	m_jobThreads.clear(true);
	m_workQueue.clear(true);
	m_completedJobs.clear(true);
}

// adds the job
void CEqParallelJobManager::AddJob(EJobType jobTypeId, EQ_JOB_FUNC func, void* args, int count /*= 1*/, EQ_JOB_COMPLETE_FUNC completeFn /*= nullptr*/)
{
	ASSERT(count > 0);

	ParallelJob* job = PPNew ParallelJob(jobTypeId, std::move(func), args, count, std::move(completeFn));
	job->flags = JOB_FLAG_DELETE;

	{
		CScopedMutex m(m_mutex);
		m_workQueue.append(job);
	}
}

void CEqParallelJobManager::AddJob(ParallelJob* job)
{
	CScopedMutex m(m_mutex);
	m_workQueue.append( job );
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobManager::Submit()
{
	CompleteJobCallbacks();

	if (!m_workQueue.numElem())
		return;

	for (CEqJobThread* jobThread : m_jobThreads)
		jobThread->SignalWork();
}

void CEqParallelJobManager::CompleteJobCallbacks()
{
	// only for main thread
	if (Threading::GetCurrentThreadID() != m_mainThreadId)
		return;

	// execute all job completion callbacks
	{
		CScopedMutex m(m_completeMutex);

		for (ParallelJob* job : m_completedJobs)
		{
			const bool deleteJob = (job->flags & JOB_FLAG_DELETE);

			job->onComplete(job->arguments, job->numIterOrig);

			// done with it
			if (deleteJob)
				delete job;
		}
		m_completedJobs.clear(false);
	}
}

bool CEqParallelJobManager::AllJobsCompleted() const
{
	return m_workQueue.numElem() == 0;
}

// wait for completion
void CEqParallelJobManager::Wait()
{
	for (CEqJobThread* jobThread : m_jobThreads)
		jobThread->WaitForThread();

	CompleteJobCallbacks();
}

// called by job thread
bool CEqParallelJobManager::AssignFreeJob( CEqJobThread* requestBy )
{
	CScopedMutex m(m_mutex);

	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		ParallelJob* job = m_workQueue[i];
		if (!job)
			continue;

		if (requestBy->TryAssignJob(job))
		{
			m_workQueue.fastRemoveIndex(i);
			return true;
		}
	}

	return false;
}

void CEqParallelJobManager::AddCompleted(ParallelJob* job)
{
	CScopedMutex m(m_completeMutex);
	m_completedJobs.append(job);
}

int	CEqParallelJobManager::GetActiveJobThreadsCount()
{
	int cnt = 0;
	for (CEqJobThread* jobThread : m_jobThreads)
		cnt += (jobThread->IsWorkDone() == false) ? 1 : 0;

	return cnt;
}

int	CEqParallelJobManager::GetJobThreadsCount()
{
	return m_jobThreads.numElem();
}

int	CEqParallelJobManager::GetActiveJobsCount(int type /*= -1*/)
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (const ParallelJob* job : m_workQueue)
	{
		if (job->typeId == type && job->threadId != 0)
			cnt++;
	}

	return cnt;
}

int	CEqParallelJobManager::GetPendingJobCount(int type /*= -1*/)
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		const ParallelJob* job = m_workQueue[i];
		if (job->typeId == type && job->threadId == 0)
			cnt++;
	}

	return cnt;
}
