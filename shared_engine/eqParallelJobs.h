//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

/*

This is the diferrent conception of jobs
	Each thread processes single job it got. Each job is assigned to thread in order

	g_parallelJobList->Submit() ----
									\
									 \
							for( each job in jobs )
								m_threads[threadId++]->AddJob( job )

							// each thread is signalled for work
							for( each thread in m_threads )
								thread->SignalWork()

*/

#ifndef EQPARALLELJOBS_H
#define EQPARALLELJOBS_H

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"

namespace Threading
{
	typedef void(*jobFunction_t)(void *);

	enum EJobFlags
	{
		JOB_FLAG_ALLOCATED	= (1 << 0),
		JOB_FLAG_CURRENT	= (1 << 1),
		JOB_FLAG_EXECUTED	= (1 << 2),
	};

	const uintptr_t JOB_THREAD_ANY = 0;

	struct eqParallelJob_t
	{
		eqParallelJob_t() : flags(0), arguments(nullptr), threadId(0)
		{
		}

		jobFunction_t	func;
		void*			arguments;
		volatile int	flags;		// EJobFlags
		uintptr_t		threadId;	// выбор потока
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
		void							AddJob( jobFunction_t func, void* args );	// and puts JOB_FLAG_ALLOCATED flag for this job
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
