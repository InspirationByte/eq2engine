//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EJobType : int
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
	JOB_TYPE_SPOOL_NAV,
	JOB_TYPE_OBJECTS,

	JOB_TYPE_COUNT,
};

struct ParallelJobThreadDesc
{
	EJobType	jobTypeId;
	int			numThreads;
};

using EQ_JOB_FUNC = EqFunction<void(void*, int i)>;
using EQ_JOB_COMPLETE_FUNC = EqFunction<void(void*, int i)>;

#if 0
class IParallelJob
{
public:
	virtual ~IParallelJob();

	virtual void			Execute() = 0;
	virtual void			FillJobGroup() = 0;

	void					InitSignal();
	Threading::CEqSignal*	GetJobSignal() const { return m_jobSignalDone; }

protected:
	void					Run();
	void					AddWait(IParallelJob* jobWait);

	EqString				m_jobName;
	Threading::CEqSignal*	m_jobSignalDone{ nullptr };
	Array<IParallelJob*>	m_waitJobs{ PP_SL };
};

IParallelJob::~IParallelJob()
{
	SAFE_DELETE(m_jobSignalDone);
}

void IParallelJob::InitSignal()
{
	if (!m_jobSignalDone)
		m_jobSignalDone = PPNew Threading::CEqSignal(true);

	m_jobSignalDone->Clear(); 
}

void IParallelJob::Run()
{
	Threading::CEqSignal* jobSignalDone = m_jobSignalDone;
	if(jobSignalDone)
		jobSignalDone->Clear();

	m_waitJobs.clear();

	FillJobGroup();

	for (IParallelJob* waitJob : m_waitJobs)
		waitJob->GetJobSignal()->Wait();

	{
		PROF_EVENT(m_jobName);
		Execute();
	}

	if(jobSignalDone)
		jobSignalDone->Raise();
}

void IParallelJob::AddWait(IParallelJob* jobWait)
{
	jobWait->InitSignal();
	m_waitJobs.append(jobWait);
}
#endif // 0

//--------------------------------------------
// job manager


class IEqParallelJobManager : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_ParallelJobs_002")

	// creates new job thread
	virtual bool			Init(ArrayCRef<ParallelJobThreadDesc> jobTypes) = 0;
	virtual void			Shutdown() = 0;

	// adds the job
	virtual void			AddJob(EJobType jobTypeId, EQ_JOB_FUNC jobFn, void* args = nullptr, int count = 1, EQ_JOB_COMPLETE_FUNC completeFn = nullptr) = 0;	// and puts JOB_FLAG_DELETE flag for this job

	// this submits jobs to the CEqJobThreads
	virtual void			Submit() = 0;

	// wait for completion
	virtual void			Wait() = 0;

	// returns state if all jobs has been done
	virtual bool			AllJobsCompleted() const = 0;

	// manually invokes job callbacks on completed jobs
	// should be called in main loop thread or in critical places
	virtual void			CompleteJobCallbacks() = 0;

	// job thread counter
	virtual int				GetActiveJobThreadsCount() = 0;
	virtual int				GetJobThreadsCount() = 0;
	virtual int				GetActiveJobsCount(int type = -1) = 0;
	virtual int				GetPendingJobCount(int type = -1) = 0;
};

INTERFACE_SINGLETON(IEqParallelJobManager, CEqParallelJobManager, g_parallelJobs)
