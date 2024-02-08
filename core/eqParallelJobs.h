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
	ParallelJob(EJobType jobTypeId, EQ_JOB_FUNC fn, void* args = nullptr, int count = 1, EQ_JOB_COMPLETE_FUNC completeFn = nullptr)
		: typeId(jobTypeId)
		, func(std::move(fn))
		, arguments(args)
		, numIter(count)
		, onComplete(std::move(completeFn))
	{
	}

	EQ_JOB_FUNC				func;					// job function. This is a parallel work
	EQ_JOB_COMPLETE_FUNC	onComplete;				// job completion callback after all numIter is complete. Always executed before Submit() called on job manager
	uintptr_t				threadId{ 0 };			// selected thread
	void*					arguments{ nullptr };	// job argument object passed to job function
	int						numIterOrig{ 1 };
	volatile int			flags{ 0 };				// EJobFlags
	int						numIter{ 1 };
	EJobType				typeId{ JOB_TYPE_ANY };	// the job type that specific thread will take
};

//
// The job execution thread
//
class CEqJobThread : public Threading::CEqThread
{
	friend class CEqParallelJobManager;
public:
	CEqJobThread(CEqParallelJobManager* owner, int threadJobTypeId );

	int						Run();
	bool					TryAssignJob(ParallelJob* job);

	const ParallelJob*		GetCurrentJob() const;

protected:

	volatile ParallelJob*	m_curJob;
	CEqParallelJobManager*	m_owner;
	int						m_threadJobTypeId;
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
	bool			Init(ArrayCRef<ParallelJobThreadDesc> jobTypes);
	void			Shutdown();

	// adds the job
	void			AddJob(EJobType jobTypeId, EQ_JOB_FUNC func, void* args = nullptr, int count = 1, EQ_JOB_COMPLETE_FUNC completeFn = nullptr);	// and puts JOB_FLAG_DELETE flag for this job
	void			AddJob(ParallelJob* job);

	// this submits jobs to the CEqJobThreads
	void			Submit();

	// wait for completion
	void			Wait();

	// returns state if all jobs has been done
	bool			AllJobsCompleted() const;

	// manually invokes job callbacks on completed jobs
	void			CompleteJobCallbacks();

	int				GetActiveJobThreadsCount();
	int				GetJobThreadsCount();

	int				GetActiveJobsCount(int type = -1);
	int				GetPendingJobCount(int type = -1);

protected:

	// called from worker thread
	bool					AssignFreeJob( CEqJobThread* requestBy );
	void					AddCompleted(ParallelJob* job);

	Array<CEqJobThread*>	m_jobThreads{ PP_SL };

	Array<ParallelJob*>		m_workQueue{ PP_SL };
	Array<ParallelJob*>		m_completedJobs{ PP_SL };

	Threading::CEqMutex		m_mutex;
	Threading::CEqMutex		m_completeMutex;
	uintptr_t				m_mainThreadId;
};