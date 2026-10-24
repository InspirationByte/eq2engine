# framework/core/platform — multithreading

Threads, synchronization primitives, thread-local storage and atomics from
`framework/core/platform/` (`coreLib`). `eqthread.h` and `eqatomic.h` are included by
`core/core_common.h`. Use them instead of `std::thread`, `std::mutex`, `std::condition_variable`,
`std::atomic` and OS APIs in engine code. For running many short tasks in parallel use the job
manager (`framework_job_manager.md`).

## Threads and synchronization — `eqthread.h`

Everything is in `namespace Threading`.

### Mutexes and locks

| Type | Purpose |
|---|---|
| `CEqMutex` | Exclusive lock: `Lock(blocking = true)` (returns `false` if a non-blocking lock failed), `Unlock()` |
| `CScopedMutex` | RAII lock of `CEqMutex` for the current scope |
| `CEqReadWriteLock` | Shared/exclusive lock: `LockRead`/`UnlockRead`, `LockWrite(blocking)`/`UnlockWrite` |
| `CScopedReadLocker`, `CScopedWriteLocker` | RAII read / write lock |

```cpp
Threading::CEqMutex m_mutex;

void CMyCache::Add(const Item& item)
{
	Threading::CScopedMutex m(m_mutex);
	m_items.append(item);
}
```

Use the scoped lockers with blocking mode only: they unlock in the destructor without checking
whether a non-blocking lock succeeded.

### Signals

`CEqSignal(manualReset = false)` — an event a thread can wait on.

- `Raise()` sets the signal, `Clear()` resets it.
- `Wait(timeout = WAIT_INFINITE)` returns `true` when signalled and `false` on timeout (milliseconds).
  An auto-reset signal (`manualReset = false`) clears itself when a wait succeeds; a manual-reset
  signal stays raised until `Clear()`.

### Threads

Derive from `CEqThread` and override `Run()`.

```cpp
class CLoaderThread : public Threading::CEqThread
{
protected:
	int Run() override
	{
		// work
		return 0;
	}
};

CLoaderThread thread;
thread.StartThread("Loader");
...
thread.StopThread();	// waits for completion by default
```

Worker mode — the thread sleeps until work is signalled:

```cpp
thread.StartWorkerThread("Worker");

thread.SignalWork();		// run Run() once on the worker
// ... other work in parallel ...
thread.WaitForThread();		// wait until Run() finished
```

- `StartThread(name, priority, stackSize)` — `Run()` executes once; `StartWorkerThread(...)` —
  `Run()` executes on every `SignalWork()`.
- `StopThread(wait = true)`, `IsRunning()`, `IsTerminating()` (check it in long loops inside
  `Run()`), `IsWorkDone()`, `GetName()`, `GetThreadID()`.
- Priorities: `TP_LOWEST` … `TP_HIGHEST`; default stack size `DEFAULT_THREAD_STACK_SIZE` (3 MB).
- For many short tasks use the job manager instead of own threads.

### Thread-local storage

- `CEqTLS<T>` — one `T` instance per thread, created on first `Get()`. Objects stay alive after
  their threads finish until `Clear()` or destruction; `ForEach(fn)` visits all instances (e.g. to
  merge per-thread results after jobs complete). Construct with `PP_SL`.
- Low level: `TLSAlloc`, `TLSFree`, `TLSSet`, `TLSGet`.

```cpp
Threading::CEqTLS<Array<int>> s_threadResults(PP_SL);

s_threadResults.Get().append(value);			// in a job
s_threadResults.ForEach([&](Array<int>& list) {	// after jobs are done
	total.append(list);
});
```

### Thread utilities

`YieldCurrentThread()`, `GetCurrentThreadID()`, `SetCurrentThreadName(name)`,
`SetCurrentThreadAffinity(mask)`, `GetCurrentProcessAffinity`, `SetCurrentProcessAffinity`,
`GetThreadName(threadID, buffer, maxLength)`.

## Atomics — `eqatomic.h`

Static functions of class `Atomic` for `int32`, `uint32`, `int64`, `uint64` and pointers. Variables
are declared `volatile`.

| Function | Returns |
|---|---|
| `Increment(var)`, `Decrement(var)` | New value |
| `Add(var, value)` | New value |
| `Exchange(var, value)` | Previous value (sequentially consistent store) |
| `CompareExchange(var, expected, newValue)` | Previous value; the store happened if it equals `expected` |
| `Load(var)`, `Store(var, value)` | Relaxed load / store |

```cpp
volatile int m_refCount{ 0 };

if (Atomic::Decrement(m_refCount) == 0)
	Destroy();

Item* oldItem = Atomic::Exchange(m_current, newItem);
```

