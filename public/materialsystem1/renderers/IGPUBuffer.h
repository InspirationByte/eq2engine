#pragma once

#include "ds/future.h"

struct BufferLockData
{
	void* data{ nullptr };
	int		offset{ 0 };
	int		size{ 0 };
	int		flags{ 0 };
};

class IGPUBuffer
{
public:
	using LockFuture = Future<BufferLockData>;

	virtual void		Update(void* data, int size, int offset) = 0;
	virtual LockFuture	Lock(int lockOfs, int sizeToLock, void** outdata, int flags) = 0;
	virtual void		Unlock() = 0;
};