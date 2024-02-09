//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// Job thread type
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

//--------------------------------------------
// parallel job type

class IParallelJob
{
	friend class CEqJobThread;
	friend class CEqParallelJobManager;
public:
	virtual ~IParallelJob();
	IParallelJob(const char* jobName, EJobType jobType)
		: m_jobName(jobName)
		, m_jobType(jobType)
	{
	}

	void					InitSignal();
	Threading::CEqSignal*	GetJobSignal() const { return m_jobSignalDone; }

	EJobType				GetType() { return m_jobType; }
	const char*				GetName() const { return m_jobName; }

	void					AddWait(Threading::CEqSignal* jobWait);
	void					AddWait(IParallelJob* jobWait);

	virtual void			Execute() = 0;

protected:
	virtual void			FillJobGroup() {}

	bool					WaitForJobGroup(int timeout = Threading::WAIT_INFINITE);
	void					OnAddedToQueue();

	void					Run();

	EqString						m_jobName;
	Threading::CEqSignal*			m_jobSignalDone{ nullptr };
	Array<Threading::CEqSignal*>	m_waitList{ PP_SL };
	EJobType						m_jobType;
};

inline IParallelJob::~IParallelJob()
{
	SAFE_DELETE(m_jobSignalDone);
}

inline void IParallelJob::InitSignal()
{
	if (!m_jobSignalDone)
		m_jobSignalDone = PPNew Threading::CEqSignal(true);

	m_jobSignalDone->Clear(); 
}

inline void IParallelJob::OnAddedToQueue()
{
	if (m_jobSignalDone)
		m_jobSignalDone->Clear();

	FillJobGroup();
}

inline bool IParallelJob::WaitForJobGroup(int timeout)
{
	// wait until all jobs has been done
	for (Threading::CEqSignal* signal : m_waitList)
	{
		if (signal->Wait(timeout))
			return false;
	}

	return true;
}

inline void IParallelJob::Run()
{
	PROF_EVENT(m_jobName);
	Execute();

	if(m_jobSignalDone)
		m_jobSignalDone->Raise();

	m_waitList.clear();
}

inline void IParallelJob::AddWait(Threading::CEqSignal* jobWait)
{
	if (!jobWait)
		return;

	m_waitList.append(jobWait);
}

inline void IParallelJob::AddWait(IParallelJob* jobWait)
{
	jobWait->InitSignal();
	AddWait(jobWait->GetJobSignal());
}

//--------------------------------------------
// Function Job
class FunctionParallelJob : public IParallelJob
{
public:
	template<typename F>
	FunctionParallelJob(const char* jobName, EJobType jobType, F func, void* data, int count)
		: IParallelJob(jobName, jobType)
		, m_jobFunction(std::move(func))
		, m_data(data)
		, m_count(count)
	{
	}

	void Execute()
	{
		for (int i = 0; i < m_count; ++i)
			m_jobFunction(m_data, i);		
	}

	EQ_JOB_FUNC	m_jobFunction;
	void*		m_data{ nullptr };
	int			m_count{ 0 };
};

//--------------------------------------------
// job manager

class IEqParallelJobManager : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_ParallelJobManager_004")

	// creates new job thread
	virtual bool			Init(ArrayCRef<ParallelJobThreadDesc> jobTypes) = 0;
	virtual void			Shutdown() = 0;

	// adds the job to the queue
	virtual void			AddJob(IParallelJob* job) = 0;

	// adds the job to the queue
	virtual void			AddJob(EJobType jobTypeId, EQ_JOB_FUNC jobFn, void* args = nullptr, int count = 1) = 0;	// and puts JOB_FLAG_DELETE flag for this job

	// submits all queued jobs
	virtual void			Submit() = 0;

	virtual void			Wait(int waitTimeout = Threading::WAIT_INFINITE) = 0;

	virtual bool			AllJobsCompleted() const = 0;

	// job thread counter
	virtual int				GetActiveJobThreadsCount() = 0;
	virtual int				GetJobThreadsCount() = 0;
	virtual int				GetActiveJobsCount(EJobType type = JOB_TYPE_ANY) = 0;
	virtual int				GetPendingJobCount(EJobType type = JOB_TYPE_ANY) = 0;
};

INTERFACE_SINGLETON(IEqParallelJobManager, CEqParallelJobManager, g_parallelJobs)
