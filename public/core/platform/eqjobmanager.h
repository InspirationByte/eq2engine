#pragma once
#include "ds/boundedqueue.h"

using EQ_JOB_FUNC = EqFunction<void(void*, int i)>;

//--------------------------------------------
// parallel job type

 class CEqJobManager;

class IParallelJob
{
    friend class CEqJobManager;
public:
	enum EPhase : int
	{
		JOB_INIT,
		JOB_STARTED,
		JOB_DONE,
	};

	virtual ~IParallelJob();
	IParallelJob(const char* jobName)
		: m_jobName(jobName)
	{
	}

	const char*				GetName() const { return m_jobName; }

	void					InitJob();
	void					DeleteOnFinish(bool del = true) { m_deleteJob = del; }

	void					AddWait(IParallelJob* jobToWait);

	void					InitSignal(bool manualReset = true);
	Threading::CEqSignal*	GetSignal() const { return m_doneEvent; }
	EPhase					GetPhase() const { return m_phase; }

	virtual void			Execute() = 0;

protected:
	EqString				m_jobName;
	Array<IParallelJob*>	m_nextJobs{ PP_SL };
	Threading::CEqSignal*	m_doneEvent{ nullptr };
	Threading::CEqMutex		m_deleteMutex;

	CEqJobManager*			m_jobMng{ nullptr };

	volatile EPhase			m_phase{ JOB_INIT };
	volatile int			m_primeJobs{ 1 };

	bool					m_deleteJob{ false };
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
class FunctionJob : public IParallelJob
{
public:
	template<typename F>
	FunctionJob(const char* jobName, F func, void* data = nullptr, int count = 1)
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
	CEqJobManager(const char* name, int numThreads, int queueSize, int stackSize = Threading::DEFAULT_THREAD_STACK_SIZE);

	void			InitStartJob(IParallelJob* job);
	void			StartJob(IParallelJob* job, bool submit = true);
	
	void			Wait(int waitTimeout = Threading::WAIT_INFINITE);

	bool			AllJobsCompleted() const;
	int				GetJobThreadsCount() const { return m_workerThreads.numElem(); }

	bool			Submit(int numWorkers);
private:

	void			DoStartJob(IParallelJob* job);
	void			ExecuteJob(IParallelJob& job);

	IParallelJob*	ExtractJobFromQueue();

	using JobQueue = BoundedQueue<IParallelJob*>;

	ArrayRef<WorkerThread>	m_workerThreads{ nullptr };
	mutable JobQueue		m_jobQueue;
	int						m_queueSize{ 0 };
	volatile int			m_jobAvailability{ 0 };
};