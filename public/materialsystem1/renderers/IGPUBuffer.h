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

//---------------------------------------

struct GPUBufferPtrView
{
	GPUBufferPtrView() = default;
	GPUBufferPtrView(IGPUBuffer* buffer, int64 offset = 0, int64 size = -1)
		: buffer(buffer), offset(offset), size((buffer&& size > 0) ? size : buffer->GetSize())
	{
	}
	GPUBufferPtrView(IGPUBufferPtr buffer, int64 offset = 0, int64 size = -1)
		: buffer(buffer), offset(offset), size((buffer&& size > 0) ? size : buffer->GetSize())
	{
	}

	operator bool() const { return buffer; }

	IGPUBufferPtr	buffer;
	int64			offset{ 0 };
	int64			size{ -1 };
};

//---------------------------------------

template<typename T>
static T AlignBufferSize(T size, int alignment = 4)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

template<typename T>
static T NextBufferOffset(T writeSize, T& offset, T bufferSize, int alignment = 4)
{
	ASSERT(writeSize < bufferSize);
	if (offset + writeSize > bufferSize)
	{
		ASSERT_FAIL("Exceeded buffer size %d, needed %d", bufferSize, offset + writeSize);
	}

	offset = (offset + (alignment - 1)) & ~(alignment - 1);

	const T writeOffset = offset;
	offset += (writeSize + (alignment - 1)) & ~(alignment - 1);
	return writeOffset;
}