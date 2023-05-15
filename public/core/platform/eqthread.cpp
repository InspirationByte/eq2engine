//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine threads
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#ifndef _Requires_lock_held_
// GCC stub
#define _Requires_lock_held_(lock)
#endif

namespace Threading
{
using threadfunc_t = unsigned int (*)(void*);

#ifdef _WIN32

#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;		// Must be 0x1000.
	LPCSTR szName;		// Pointer to name (in user addr space).
	DWORD dwThreadID;	// Thread ID (-1=caller thread).
	DWORD dwFlags;		// Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

using SetThreadDescriptionPROC = HRESULT(__cdecl* )(HANDLE, PCWSTR);
using GetThreadDescriptionPROC = HRESULT(__cdecl* )(HANDLE, PWSTR*);

static bool	s_loadedThreadProc = false;
static SetThreadDescriptionPROC s_SetThreadDescription = nullptr;
static GetThreadDescriptionPROC s_GetThreadDescription = nullptr;

void InitThreadNameAPI()
{
	if (s_loadedThreadProc)
		return;

	HINSTANCE hinstLib;
	hinstLib = LoadLibrary(TEXT("Kernel32.dll"));
	if (hinstLib != NULL)
	{
		s_SetThreadDescription = (SetThreadDescriptionPROC)GetProcAddress(hinstLib, "SetThreadDescription");
		s_GetThreadDescription = (GetThreadDescriptionPROC)GetProcAddress(hinstLib, "GetThreadDescription");
	}
	s_loadedThreadProc = true;
}

void GetThreadName(uintptr_t threadID, char* name, int maxLength)
{
	EqWString threadName;
#ifdef _WIN64
	InitThreadNameAPI();
	if (s_GetThreadDescription)
	{
		HANDLE threadHandle = OpenThread(THREAD_QUERY_INFORMATION, false, threadID);
		if (threadHandle != nullptr)
		{
			PWSTR wName;
			if (!FAILED(s_GetThreadDescription(threadHandle, &wName)))
			{
				threadName = wName;
				LocalFree(wName);
			}
			CloseHandle(threadHandle);
		}
	}
#endif
	if (!threadName.Length())
		threadName = EqWString::Format(L"Thread %d", threadID);

	strcpy_s(name, maxLength, EqString(threadName));
}

void SetThreadNameOLD(uintptr_t threadID, const char* name)
{
	THREADNAME_INFO info;

	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadID;
	info.dwFlags = 0;

	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
	}
	// this much is just to keep /analyze quiet
	__except (GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		info.dwFlags = 0;
	}
}

void SetThreadName(uintptr_t threadID, const char * name )
{
#ifdef _WIN64
	// Load up SetThreadDescription and see if it is available
	InitThreadNameAPI();

	if (s_SetThreadDescription)
	{
		EqWString wThreadName(name);

		HANDLE threadHandle = OpenThread(THREAD_SET_INFORMATION, false, threadID);
		if (threadHandle != nullptr)
		{
			s_SetThreadDescription(threadHandle, wThreadName);
			CloseHandle(threadHandle);
		}
		return;
	}
#endif
	SetThreadNameOLD(threadID, name);
}

uintptr_t ThreadCreate( threadfunc_t fnThread, void* pThreadParams, ThreadPriority_e nPriority,
								  const char* pszThreadName, int nStackSize, bool bSuspended )
{
	const DWORD flags = STACK_SIZE_PARAM_IS_A_RESERVATION | ( bSuspended ? CREATE_SUSPENDED : 0 );

	DWORD threadId;
	HANDLE handle = CreateThread(nullptr,	// LPSECURITY_ATTRIBUTES lpsa, //-V513
									nStackSize,
									(LPTHREAD_START_ROUTINE)fnThread,
									pThreadParams,
									flags,
									&threadId);
	if ( handle == 0 )
	{
		MsgError( "CreateThread error: %i", GetLastError() );
		return (uintptr_t)0;
	}

	if( nPriority == TP_HIGHEST )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_HIGHEST );		//  we better sleep enough to do this
	}
	else if( nPriority == TP_ABOVE_NORMAL )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_ABOVE_NORMAL );
	}
	else if( nPriority == TP_BELOW_NORMAL )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_BELOW_NORMAL );
	}
	else if ( nPriority == TP_LOWEST )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_LOWEST );
	}

	SetThreadName(threadId, pszThreadName);

	// Under Windows, we don't set the thread affinity and let the OS deal with scheduling

	return (uintptr_t)handle;
}

