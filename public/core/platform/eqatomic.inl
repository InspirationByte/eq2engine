
#include "eqatomic.h"

#ifdef _MSC_VER
extern "C" long _InterlockedIncrement(long volatile*);
extern "C" long _InterlockedDecrement(long volatile*);
extern "C" long _InterlockedCompareExchange(long volatile*, long, long);
extern "C" __int64 _InterlockedCompareExchange64(__int64 volatile*, __int64, __int64);
extern "C" long _InterlockedExchange(long volatile*, long);
extern "C" long _InterlockedExchangeAdd(long volatile*, long);
extern "C" long _InterlockedOr(long volatile*, long);
#ifdef _M_AMD64
extern "C" __int64 _InterlockedIncrement64(__int64 volatile*);
extern "C" __int64 _InterlockedDecrement64(__int64 volatile*);
extern "C" __int64 _InterlockedCompareExchange64(__int64 volatile*, __int64, __int64);
extern "C" __int64 _InterlockedExchange64(__int64 volatile*, __int64);
extern "C" __int64 _InterlockedExchangeAdd64(__int64 volatile*, __int64);
extern "C" __int64 _InterlockedOr64(__int64 volatile*, __int64);
#endif // _M_AMD64
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedOr)
#ifdef _M_AMD64
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedOr64)
#endif // _M_AMD64
#endif

#ifdef _MSC_VER

inline int32	Atomic::Increment(volatile int32& var)						{ return _InterlockedIncrement((long volatile*)&var); }
inline uint32	Atomic::Increment(volatile uint32& var)						{ return _InterlockedIncrement((long volatile*)&var); }
inline int32	Atomic::Decrement(volatile int32& var)						{ return _InterlockedDecrement((long volatile*)&var); }
inline uint32	Atomic::Decrement(volatile uint32& var)						{ return _InterlockedDecrement((long volatile*)&var); }
inline int32	Atomic::Add(int32 volatile& var, int32 val)					{ return _InterlockedExchangeAdd((long volatile*)&var, val) + val; }
inline uint32	Atomic::Add(uint32 volatile& var, uint32 val)				{ return _InterlockedExchangeAdd((long volatile*)&var, val) + val; }

inline int32	Atomic::CompareExchange(int32 volatile& var, int32 oldVal, int32 newVal)	{ return _InterlockedCompareExchange((long volatile*)&var, newVal, oldVal); }
inline uint32	Atomic::CompareExchange(uint32 volatile& var, uint32 oldVal, uint32 newVal)	{ return _InterlockedCompareExchange((long volatile*)&var, newVal, oldVal); }
inline int64	Atomic::CompareExchange(int64 volatile& var, int64 oldVal, int64 newVal)	{ return _InterlockedCompareExchange64(&var, newVal, oldVal); }
inline uint64	Atomic::CompareExchange(uint64 volatile& var, uint64 oldVal, uint64 newVal)	{ return _InterlockedCompareExchange64((volatile __int64*)&var, newVal, oldVal); }

inline int32	Atomic::Exchange(int32 volatile& var, int32 val)			{ return _InterlockedExchange((long volatile*)&var, val); }
inline uint32	Atomic::Exchange(uint32 volatile& var, uint32 val)			{ return _InterlockedExchange((long volatile*)&var, val); }

#ifndef _M_ARM
inline int32	Atomic::Load(const int32 volatile& var)						{ return var; }
inline uint32	Atomic::Load(const uint32 volatile& var)					{ return var; }
inline void		Atomic::Store(int32 volatile& var, int32 val)				{ var = val; }
inline void		Atomic::Store(uint32 volatile& var, uint32 val)				{ var = val; }
#endif // _M_ARM

#ifdef _M_AMD64
inline int64	Atomic::Increment(volatile int64& var)						{ return _InterlockedIncrement64((__int64 volatile*)&var); }
inline uint64	Atomic::Increment(volatile uint64& var)						{ return _InterlockedIncrement64((__int64 volatile*)&var); }
inline int64	Atomic::Decrement(volatile int64& var)						{ return _InterlockedDecrement64((__int64 volatile*)&var); }
inline uint64	Atomic::Decrement(volatile uint64& var)						{ return _InterlockedDecrement64((__int64 volatile*)&var); }
inline int64	Atomic::Add(int64 volatile& var, int64 val)					{ return _InterlockedExchangeAdd64(&var, val) + val; }
inline uint64	Atomic::Add(uint64 volatile& var, uint64 val)				{ return _InterlockedExchangeAdd64((__int64 volatile*)&var, (__int64)val) + val; }

template <typename T> 
inline T* Atomic::CompareExchange(T* volatile& ptr, T* oldVal, T* newVal)	{ return (T*)_InterlockedCompareExchange64((__int64 volatile*)&ptr, (__int64&)newVal, (__int64&)oldVal); }

