//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IEqParallelJobs.h"

/*

This is the diferrent conception of jobs
	Each thread processes single job it got. Each job is assigned to thread in order

	g_parallelJobList->Submit() ----
									\
									 \
							// each thread is signalled for work
							for( each thread in m_threads )
								thread->SignalWork()

	Threads are searching for their jobs by calling CEqParallelJobThreads::AssignFreeJob

*/

class CEqParallelJobManager;

enum EJobFlags : int
{
	JOB_FLAG_DELETE = (1 << 0),		// job has to be deleted after executing. If not set, please specify 'onComplete' function
	JOB_FLAG_CURRENT = (1 << 1),	// it's current job
	JOB_FLAG_EXECUTED = (1 << 2),	// execution is completed
};

struct ParallelJob
{
	ParallelJob() = default;
	ParallelJob(IParallelJob* job)
		: job(job)
	{
	}

	IParallelJob*	job{ nullptr };

	volatile uintptr_t	threadId{ 0 };			// selected thread
	volatile int		flags{ 0 };				// EJobFlags
};

//
// The job execution thread
//
class CEqJobThread : public Threading::CEqThread
{
	friend class CEqParallelJobManager;
public:
	CEqJobThread(CEqParallelJobManager* owner);

	int						Run();
	bool					TryAssignJob(ParallelJob* job);

	const ParallelJob*		GetCurrentJob() const;

protected:

	volatile ParallelJob*	m_curJob;
	CEqParallelJobManager*	m_owner;
};

//
// Job thread list
//
class CEqParallelJobManager : public IEqParallelJobManager
{
	friend class CEqJobThread;
public:
	CEqParallelJobManager();
	virtual ~CEqParallelJobManager();

	bool			IsInitialized() const { return m_jobThreads.numElem() > 0; }

	// creates new job thread
	bool			Init();
	void			Shutdown();

	// adds the job
	void			AddJob(IParallelJob* job);
	void			AddJob(EQ_JOB_FUNC func, void* args = nullptr, int count = 1);	// and puts JOB_FLAG_DELETE flag for this job

	// this submits jobs to the CEqJobThreads
	void			Submit();

	void			Wait(int waitTimeout = Threading::WAIT_INFINITE);

	bool			AllJobsCompleted() const;

	int				GetActiveJobThreadsCount();
	int				GetJobThreadsCount();

	int				GetActiveJobsCount();
	int				GetPendingJobCount();

protected:

	// called from worker thread
	bool					TryPopNewJob( CEqJobThread* requestBy );

	Array<CEqJobThread*>	m_jobThreads{ PP_SL };
	Array<ParallelJob*>		m_workQueue{ PP_SL };

	Threading::CEqMutex		m_mutex;
	Threading::CEqMutex		m_completeMutex;
	uintptr_t				m_mainThreadId;
};