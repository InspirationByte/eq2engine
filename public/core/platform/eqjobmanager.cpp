#include "core/core_common.h"
#include "eqjobmanager.h"

using namespace Threading;

static constexpr const int MAX_JOB_MANAGER_THREADS = 32;

IParallelJob::~IParallelJob()
{
	CScopedMutex m(m_deleteMutex);

	if (m_phase != JOB_DONE)
	{
		m_phase = JOB_DONE;
		for (IParallelJob* nextJobs : m_nextJobs)
			Atomic::Decrement(nextJobs->m_primeJobs);
		m_nextJobs.clear();
	}

	SAFE_DELETE(m_doneEvent);
}

void IParallelJob::InitJob()
{
	ASSERT(m_phase != JOB_STARTED);

	if (m_doneEvent)
		m_doneEvent->Clear();

	if (m_phase == JOB_INIT)
		return;

	m_nextJobs.clear();

	m_phase = JOB_INIT;
	m_primeJobs = 1;
}

void IParallelJob::InitSignal(bool manualReset)
{
	if (!m_doneEvent)
	{
		m_doneEvent = PPNew Threading::CEqSignal(manualReset);
		m_doneEvent->Raise();
	}
}

void IParallelJob::AddWait(IParallelJob* jobToWait)
{
	ASSERT(jobToWait != this);
	ASSERT(m_phase == JOB_INIT || (m_phase == JOB_STARTED && m_primeJobs > 0));

	CScopedMutex m(jobToWait->m_deleteMutex);
	if (jobToWait->m_phase != JOB_DONE)
	{
		Atomic::Increment(m_primeJobs);
		jobToWait->m_nextJobs.append(this);
	}
}

//---------------------------------------------------------- 

class CEqJobManager;

class CEqJobManager::WorkerThread : public CEqThread
{
	friend class CEqJobManager;
public:
	WorkerThread(CEqJobManager& jobManager);

	int						Run();
protected:
	CEqJobManager&      	m_jobManager;
};

CEqJobManager::WorkerThread::WorkerThread(CEqJobManager& jobMng) 
	: m_jobManager(jobMng)
{
}

int CEqJobManager::WorkerThread::Run()
{
	// thread will find job by himself
	IParallelJob* job = m_jobManager.ExtractJobFromQueue();
	if (!job)
		return 0;

	m_jobManager.ExecuteJob(*job);

	if(m_jobManager.m_jobAvailability)
		SignalWork();

	return 0;
}

//---------------------------------------------------------- 

CEqJobManager::~CEqJobManager()
{
	for (WorkerThread& thread : m_workerThreads)
		thread.~WorkerThread();

	PPFree(m_workerThreads.ptr());

	// cleanup queue
	IParallelJob* job = nullptr;
	do
	{
		m_jobQueue.dequeue(job);
		if(job && job->m_deleteJob)
			delete job;
	} while (job);
}

CEqJobManager::CEqJobManager(const char* name, int numThreads, int queueSize, int stackSize)
	: m_jobQueue(queueSize)
{
	numThreads = min(numThreads, MAX_JOB_MANAGER_THREADS);
	
	m_queueSize = queueSize;
	m_workerThreads = PPAllocStructArrayRef(WorkerThread, numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		new (&m_workerThreads[i]) WorkerThread(*this);
		m_workerThreads[i].StartWorkerThread(EqString::Format("%s_%d", name, i), TP_NORMAL, stackSize);
	}
}

void CEqJobManager::InitStartJob(IParallelJob* job)
{
	job->InitJob();
	StartJob(job);
}

void CEqJobManager::StartJob(IParallelJob* job, bool submit)
{
	ASSERT(job->m_phase == IParallelJob::JOB_INIT);
	ASSERT(job->m_primeJobs > 0);
	
	job->m_jobMng = this;
	job->m_phase = IParallelJob::JOB_STARTED;

	const bool canBeStarted = Atomic::Decrement(job->m_primeJobs) == 0;
	if (canBeStarted)
	{
		DoStartJob(job);

		if(submit)
			Submit(1);
	}
}

