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

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"

namespace Threading
{
	typedef void(*jobFunction_t)(void *, int i);
	typedef void(*jobComplete_t)(struct eqParallelJob_t *);

	enum EJobFlags
	{
		JOB_FLAG_DELETE		= (1 << 0),		// job has to be deleted after executing. If not set, please specify 'onComplete' function
		JOB_FLAG_CURRENT	= (1 << 1),		// it's current job
		JOB_FLAG_EXECUTED	= (1 << 2),		// execution is completed
	};

	const uintptr_t JOB_THREAD_ANY = 0;

	struct eqParallelJob_t
	{
		eqParallelJob_t() : flags(0), arguments(nullptr), threadId(0), numIter(1), onComplete(nullptr)
		{
		}

		jobFunction_t	func;
		jobComplete_t	onComplete;
		void*			arguments;
		volatile int	flags;		// EJobFlags
		uintptr_t		threadId;	// выбор потока
		int				numIter;
	};

	class CEqParallelJobThreads;

	//
	// The job execution thread
	//
	class CEqJobThread : public CEqThread
	{
		friend class CEqParallelJobThreads;
	public:

		CEqJobThread( CEqParallelJobThreads* owner );

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
	class CEqParallelJobThreads // TODO: interface
	{
		friend class CEqJobThread;
	public:
		CEqParallelJobThreads();
		virtual ~CEqParallelJobThreads();

		// creates new job thread
		bool							Init( int numThreads );
		void							Shutdown();

		CEqJobThread*					GetThreadById( uintptr_t threadId ) const;
		void							GetThreadIds( DkList<uintptr_t>& list ) const;

		// adds the job
		void							AddJob( jobFunction_t func, void* args );	// and puts JOB_FLAG_DELETE flag for this job
		void							AddJob( eqParallelJob_t* job );

		// this submits jobs to the CEqJobThreads
		void							Submit();

		// wait for completion
		void							Wait();

		// wait for specific job
		void							WaitForJob(eqParallelJob_t* job);

	protected:

		// called from worker thread
		bool							AssignFreeJob( CEqJobThread* requestBy );

		CEqMutex&						m_mutex;
		DkList<CEqJobThread*>			m_jobThreads;

		DkLinkedList<eqParallelJob_t*>	m_workQueue;
	};
}

extern Threading::CEqParallelJobThreads* g_parallelJobs;

#endif // EQPARALLELJOBS_H
