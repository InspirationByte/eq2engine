//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>

#include "renderers/IVertexBuffer.h"
#include "renderers/IIndexBuffer.h"
#include "renderers/IGPUBuffer.h"

class CWGPUBuffer : public IGPUBuffer
{
public:
	void		Init(const BufferInfo& bufferInfo, int wgpuUsage);

	int			GetSize() const { return m_bufSize; }

	void		Update(void* data, int size, int offset);
	LockFuture	Lock(int lockOfs, int sizeToLock, int flags);
	void		Unlock();

	WGPUBuffer	GetWGPUBuffer() const { return m_rhiBuffer; }

private:
	WGPUBuffer		m_rhiBuffer{ nullptr };
	int				m_bufSize{ 0 };
};

// OLD deprecated buffers - stubs for API compatibility

class CWGPUVertexBuffer : public IVertexBuffer
{
public:
	CWGPUVertexBuffer(const BufferInfo& bufferInfo);

	int				GetSizeInBytes() const { return m_bufElemCapacity * m_bufElemSize; }
	int				GetVertexCount() const { return m_bufElemCapacity; }
	int				GetStrideSize() const { return m_bufElemSize; }

	void			Update(void* data, int size, int offset = 0);
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void			Unlock();

	void			SetFlags(int flags) { m_flags = flags; }
	int				GetFlags() const { return m_flags; }

	WGPUBuffer		GetWGPUBuffer() const { return m_buffer.GetWGPUBuffer(); }

protected:
	BufferLockData	m_lockData;
	CWGPUBuffer		m_buffer;

	int				m_bufElemCapacity{ 0 };
	int				m_bufElemSize{ 0 };

	int				m_flags{ 0 };
};



class CWGPUIndexBuffer : public IIndexBuffer
{
public:
	CWGPUIndexBuffer(const BufferInfo& bufferInfo);

	int				GetSizeInBytes() const { return m_bufElemCapacity * m_bufElemSize; }
	int				GetIndicesCount() const { return m_bufElemCapacity; }
	int				GetIndexSize() const { return m_bufElemSize; }

	void			Update(void* data, int size, int offset = 0);
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void			Unlock();

	WGPUBuffer		GetWGPUBuffer() const { return m_buffer.GetWGPUBuffer(); }

protected:
	BufferLockData	m_lockData;
	CWGPUBuffer		m_buffer;

	int				m_bufElemCapacity{ 0 };
	int				m_bufElemSize{ 0 };
};