uintptr_t GetCurrentThreadID()
{
	return GetCurrentThreadId();
}

void SetCurrentThreadName(const char* name)
{
	SetThreadName(GetCurrentThreadID(), name);
}

uintptr_t ThreadGetID(uintptr_t handle)
{
	return (uintptr_t)GetThreadId((HANDLE)handle);
}

void ThreadDestroy( uintptr_t threadHandle )
{
	if ( threadHandle == 0 )
	{
		return;
	}

	WaitForSingleObject( (HANDLE)threadHandle, INFINITE );
	CloseHandle( (HANDLE)threadHandle );
}

void Yield()
{
	// FIXME: Sleep(1)
	SwitchToThread();
}

//----------------------------------------------------------
// Signal

void SignalCreate( SignalHandle_t& handle, bool bManualReset )
{
	handle = CreateEvent(nullptr, bManualReset, FALSE, nullptr);
}

void SignalDestroy( SignalHandle_t& handle )
{
	CloseHandle( handle );
}

void SignalRaise( SignalHandle_t& handle )
{
	SetEvent( handle );
}

void SignalClear( SignalHandle_t& handle )
{
	// events are created as auto-reset so this should never be needed
	ResetEvent( handle );
}

bool SignalWait( SignalHandle_t& handle, int nTimeout )
{
	if(handle == 0)
		return true;

	DWORD result = WaitForSingleObject( handle, nTimeout );
	ASSERT( result == WAIT_OBJECT_0 || ( nTimeout != INFINITE && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
}

//----------------------------------------------------------
// Mutual Exclusion

void MutexCreate( MutexHandle_t& handle )
{
	InitializeCriticalSection( &handle );
}

void MutexDestroy( MutexHandle_t& handle )
{
	DeleteCriticalSection( &handle );
}

bool MutexLock( MutexHandle_t& handle, bool bBlocking )
{
	if ( TryEnterCriticalSection( &handle ) == 0 )
	{
		if ( !bBlocking )
			return false;

		EnterCriticalSection( &handle );
	}

	return true;
}

void MutexUnlock( MutexHandle_t& handle )
{
	LeaveCriticalSection( &handle );
}

//----------------------------------------------------------
// Read-Write locks

void ReadWriteLockCreate(ReadWriteLockHandle_t& handle)
{
	InitializeSRWLock(&handle);
}

void ReadWriteLockDestroy(ReadWriteLockHandle_t& handle)
{
}

void ReadWriteLockRead(ReadWriteLockHandle_t& handle)
{
	AcquireSRWLockShared(&handle);
}

bool ReadWriteLockWrite(ReadWriteLockHandle_t& handle, bool bBlocking)
{
	if (bBlocking)
	{
		AcquireSRWLockExclusive(&handle);
	}
	else
	{
		if (!TryAcquireSRWLockExclusive(&handle))
			return false;
	}

	return true;
}

void ReadWriteUnlockRead(ReadWriteLockHandle_t& handle)
{
	ReleaseSRWLockShared(&handle);
}

void ReadWriteUnlockWrite(_Requires_lock_held_(handle) ReadWriteLockHandle_t& handle)
{
	ReleaseSRWLockExclusive(&handle);
}

#elif defined(PLAT_POSIX)
// Any other (POSIX)

void GetThreadName(uintptr_t threadID, char* name, int maxLength)
{
	// TODO !!!
	EqString threadName = EqString::Format("Thread %d", threadID);
	strcpy(name, threadName);
}

void SetThreadName(uintptr_t threadID, const char* name)
{
	// TODO !!!
}

using pthread_function_t = void* ( * )( void* );

uintptr_t ThreadCreate( threadfunc_t fnThread, void* pThreadParams, ThreadPriority_e nPriority,
								  const char* pszThreadName, int nStackSize, bool bSuspended )
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );

	if( pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) != 0 )
	{
		ASSERT_FAIL( "ERROR: pthread_attr_setdetachstate %s failed\n", pszThreadName );
		return ( uintptr_t )0;
	}

	pthread_t handle;
	if( pthread_create( ( pthread_t* )&handle, &attr, ( pthread_function_t )fnThread, pThreadParams ) != 0 )
	{
		ASSERT_FAIL( "ERROR: pthread_create %s failed\n", pszThreadName );
		return ( uintptr_t )0;
	}

	pthread_attr_destroy( &attr );

	return ( uintptr_t )handle;
}

