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

//--------------------------------------------
// Batched Job

class CEqJobManager;

template<typename ITEM>
class BatchedJob : public SyncJob
{
public:
	using BatchItemList = Array<ITEM>;
	using BatchItems = ArrayRef<ITEM>;
	using BatchItemSpan = ArrayRef<ITEM>;
	
	class Worker : public IParallelJob
	{
		friend class BatchedJob;
	public:
		Worker(const char* name, BatchedJob& ownerJob) : IParallelJob(name), m_owner(ownerJob) {}
	private:
		void 			Execute() override;

		BatchedJob&		m_owner;
		BatchItemSpan 	m_batchItems{ nullptr };
		int				m_threadCount{ 0 };
		int				m_firstTask{ 0 };
	};

	BatchedJob(const char* name);
	void StartJobs(CEqJobManager& jobMng);

private:
	virtual BatchItems GetJobItems() = 0;
	virtual void OnInitWorker(Worker& workerJob) {};
	virtual void Process(ITEM jobItem) = 0;

	Array<Worker>	m_workerJobs{ PP_SL };
	EqString		m_batchJobName;
};

//----------------------------------------------------------

// Job manager 
// Provides job queue with worker threads
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


// TODO: hpp

template<typename ITEM>
BatchedJob<ITEM>::BatchedJob(const char* name)
	: SyncJob(name)
	, m_batchJobName(m_jobName + "Worker")
{ 
	InitSignal();
}

template<typename ITEM>
void BatchedJob<ITEM>::StartJobs(CEqJobManager& jobMng)
{
	BatchItems batchJobItems = GetJobItems();
	if (!batchJobItems.numElem())
	{
		jobMng.StartJob(this);
		return;
	}

	const int tasksPerBatch = batchJobItems.numElem() / jobMng.GetJobThreadsCount();
	m_workerJobs.assureSizeEmplace(jobMng.GetJobThreadsCount(), m_batchJobName, *this);

	int numBatchs = 0;
	for (Worker& workerJob : m_workerJobs)
	{
		workerJob.m_batchItems = ArrayRef(batchJobItems.ptr(), batchJobItems.numElem());
		workerJob.m_firstTask = numBatchs++;
		workerJob.m_threadCount = jobMng.GetJobThreadsCount();
		workerJob.InitJob();
		AddWait(&workerJob);
		OnInitWorker(workerJob);
		jobMng.StartJob(&workerJob, false);
	}

	jobMng.StartJob(this, false);
	jobMng.Submit(m_workerJobs.numElem() + 1);
}

template<typename ITEM>
void BatchedJob<ITEM>::Worker::Execute()
{
	for (int i = m_firstTask; i < m_batchItems.numElem(); i += m_threadCount)
		m_owner.Process(m_batchItems[i]);
}