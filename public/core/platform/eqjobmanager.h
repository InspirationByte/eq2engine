 #pragma once
 #include "ds/boundedqueue.h"

 using EQ_JOB_FUNC = EqFunction<void(void*, int i)>;

//--------------------------------------------
// parallel job type

class IParallelJob
{
    friend class CEqJobManager;
	friend class CEqJobThread;
	friend class CEqParallelJobManager;
public:
	virtual ~IParallelJob();
	IParallelJob(const char* jobName)
		: m_jobName(jobName)
	{
	}

	void					InitSignal();
	Threading::CEqSignal*	GetJobSignal() const { return m_jobSignalDone; }

	const char*				GetName() const { return m_jobName; }

	void					AddWait(Threading::CEqSignal* jobWait);
	void					AddWait(IParallelJob* jobWait);

	virtual void			Execute() = 0;

protected:
	enum EPhase : int
	{
		JOB_INIT,
		JOB_STARTED,
		JOB_DONE,
	};

	virtual void			FillJobGroup() {}

	bool					WaitForJobGroup(int timeout = Threading::WAIT_INFINITE);
	void					OnAddedToQueue();

	void					Run();

	EqString						m_jobName;
	Array<Threading::CEqSignal*>	m_waitList{ PP_SL };
	Threading::CEqSignal*			m_jobSignalDone{ nullptr };
	volatile EPhase					m_phase{ JOB_INIT };
	bool							m_deleteJob{ false };
};

//--------------------------------------------
// Dummy Sync Job
class SyncJob : public IParallelJob
{
public:
	SyncJob(const char* jobName)
		: IParallelJob(jobName)
	{}

	void	Execute() override {}
};

//--------------------------------------------
// Function Job
class FunctionParallelJob : public IParallelJob
{
public:
	template<typename F>
	FunctionParallelJob(const char* jobName, F func, void* data, int count)
		: IParallelJob(jobName)
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

//----------------------------------------------------------

class CEqJobManager
{
public:
	class WorkerThread;

	~CEqJobManager();
	CEqJobManager(const char* name, int numThreads, int queueSize);

	void			AddJob(IParallelJob* job);
	
	void			Wait(int waitTimeout = Threading::WAIT_INFINITE);

	bool			AllJobsCompleted() const;
	int				GetJobThreadsCount() const { return m_workerThreads.numElem(); }

	void			Submit();
private:

	bool			TryPopNewJob(WorkerThread& requestBy);

	ArrayRef<WorkerThread>	m_workerThreads{ nullptr };
	mutable BoundedQueue<IParallelJob*>	m_jobQueue;
};