uintptr_t GetCurrentThreadID()
{
	return ( uintptr_t )pthread_self();
}

void SetCurrentThreadName(const char* name)
{
	SetThreadName(GetCurrentThreadID(), name);
}

uintptr_t ThreadGetID(uintptr_t threadHandle)
{
    return threadHandle;
}

void ThreadDestroy( uintptr_t threadHandle )
{
	if( threadHandle == 0 )
		return;

	char name[128]{ 0 };

#if 0 //!defined(__ANDROID__)
	if( pthread_cancel( ( pthread_t )threadHandle ) != 0 )
	{
		ASSERT_FAIL( "ERROR: pthread_cancel %s failed\n", name );
	}
#endif

	if( pthread_join( ( pthread_t )threadHandle, nullptr) != 0 )
	{
		ASSERT_FAIL( "ERROR: pthread_join %s failed\n", name );
	}
}

void Yield()
{
#if defined(__ANDROID__) || defined(__APPLE__)
	sched_yield();
#else
	pthread_yield();
#endif
}

//----------------------------------------------------------
// Signal

void SignalCreate( SignalHandle_t& handle, bool bManualReset )
{
	handle.manualReset = bManualReset;

	// if this is true, the signal is only set to nonsignaled when Clear() is called,
	// else it's "auto-reset" and the state is set to !signaled after a single waiting
	// thread has been released

	// the inital state is always "not signaled"
	handle.signaled = false;
	handle.waiting = 0;

	pthread_mutex_init( &handle.mutex, nullptr);

	pthread_cond_init( &handle.cond, nullptr);
}

void SignalDestroy( SignalHandle_t& handle )
{
	handle.signaled = false;
	handle.waiting = 0;
	pthread_mutex_destroy( &handle.mutex );
	pthread_cond_destroy( &handle.cond );
}

void SignalRaise( SignalHandle_t& handle )
{
	pthread_mutex_lock( &handle.mutex );

	if( handle.manualReset )
	{
		// signaled until reset
		handle.signaled = true;
		// wake *all* threads waiting on this cond
		pthread_cond_broadcast( &handle.cond );
	}
	else
	{
		// automode: signaled until first thread is released
		if( handle.waiting > 0 )
		{
			// there are waiting threads => release one
			pthread_cond_signal( &handle.cond );
		}
		else
		{
			// no waiting threads, save signal
			handle.signaled = true;
			// while the MSDN documentation is a bit unspecific about what happens
			// when SetEvent() is called n times without a wait inbetween
			// (will only one wait be successful afterwards or n waits?)
			// it seems like the signaled state is a flag, not a counter.
			// http://stackoverflow.com/a/13703585 claims the same.
		}
	}

	pthread_mutex_unlock( &handle.mutex );
}

void SignalClear( SignalHandle_t& handle )
{
	// events are created as auto-reset so this should never be needed
	pthread_mutex_lock( &handle.mutex );

	// TODO: probably signaled could be atomically changed?
	handle.signaled = false;

	pthread_mutex_unlock( &handle.mutex );
}

