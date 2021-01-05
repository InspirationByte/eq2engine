//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"

//
// The job execution thread
//
class CEqJobThread : public Threading::CEqThread
{
	friend class CEqParallelJobThreads;
public:

	CEqJobThread(CEqParallelJobThreads* owner );

	int							Run();
	bool						AssignJob(eqParallelJob_t* job);

	const eqParallelJob_t*		GetCurrentJob() const;

protected:

	volatile eqParallelJob_t*	m_curJob;
	CEqParallelJobThreads*		m_owner;
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
	bool							Init( int numThreads );
	void							Shutdown();

	// adds the job
	eqParallelJob_t*				AddJob( jobFunction_t func, void* args, int count = 1);	// and puts JOB_FLAG_DELETE flag for this job
	void							AddJob( eqParallelJob_t* job );

	// this submits jobs to the CEqJobThreads
	void							Submit();

	// wait for completion
	void							Wait();

	// returns state if all jobs has been done
	bool							AllJobsCompleted() const;

	// wait for specific job
	void							WaitForJob(eqParallelJob_t* job);

protected:

	// called from worker thread
	bool							AssignFreeJob( CEqJobThread* requestBy );
	void							AddCompleted(eqParallelJob_t* job);
	void							CompleteJobCallbacks();


	DkList<CEqJobThread*>			m_jobThreads;

	DkLinkedList<eqParallelJob_t*>	m_workQueue;
	DkLinkedList<eqParallelJob_t*>	m_completedJobs;

	Threading::CEqMutex				m_mutex;
	uintptr_t						m_mainThreadId;
};

#endif // EQPARALLELJOBS_H
