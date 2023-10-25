#pragma once
#include <webgpu/webgpu.h>

#include "renderers/IVertexBuffer.h"
#include "renderers/IIndexBuffer.h"
#include "renderers/IGPUBuffer.h"

class CWGPUBuffer : public IGPUBuffer
{
public:
	void		Init(const BufferInfo& bufferInfo, int wgpuUsage);

	void		Update(void* data, int size, int offset);
	LockFuture	Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void		Unlock();

private:
	WGPUBuffer	m_rhiBuffer{ nullptr };
	int			m_bufSize{ 0 };
};

// OLD deprecated buffers - stubs for API compatibility

class CWGPUVertexBuffer : public IVertexBuffer
{
public:
	CWGPUVertexBuffer(int stride)
		: m_lockData(nullptr), m_stride(stride)
	{}

	int			GetSizeInBytes() const { return 0; }

	int			GetVertexCount() const { return 0; }
	int			GetStrideSize() const { return m_stride; }

	void		Update(void* data, int size, int offset = 0)
	{
	}

	bool		Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
	{
		*outdata = PPAlloc(sizeToLock*m_stride);
		return true;
	}

	void		Unlock() { PPFree(m_lockData); m_lockData = nullptr; }

	void		SetFlags( int flags ) {}
	int			GetFlags() const { return 0; }

	void*		m_lockData{ nullptr };
	int			m_stride{ 0 };
};



class CWGPUIndexBuffer : public IIndexBuffer
{
public:
	CWGPUIndexBuffer(int stride)
		: m_lockData(nullptr), m_stride(stride)
	{}

	int			GetSizeInBytes() const { return 0; }
	int			GetIndexSize() const {return m_stride;}
	int			GetIndicesCount()  const {return 0;}

	void		Update(void* data, int size, int offset = 0)
	{
	}

	bool		Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
	{
		*outdata = malloc(sizeToLock*m_stride);
		return true;
	}
	void		Unlock() {free(m_lockData); m_lockData = nullptr;}

	void		SetFlags( int flags ) {}
	int			GetFlags() const {return 0;}

	void*		m_lockData{ nullptr };
	int			m_stride{ 0 };
};