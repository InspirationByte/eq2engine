# Job manager — `framework/core/platform/eqjobmanager.h`

The job system runs short tasks on a pool of worker threads, with dependencies between jobs. It is
not included by `core/core_common.h`:

Always include the public interface header, not `eqjobmanager.h` directly:

```cpp
#include "core/IEqParallelJobs.h"
```

Implementation: `framework/core/platform/eqjobmanager.cpp`.

The engine-wide instance is available through `g_parallelJobs` (`public/core/IEqParallelJobs.h`,
implemented in `e2Core`). Create a separate `CEqJobManager` only for a dedicated pool.

## Quick use — `g_parallelJobs`

```cpp
// function job: fn(data, index) is called `count` times on one worker
g_parallelJobs->AddJob([](void* data, int i) {
	ProcessChunk(static_cast<Chunk*>(data), i);
}, chunks, numChunks);

// own job object
g_parallelJobs->AddJob(&m_buildJob);

g_parallelJobs->Wait();			// wait until all jobs completed
```

`IEqParallelJobManager`: `AddJob(job)`, `AddJob(fn, data, count)` (the created job is deleted
when finished), `Wait(timeout)`, `AllJobsCompleted()`, `GetJobThreadsCount()`, `GetJobMng()`
(underlying `CEqJobManager`).

## Jobs — `IParallelJob`

Derive from `IParallelJob` and implement `Execute()`.

```cpp
class CBuildNavJob : public IParallelJob
{
public:
	CBuildNavJob() : IParallelJob("BuildNav") {}
	void Execute() override { ... }
};
```

Lifecycle (`EPhase`): `JOB_INIT` → `JOB_STARTED` (submitted, may be waiting for dependencies) →
`JOB_DONE`.

- `InitJob()` — resets a finished job so it can be started again; clears its dependency links.
  A job object may be reused only after it is done.
- `AddWait(otherJob)` — this job starts only after `otherJob` is done. Call it before starting
  this job; a dependency that is already done is ignored.
- `InitSignal(manualReset = true)` — creates a done-signal (`GetSignal()`), raised when the job is
  done and cleared by `InitJob()`. Use it to wait for one specific job:
  `job.GetSignal()->Wait()`.
- `DeleteOnFinish(true)` — the manager deletes the job object after execution (for jobs created
  with `PPNew`).
- `GetName()` — the name is used as the profiler zone name (`PROF_EVENT`) during execution.

Built-in job types:

| Type | Purpose |
|---|---|
| `FunctionJob(name, fn, data, count)` | Calls `fn(data, i)` for `i` in `[0, count)` sequentially in one job |
| `SyncJob(name)` | Empty job — a synchronization point for several dependencies |
| `BatchedJob<ITEM>` | Splits a list of items across all worker threads (see below) |

## Dependencies

```cpp
// A and B run in parallel, C runs after both
jobC.AddWait(&jobA);
jobC.AddWait(&jobB);

jobMng.StartJob(&jobC, false);	// C is registered but blocked
jobMng.StartJob(&jobA, false);
jobMng.StartJob(&jobB, false);
jobMng.Submit(3);				// wake workers

jobC.GetSignal()->Wait();		// requires jobC.InitSignal()
```

Jobs whose dependencies are done are queued immediately; blocked jobs are queued by the worker that
finishes their last dependency.

## CEqJobManager

```cpp
CEqJobManager jobMng("MyPool", numThreads, queueSize);
```

- `StartJob(job, submit = true)` — starts a job in `JOB_INIT` phase. With `submit = false` the job
  is only queued; call `Submit(numWorkers)` once after starting a group of jobs to wake workers.
- `InitStartJob(job)` — `InitJob()` + `StartJob()` (for reused job objects).
- `Wait(timeout)` — waits until every worker thread has run out of work (the timeout applies to
  each worker in turn).
- `AllJobsCompleted()`, `GetJobThreadsCount()`.
- `queueSize` limits the number of jobs queued at the same time; overflowing it asserts.

## BatchedJob

`BatchedJob<ITEM>` processes a list of items on all job threads: worker `k` processes items
`k, k + N, k + 2N, ...` (N — thread count). The batched job itself completes after all workers.

```cpp
class CCullObjectsJob : public BatchedJob<CRenderObject*>
{
public:
	CCullObjectsJob() : BatchedJob("CullObjects") {}

private:
	BatchItems GetJobItems() override { return m_objects; }
	void Process(CRenderObject* obj) override { obj->Cull(m_frustum); }

	Array<CRenderObject*>	m_objects{ PP_SL };
	Volume					m_frustum;
};

m_cullJob.InitJob();
m_cullJob.StartJobs(*g_parallelJobs->GetJobMng());
m_cullJob.GetSignal()->Wait();
```

- `GetJobItems()` returns the items as `ArrayRef<ITEM>`; `Process(item)` is called for each item
  on worker threads, so it must be thread-safe.
- `OnInitWorker(worker)` — optional hook called for each worker before it starts.
- The signal is created by the `BatchedJob` constructor.

## Guidelines

- Jobs must not block waiting for other jobs; express ordering with `AddWait` or `SyncJob`.
- Keep per-thread results in `Threading::CEqTLS<T>` (`framework_multithreading.md`) and merge them
  after the jobs are done.
- Do not destroy or re-init a job object while it is in `JOB_STARTED` phase.