void CEqJobManager::DoStartJob(IParallelJob* job)
{
	ASSERT(job->m_primeJobs <= 0);

	const bool queued = m_jobQueue.enqueue(job);
	ASSERT_MSG(queued, "Jobs queue is too small (%d), please increase", m_queueSize);
}

void CEqJobManager::ExecuteJob(IParallelJob& job)
{
	ASSERT(job.m_primeJobs <= 0);
	ASSERT(job.m_phase == IParallelJob::JOB_STARTED);
	ASSERT(job.m_jobMng == this);

	// execute
	{
		PROF_EVENT(job.m_jobName);
		job.Execute();
	}

	job.m_deleteMutex.Lock();
	job.m_phase = IParallelJob::JOB_DONE;

	IParallelJob** unblockedJobs = reinterpret_cast<IParallelJob**>(stackalloc(m_queueSize * sizeof(IParallelJob*)));
	int numUnblocked = 0;
	for (int i = 0; i < job.m_nextJobs.numElem(); ++i)
	{
		const bool canBeStarted = Atomic::Decrement(job.m_nextJobs[i]->m_primeJobs) == 0;
		if (canBeStarted)
			unblockedJobs[numUnblocked++] = job.m_nextJobs[i];
	}

	if (job.m_doneEvent)
		job.m_doneEvent->Raise();

	job.m_deleteMutex.Unlock();

	struct JobBatch
	{
		CEqJobManager* mng;
		int count;
	};
	JobBatch* batchs = nullptr;
	int numBatchs = 0;

	if(numUnblocked)
		batchs = reinterpret_cast<JobBatch*>(stackalloc(numUnblocked * sizeof(JobBatch)));

	for (int i = 0; i < numUnblocked; ++i)
	{
		IParallelJob* unblockedJob = unblockedJobs[i];
		CEqJobManager* unblockedMng = unblockedJob->m_jobMng;
		unblockedMng->DoStartJob(unblockedJob);

		int j = 0;
		while(j < numBatchs)
		{
			if(batchs[j].mng == unblockedMng)
			{
				++batchs[j].count;
				break;
			}
		}
		if(j == numBatchs)
		{
			batchs[numBatchs].count = 1;
			batchs[numBatchs].mng = unblockedMng;
			++numBatchs;
		}
	}

	//if(numBatchs == 1 && batchs[0].count == 1 && batchs[0].mng == this)
	//{
	//	// don't bother, we'll check job queue anyway
	//}
	//else if(numBatchs)
	{
		for(int i = 0; i < numBatchs; ++i)
			batchs[i].mng->Submit(batchs[i].count);
	}

	if (job.m_deleteJob)
		delete& job;
}

bool CEqJobManager::Submit(int numWorkers)
{
	Atomic::Add(m_jobAvailability, numWorkers);

	int count = numWorkers;
	for (WorkerThread& thread : m_workerThreads)
	{
		if(count == 0)
			return true;
		
		if(!thread.IsWorkDone())
			continue;

		thread.SignalWork();
		--count;
	}

	return false;
}

bool CEqJobManager::AllJobsCompleted() const
{
	for (WorkerThread& thread : m_workerThreads)
	{
		if(!thread.WaitForThread(0))
			return false;
	}

	IParallelJob* job = nullptr;
	if (!m_jobQueue.dequeue(job))
		return true;

	m_jobQueue.enqueue(job);
	return false;
}

void CEqJobManager::Wait(int waitTimeout)
{
	for (WorkerThread& thread : m_workerThreads)
		thread.WaitForThread(waitTimeout);
}

IParallelJob* CEqJobManager::ExtractJobFromQueue()
{
	IParallelJob* job = nullptr;
	if (!m_jobQueue.dequeue(job))
		return nullptr;

	Atomic::Decrement(m_jobAvailability);

	return job;
}