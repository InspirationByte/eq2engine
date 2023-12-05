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

struct BufferInfo;

class CWGPUBuffer : public IGPUBuffer
{
public:
	~CWGPUBuffer();

	void		Init(const BufferInfo& bufferInfo, int wgpuUsage, const char* label = nullptr);
	void		Terminate();

	int			GetSize() const { return m_bufSize; }

	void		Update(const void* data, int64 size, int64 offset);
	LockFuture	Lock(int lockOfs, int sizeToLock, int flags);
	void		Unlock();

	WGPUBuffer	GetWGPUBuffer() const { return m_rhiBuffer; }
	int			GetUsageFlags() const { return m_usageFlags; }

private:
	WGPUBuffer	m_rhiBuffer{ nullptr };
	int			m_bufSize{ 0 };
	int			m_usageFlags{ 0 };
};