inline int64	Atomic::Exchange(int64 volatile& var, int64 val)			{ return _InterlockedExchange64(&var, val); }
inline uint64	Atomic::Exchange(uint64 volatile& var, uint64 val)			{ return _InterlockedExchange64((__int64 volatile*)&var, val); }
template <typename T>
inline T*		Atomic::Exchange(T* volatile& ptr, T* val)					{ return (T*)_InterlockedExchange64((__int64 volatile*)&ptr, (__int64&)val); }

#ifndef _M_ARM
inline int64	Atomic::Load(const int64 volatile& var)						{ return var; }
inline uint64	Atomic::Load(const uint64 volatile& var)					{ return var; }
template <typename T>
inline T*		Atomic::Load(T* const volatile& ptr)						{ return ptr; }

inline void		Atomic::Store(int64 volatile& var, int64 val)				{ var = val; }
inline void		Atomic::Store(uint64 volatile& var, uint64 val)				{ var = val; }
template <typename T>
inline void		Atomic::Store(T* volatile& ptr, T* val)						{ ptr = val; }
#endif	// !_M_ARM

#else // _M_AMD64
inline int64	Atomic::Increment(volatile int64& var)						{ for (int64 oldVal = var, newVal = oldVal + 1;; newVal = (oldVal = var) + 1) if (_InterlockedCompareExchange64(&var, newVal, oldVal) == oldVal) return newVal; }
inline uint64	Atomic::Increment(volatile uint64& var)						{ for (uint64 oldVal = var, newVal = oldVal + 1;; newVal = (oldVal = var) + 1) if (_InterlockedCompareExchange64((volatile __int64*)&var, newVal, oldVal) == oldVal) return newVal; }
inline int64	Atomic::Decrement(volatile int64& var)						{ for (int64 oldVal = var, newVal = oldVal - 1;; newVal = (oldVal = var) - 1) if (_InterlockedCompareExchange64(&var, newVal, oldVal) == oldVal) return newVal; }
inline uint64	Atomic::Decrement(volatile uint64& var)						{ for (uint64 oldVal = var, newVal = oldVal - 1;; newVal = (oldVal = var) - 1) if (_InterlockedCompareExchange64((volatile __int64*)&var, newVal, oldVal) == oldVal) return newVal; }

inline int64	Atomic::Add(int64 volatile& var, int64 val)	
{
	for (int64 oldVal = var;; oldVal = var)
		if (_InterlockedCompareExchange64(&var, oldVal + val, oldVal) == oldVal) 
			return oldVal + val;
}

inline uint64	Atomic::Add(uint64 volatile& var, uint64 val)
{
	for (uint64 oldVal = var;; oldVal = var)
		if (_InterlockedCompareExchange64((__int64 volatile*)&var, (__int64)(oldVal + val), oldVal) == oldVal)
			return oldVal + val; 
}

template <typename T>
inline T*		Atomic::CompareExchange(T* volatile& ptr, T* oldVal, T* newVal)	{ return (T*)_InterlockedCompareExchange((long volatile*)&ptr, (long&)newVal, (long&)oldVal); }
inline int64	Atomic::Exchange(int64 volatile& var, int64 val)			{ for (int64 oldVal = var;; oldVal = var) if (_InterlockedCompareExchange64(&var, val, oldVal) == oldVal) return oldVal; }
inline uint64	Atomic::Exchange(uint64 volatile& var, uint64 val)			{ for (uint64 oldVal = var;; oldVal = var) if (_InterlockedCompareExchange64((__int64 volatile*)&var, (__int64)val, oldVal) == oldVal) return oldVal; }
template <typename T>
inline T*		Atomic::Exchange(T* volatile& ptr, T* val)					{ return (T*)_InterlockedExchange((long volatile*)&ptr, (long)val); }

inline int64	Atomic::Add(int64 volatile& var, int64 val)			{ for (int64 oldVal = var;; oldVal = var) if (_InterlockedCompareExchange64(&var, oldVal + val, oldVal) == oldVal) return oldVal; }
inline uint64	Atomic::Add(uint64 volatile& var, uint64 val)		{ for (uint64 oldVal = var;; oldVal = var) if (_InterlockedCompareExchange64((__int64 volatile*)&var, (__int64)(oldVal + val), oldVal) == oldVal) return oldVal; }

inline int64	Atomic::Load(const volatile int64& var)
{
	for (int64 val = var;; val = var) 
		if (_InterlockedCompareExchange64((volatile __int64*)&var, val, val) == val) 
			return val;
}
inline uint64	Atomic::Load(const volatile uint64& var)
{
	for (uint64 val = var;; val = var)
		if (_InterlockedCompareExchange64((volatile __int64*)&var, val, val) == val) 
			return val; 
}
template <typename T>
inline T*		Atomic::Load(T* const volatile& ptr)						{ return (T*)_InterlockedOr((long volatile*)&ptr, 0); }

