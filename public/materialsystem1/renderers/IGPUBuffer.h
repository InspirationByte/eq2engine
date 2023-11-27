#pragma once

#include "ds/future.h"

struct BufferLockData
{
	void*	data{ nullptr };
	int		offset{ 0 };
	int		size{ 0 };
	int		flags{ 0 };
};

class IGPUBuffer : public RefCountedObject<IGPUBuffer>
{
public:
	using LockFuture = Future<BufferLockData>;

	virtual void		Update(const void* data, int64 size, int64 offset) = 0;
	virtual LockFuture	Lock(int lockOfs, int sizeToLock, int flags) = 0;
	virtual void		Unlock() = 0;

	virtual int			GetSize() const = 0;
};
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;