bool SignalWait( SignalHandle_t& handle, int nTimeout )
{
	int status;
	pthread_mutex_lock( &handle.mutex );

	if( handle.signaled ) // there is a signal that hasn't been used yet
	{
		if( ! handle.manualReset ) // for auto-mode only one thread may be released - this one.
			handle.signaled = false;

		status = 0; // success!
	}
	else // we'll have to wait for a signal
	{
		++handle.waiting;
		if( nTimeout == WAIT_INFINITE )
		{
			status = pthread_cond_wait( &handle.cond, &handle.mutex );
		}
		else
		{
			timespec ts;

			clock_gettime( CLOCK_REALTIME, &ts );

			ts.tv_nsec += ( nTimeout % 1000 ) * 1000000; // millisec to nanosec
			ts.tv_sec  += nTimeout / 1000;
			if( ts.tv_nsec >= 1000000000 ) // nanoseconds are more than one second
			{
				ts.tv_nsec -= 1000000000; // remove one second in nanoseconds
				ts.tv_sec += 1; // add one second to seconds
			}

			status = pthread_cond_timedwait( &handle.cond, &handle.mutex, &ts );
		}
		--handle.waiting;
	}

	pthread_mutex_unlock( &handle.mutex );

	ASSERT( status == 0 || ( nTimeout != WAIT_INFINITE && status == ETIMEDOUT ) );

	return ( status == 0 );
}

//----------------------------------------------------------
// Mutual Exclusion

void MutexCreate( MutexHandle_t& handle )
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle, &attr );

	pthread_mutexattr_destroy( &attr );
}

void MutexDestroy( MutexHandle_t& handle )
{
	pthread_mutex_destroy( &handle );
}

bool MutexLock( MutexHandle_t& handle, bool bBlocking )
{
	if( pthread_mutex_trylock( &handle ) != 0 )
	{
		if( !bBlocking )
		{
			return false;
		}
		pthread_mutex_lock( &handle );
	}

	return true;
}

void MutexUnlock( MutexHandle_t& handle )
{
	pthread_mutex_unlock( & handle );
}

//----------------------------------------------------------
// Read-Write locks

void ReadWriteLockCreate(ReadWriteLockHandle_t& handle)
{
	pthread_rwlock_init(&handle, nullptr);
}

void ReadWriteLockDestroy(ReadWriteLockHandle_t& handle)
{
	pthread_rwlock_destroy(&handle);
}

void ReadWriteLockRead(ReadWriteLockHandle_t& handle)
{
	pthread_rwlock_rdlock(&handle);
}

bool ReadWriteLockWrite(ReadWriteLockHandle_t& handle, bool bBlocking)
{
	if (bBlocking)
	{
		pthread_rwlock_wrlock(&handle);
	}
	else
	{
		if (pthread_rwlock_trywrlock(&handle) != 0)
			return false;
	}

	return true;
}

void ReadWriteUnlockRead(ReadWriteLockHandle_t& handle)
{
	pthread_rwlock_unlock(&handle);
}

void ReadWriteUnlockWrite(_Requires_lock_held_(handle) ReadWriteLockHandle_t& handle)
{
	pthread_rwlock_unlock(&handle);
}

#endif // _WIN32

//-------------------------------------------------------------------------------------------------------------------------

CEqThread::CEqThread() 
	: m_SignalWorkerDone(true)
{
}

CEqThread::~CEqThread()
{
	StopThread( true );

	if ( m_nThreadHandle )
		ThreadDestroy( m_nThreadHandle );
}

uintptr_t CEqThread::GetThreadID() const 
{
	return ThreadGetID(m_nThreadHandle);
}

bool CEqThread::StartThread( const char * name, ThreadPriority_e priority, int stackSize )
{
	if ( m_bIsRunning )
		return false;

	m_szName = name;

	m_bIsTerminating = false;

	if ( m_nThreadHandle )
		ThreadDestroy( m_nThreadHandle );

	m_nThreadHandle = ThreadCreate( (threadfunc_t)ThreadProc, this, priority, name, stackSize, false );

	m_bIsRunning = true;
	return true;
}

bool CEqThread::StartWorkerThread( const char * name_, ThreadPriority_e priority, int stackSize )
{
	if ( m_bIsRunning )
		return false;

	m_bIsWorker = true;

	const bool result = StartThread( name_, priority, stackSize );

	m_SignalWorkerDone.Wait();

	return result;
}

