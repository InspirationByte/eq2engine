#pragma once

#include "ds/future.h"

struct BufferMapData
{
	void*	data{ nullptr };
	int		offset{ 0 };
	int		size{ 0 };
	int		flags{ 0 };
};

class IGPUBuffer : public RefCountedObject<IGPUBuffer>
{
public:
	using MapFuture = Future<BufferMapData>;

	virtual void		Update(const void* data, int64 size, int64 offset) = 0;
	virtual MapFuture	Lock(int lockOfs, int sizeToLock, int flags) = 0;
	virtual void		Unlock() = 0;

	virtual int			GetSize() const = 0;
	virtual int			GetUsageFlags() const = 0;
};
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

//---------------------------------------

struct GPUBufferView
{
	GPUBufferView() = default;
	GPUBufferView(IGPUBuffer* buffer, int64 offset = 0, int64 size = -1)
		: buffer(buffer), offset(offset), size(size > 0 ? size : (buffer ? buffer->GetSize() - offset : -1))
	{
	}
	GPUBufferView(IGPUBufferPtr buffer, int64 offset = 0, int64 size = -1)
		: buffer(buffer), offset(offset), size(size > 0 ? size : (buffer ? buffer->GetSize() - offset : -1))
	{
	}

	operator bool() const { return buffer; }

	friend bool operator==(const GPUBufferView& a, const GPUBufferView& b) { return a.buffer == b.buffer && a.offset == b.offset && a.size == b.size; }
	friend bool operator!=(const GPUBufferView& a, const GPUBufferView& b) { return !(a == b); }

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