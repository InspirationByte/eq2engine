//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

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

#ifndef EQPARALLELJOBS_H
#define EQPARALLELJOBS_H

#include "core/IEqParallelJobs.h"
#include "utils/eqthread.h"
#include "ds/DkList.h"
#include "ds/DkLinkedList.h"

class CEqParallelJobThreads;

//
// The job execution thread
//
class CEqJobThread : public Threading::CEqThread
{
	friend class CEqParallelJobThreads;
public:

	CEqJobThread(CEqParallelJobThreads* owner, int threadJobTypeId );

	int							Run();
	bool						AssignJob(eqParallelJob_t* job);

	const eqParallelJob_t*		GetCurrentJob() const;

protected:

	volatile eqParallelJob_t*	m_curJob;
	CEqParallelJobThreads*		m_owner;
	int							m_threadJobTypeId;
};

//
// Job thread list
//
class CEqParallelJobThreads : public IEqParallelJobThreads
{
	friend class CEqJobThread;
public:
	CEqParallelJobThreads();
	virtual ~CEqParallelJobThreads();

	bool							IsInitialized() const { return m_jobThreads.numElem() > 0; }
	const char*						GetInterfaceName() const { return PARALLELJOBS_INTERFACE_VERSION; }

	// creates new job thread
	bool							Init(int numJobTypes, eqJobThreadDesc_t* jobTypes);
	void							Shutdown();

	// adds the job
	eqParallelJob_t*				AddJob( int jobTypeId, jobFunction_t func, void* args, int count = 1, jobComplete_t completeFn = nullptr);	// and puts JOB_FLAG_DELETE flag for this job
	void							AddJob( eqParallelJob_t* job );

	// this submits jobs to the CEqJobThreads
	void							Submit();

	// wait for completion
	void							Wait();

	// returns state if all jobs has been done
	bool							AllJobsCompleted() const;

	// wait for specific job
	void							WaitForJob(eqParallelJob_t* job);

	// manually invokes job callbacks on completed jobs
	void							CompleteJobCallbacks();

protected:

	// called from worker thread
	bool							AssignFreeJob( CEqJobThread* requestBy );
	void							AddCompleted(eqParallelJob_t* job);


	DkList<CEqJobThread*>			m_jobThreads;

	DkLinkedList<eqParallelJob_t*>	m_workQueue;
	DkLinkedList<eqParallelJob_t*>	m_completedJobs;

	Threading::CEqMutex				m_mutex;
	Threading::CEqMutex				m_completeMutex;
	uintptr_t						m_mainThreadId;
};

#endif // EQPARALLELJOBS_H
