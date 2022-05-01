#ifndef IEQPARALLELJOBS_H
#define IEQPARALLELJOBS_H

#include "utils/eqthread.h"
#include "ds/Array.h"
#include "ds/function.h"
#include "core/InterfaceManager.h"

#define PARALLELJOBS_INTERFACE_VERSION		"CORE_ParallelJobs_002"

//typedef void(*jobFunction_t)(void*, int i);
//typedef void(*jobComplete_t)(struct eqParallelJob_t*);

using EQ_JOB_FUNC = EqFunction<void(void*, int i)>;
using EQ_JOB_COMPLETE_FUNC = EqFunction<void(struct eqParallelJob_t*)>;

enum EJobTypes
{
	JOB_TYPE_ANY = -1,

	JOB_TYPE_AUDIO,
	JOB_TYPE_PHYSICS,
	JOB_TYPE_RENDERER,
	JOB_TYPE_PARTICLES,
	JOB_TYPE_DECALS,
	JOB_TYPE_SPOOL_AUDIO,
	JOB_TYPE_SPOOL_EGF,
	JOB_TYPE_SPOOL_WORLD,
	JOB_TYPE_OBJECTS,

	JOB_TYPE_COUNT,
};

enum EJobFlags
{
	JOB_FLAG_DELETE = (1 << 0),			// job has to be deleted after executing. If not set, please specify 'onComplete' function
	JOB_FLAG_CURRENT = (1 << 1),		// it's current job
	JOB_FLAG_EXECUTED = (1 << 2),		// execution is completed
};

const uintptr_t JOB_THREAD_ANY = 0;

struct eqParallelJob_t
{
	eqParallelJob_t() 
		: flags(0), typeId(-1), func(nullptr), arguments(nullptr), numIter(1), threadId(0), onComplete(nullptr)
	{}

	eqParallelJob_t(int jobTypeId, EQ_JOB_FUNC fn, void* args = nullptr, int count = 1, EQ_JOB_COMPLETE_FUNC completeFn = nullptr)
		: flags(0), typeId(jobTypeId), func(fn), arguments(args), numIter(count), threadId(0), onComplete(completeFn)
	{
	}

	EQ_JOB_FUNC				func;				// job function. This is a parallel work
	EQ_JOB_COMPLETE_FUNC	onComplete;			// job completion callback after all numIter is complete. Always executed before Submit() called on job manager
	void*					arguments;			// job argument object passed to job function
	volatile int			flags;				// EJobFlags
	uintptr_t				threadId;			// selected thread
	int						numIter;
	int						typeId;				// the job type that specific thread will take
};

// structure for initialization
struct eqJobThreadDesc_t
{
	int jobTypeId;
	int numThreads;
};

// job arranger thread
abstract_class IEqParallelJobThreads : public IEqCoreModule
{
public:
	// creates new job thread
	virtual bool							Init(int numJobTypes, eqJobThreadDesc_t* jobTypes) = 0;
	virtual void							Shutdown() = 0;

	// adds the job
	virtual eqParallelJob_t*				AddJob(int jobTypeId, EQ_JOB_FUNC jobFn, void* args = nullptr, int count = 1, EQ_JOB_COMPLETE_FUNC completeFn = nullptr) = 0;	// and puts JOB_FLAG_DELETE flag for this job
	virtual void							AddJob(eqParallelJob_t* job) = 0;

	// this submits jobs to the CEqJobThreads
	virtual void							Submit() = 0;

	// wait for completion
	virtual void							Wait() = 0;

	// returns state if all jobs has been done
	virtual bool							AllJobsCompleted() const = 0;

	// wait for specific job
	virtual void							WaitForJob(eqParallelJob_t* job) = 0;

	// manually invokes job callbacks on completed jobs
	// should be called in main loop thread or in critical places
	virtual void							CompleteJobCallbacks() = 0;

	// job thread counter
	virtual int								GetActiveJobThreadsCount() = 0;
	virtual int								GetJobThreadsCount() = 0;
	virtual int								GetActiveJobsCount(int type = -1) = 0;
	virtual int								GetPendingJobCount(int type = -1) = 0;
};

INTERFACE_SINGLETON(IEqParallelJobThreads, CEqParallelJobThreads, PARALLELJOBS_INTERFACE_VERSION, g_parallelJobs)

#endif // IEQPARALLELJOBS_H