inline void		Atomic::Store(int64 volatile& var, int64 val)				{ Atomic::Exchange(var, val); }
inline void		Atomic::Store(uint64 volatile& var, uint64 val)				{ Atomic::Exchange(var, val); }
template <typename T> 
inline  void	Atomic::Store(T* volatile& ptr, T* val)						{ _InterlockedExchange((long volatile*)&ptr, (long)val); }
#endif // _M_AMD64

#else

inline int32	Atomic::Increment(volatile int32& var)						{ return __sync_add_and_fetch(&var, 1); }
inline uint32	Atomic::Increment(volatile uint32& var)						{ return __sync_add_and_fetch(&var, 1); }
inline int64	Atomic::Increment(volatile int64& var)						{ return __sync_add_and_fetch(&var, 1); }
inline uint64	Atomic::Increment(volatile uint64& var)						{ return __sync_add_and_fetch(&var, 1); }

inline int32	Atomic::Decrement(volatile int32& var)						{ return __sync_add_and_fetch(&var, -1); }
inline uint32	Atomic::Decrement(volatile uint32& var)						{ return __sync_add_and_fetch(&var, -1); }
inline int64	Atomic::Decrement(volatile int64& var)						{ return __sync_add_and_fetch(&var, -1); }
inline uint64	Atomic::Decrement(volatile uint64& var)						{ return __sync_add_and_fetch(&var, -1); }

inline int32	Atomic::Add(int32 volatile& var, int32 val)					{ return __sync_add_and_fetch(&var, val); }
inline uint32	Atomic::Add(uint32 volatile& var, uint32 val)				{ return __sync_add_and_fetch(&var, val); }
inline int64	Atomic::Add(int64 volatile& var, int64 val)					{ return __sync_add_and_fetch(&var, val); }
inline uint64	Atomic::Add(uint64 volatile& var, uint64 val)				{ return __sync_add_and_fetch(&var, val); }

inline int32	Atomic::CompareExchange(int32 volatile& var, int32 oldVal, int32 newVal)	{ return __sync_val_compare_and_swap(&var, oldVal, newVal); }
inline uint32	Atomic::CompareExchange(uint32 volatile& var, uint32 oldVal, uint32 newVal)	{ return __sync_val_compare_and_swap(&var, oldVal, newVal); }
inline int64	Atomic::CompareExchange(int64 volatile& var, int64 oldVal, int64 newVal)	{ return __sync_val_compare_and_swap(&var, oldVal, newVal); }
inline uint64	Atomic::CompareExchange(uint64 volatile& var, uint64 oldVal, uint64 newVal)	{ return __sync_val_compare_and_swap(&var, oldVal, newVal); }
template <typename T> 
inline T*		Atomic::CompareExchange(T* volatile& ptr, T* oldVal, T* newVal)	{ return (T*)__sync_val_compare_and_swap(&ptr, oldVal, newVal); }

inline int32	Atomic::Exchange(int32 volatile& var, int32 val)			{ return __sync_lock_test_and_set(&var, val); }
inline uint32	Atomic::Exchange(uint32 volatile& var, uint32 val)			{ return __sync_lock_test_and_set(&var, val); }
inline int64	Atomic::Exchange(int64 volatile& var, int64 val)			{ return __sync_lock_test_and_set(&var, val); }
inline uint64	Atomic::Exchange(uint64 volatile& var, uint64 val)			{ return __sync_lock_test_and_set(&var, val); }
template <typename T>
inline T*		Atomic::Exchange(T* volatile& ptr, T* val)					{ return (T*)__sync_lock_test_and_set(&ptr, val); }

inline int32	Atomic::Load(const int32 volatile& var)						{ __sync_synchronize(); return var; }
inline uint32	Atomic::Load(const uint32 volatile& var)					{ __sync_synchronize(); return var; }
inline int64	Atomic::Load(const int64 volatile& var)						{ __sync_synchronize(); return var; }
inline uint64	Atomic::Load(const uint64 volatile& var)					{ __sync_synchronize(); return var; }
template <typename T>
inline T*		Atomic::Load(T* const volatile& ptr)						{ __sync_synchronize(); return ptr; }

inline void	Atomic::Store(int32 volatile& var, int32 value)					{ __sync_synchronize(); var = value; }
inline void	Atomic::Store(uint32 volatile& var, uint32 value)				{ __sync_synchronize(); var = value; }
inline void	Atomic::Store(int64 volatile& var, int64 value)					{ __sync_synchronize(); var = value; }
inline void	Atomic::Store(uint64 volatile& var, uint64 value)				{ __sync_synchronize(); var = value; }
template <typename T>
inline void	Atomic::Store(T* volatile& ptr, T* value)						{ __sync_synchronize(); ptr = value; }

#endif // _MSC_VER