void CEqThread::StopThread( bool wait )
{
	if ( !m_bIsRunning || m_bIsTerminating )
		return;

	if ( m_bIsWorker )
	{
		m_SignalMutex.Lock();

		m_bMoreWorkToDo = true;

		m_SignalWorkerDone.Clear();

		m_bIsTerminating = true;

		m_SignalMoreWorkToDo.Raise();

		m_SignalMutex.Unlock();
	}
	else
		m_bIsTerminating = true;

	if ( wait )
		WaitForThread();
}

void CEqThread::WaitForThread(int timeout)
{
	if ( m_bIsWorker )
	{
		m_SignalWorkerDone.Wait(timeout);
	}
	else if ( m_bIsRunning )
	{
		ThreadDestroy( m_nThreadHandle );
		m_nThreadHandle = 0;
	}
}

void CEqThread::SignalWork()
{
	if ( m_bIsWorker )
	{
		m_SignalMutex.Lock();

		m_bMoreWorkToDo = true;

		m_SignalWorkerDone.Clear();

		m_SignalMoreWorkToDo.Raise();

		m_SignalMutex.Unlock();
	}
}

bool CEqThread::IsWorkDone()
{
	if ( m_bIsWorker )
	{
		// a timeout of 0 will return immediately with true if signaled
		if ( m_SignalWorkerDone.Wait( 0 ) )
			return true;
	}

	return false;
}

int CEqThread::ThreadProc( CEqThread* thread )
{
	int retVal = 0;

	if ( thread->m_bIsWorker )
	{
		for( ; ; )
		{
			thread->m_SignalMutex.Lock();

			if ( thread->m_bMoreWorkToDo )
			{
				thread->m_bMoreWorkToDo = false;

				thread->m_SignalMoreWorkToDo.Clear();
				thread->m_SignalMutex.Unlock();
			}
			else
			{
				thread->m_SignalWorkerDone.Raise();
				thread->m_SignalMutex.Unlock();

				thread->m_SignalMoreWorkToDo.Wait();
				continue;
			}

			if ( thread->m_bIsTerminating )
				break;

			retVal = thread->Run();
		}

		thread->m_SignalWorkerDone.Raise();
	}
	else
		retVal = thread->Run();

	thread->m_bIsRunning = false;

	PROF_RELEASE_THREAD_MARKERS();

	return retVal;
}

int CEqThread::Run()
{
	// The Run() is not pure virtual because on destruction of a derived class
	// the virtual function pointer will be set to NULL before the CEqThread
	// destructor actually stops the thread.

	return 0;
}

//--------------------------------------------------------

CEqMutex::CEqMutex()
{
	MutexCreate(m_nHandle);
}

CEqMutex::~CEqMutex()
{
	MutexDestroy(m_nHandle);
}

bool CEqMutex::Lock(bool blocking)
{
	return MutexLock(m_nHandle, blocking);
}

void CEqMutex::Unlock()
{
	MutexUnlock(m_nHandle);
}

CEqSignal::CEqSignal(bool manualReset)
{
	SignalCreate(m_nHandle, manualReset); 
}

CEqSignal::~CEqSignal()
{
	SignalDestroy(m_nHandle);
}

void CEqSignal::Raise()
{
	SignalRaise(m_nHandle); 
}

void CEqSignal::Clear() 
{
	SignalClear(m_nHandle); 
}

bool CEqSignal:: Wait(int timeout)
{
	return SignalWait(m_nHandle, timeout); 
}

//--------------------------------------------------------

CEqReadWriteLock::CEqReadWriteLock()
{
	ReadWriteLockCreate(m_nHandle);
}

CEqReadWriteLock::~CEqReadWriteLock()
{
	ReadWriteLockDestroy(m_nHandle);
}

void CEqReadWriteLock::LockRead()
{
	ReadWriteLockRead(m_nHandle);
}

void CEqReadWriteLock::UnlockRead()
{
	ReadWriteUnlockRead(m_nHandle);
}

bool CEqReadWriteLock::LockWrite(bool blocking)
{
	return ReadWriteLockWrite(m_nHandle, blocking);
}

void CEqReadWriteLock::UnlockWrite()
{
	ReadWriteUnlockWrite(_Requires_lock_held_(m_nHandle) m_nHandle);
}


};
