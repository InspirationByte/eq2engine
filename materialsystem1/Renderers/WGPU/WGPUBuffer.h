//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "WGPURenderDefs.h"
#include "renderers/IGPUBuffer.h"

struct BufferInfo;

class CWGPUBuffer : public IGPUBuffer
{
public:
	~CWGPUBuffer();
	CWGPUBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label = nullptr);

	int			GetSize() const { return m_bufSize; }

	void		Update(const void* data, int64 size, int64 offset);
	MapFuture	Lock(int lockOfs, int sizeToLock, int flags);
	void		Unlock();

	WGPUBuffer	GetWGPUBuffer() const { return m_rhiBuffer; }
	int			GetUsageFlags() const { return m_usageFlags; }

private:
	WGPUBuffer	m_rhiBuffer{ nullptr };
	int			m_bufSize{ 0 };
	int			m_usageFlags{ 0 };
};
