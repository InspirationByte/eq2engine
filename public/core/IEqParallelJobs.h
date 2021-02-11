#ifndef IEQPARALLELJOBS_H
#define IEQPARALLELJOBS_H

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "core/InterfaceManager.h"

#define PARALLELJOBS_INTERFACE_VERSION		"CORE_ParallelJobs_001"

typedef void(*jobFunction_t)(void*, int i);
typedef void(*jobComplete_t)(struct eqParallelJob_t*);

enum EJobFlags
{
	JOB_FLAG_DELETE = (1 << 0),		// job has to be deleted after executing. If not set, please specify 'onComplete' function
	JOB_FLAG_CURRENT = (1 << 1),		// it's current job
	JOB_FLAG_EXECUTED = (1 << 2),		// execution is completed
};

const uintptr_t JOB_THREAD_ANY = 0;

struct eqParallelJob_t
{
	eqParallelJob_t() : flags(0), arguments(nullptr), threadId(0), numIter(1), onComplete(nullptr)
	{
	}

	jobFunction_t	func;				// job function. This is a parallel work
	jobComplete_t	onComplete;			// job completion callback after all numIter is complete. Always executed before Submit() called on job manager
	void* arguments;
	volatile int	flags;		// EJobFlags
	uintptr_t		threadId;	// выбор потока
	int				numIter;
};

// job arranger thread
abstract_class IEqParallelJobThreads : public ICoreModuleInterface
{
public:
	// creates new job thread
	virtual bool							Init(int numThreads) = 0;
	virtual void							Shutdown() = 0;

	// adds the job
	virtual eqParallelJob_t*				AddJob(jobFunction_t func, void* args, int count = 1) = 0;	// and puts JOB_FLAG_DELETE flag for this job
	virtual void							AddJob(eqParallelJob_t* job) = 0;

	// this submits jobs to the CEqJobThreads
	virtual void							Submit() = 0;

	// wait for completion
	virtual void							Wait() = 0;

	// returns state if all jobs has been done
	virtual bool							AllJobsCompleted() const = 0;

	// wait for specific job
	virtual void							WaitForJob(eqParallelJob_t* job) = 0;
};

INTERFACE_SINGLETON(IEqParallelJobThreads, CEqParallelJobThreads, PARALLELJOBS_INTERFACE_VERSION, g_parallelJobs)

#endif // IEQPARALLELJOBS_H