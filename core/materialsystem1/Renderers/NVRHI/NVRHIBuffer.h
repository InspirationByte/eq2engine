//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "NVRHIRenderDefs.h"
#include "renderers/IGPUBuffer.h"

struct BufferInfo;

class CNVRHIBuffer : public IGPUBuffer
{
public:
	~CNVRHIBuffer();
	CNVRHIBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label = nullptr);

	int			GetSize() const { return m_bufSize; }

	void		Update(const void* data, int64 size, int64 offset);
	MapFuture	Lock(int lockOfs, int sizeToLock, int flags);
	void		Unlock();

	nvrhi::BufferHandle		GetNVRHIBufferHandle() const { return m_rhiBuffer; }
	nvrhi::ResourceStates	GetNVRHIResourceStates() const;
	bool					IsFirstUpdate() const { return m_firstUpdate; }
	void					OnUpdated() { m_firstUpdate = false; }
	int						GetUsageFlags() const { return m_usageFlags; }

private:
	nvrhi::BufferHandle	m_rhiBuffer{ nullptr };
	int			m_bufSize{ 0 };
	int			m_usageFlags{ 0 };
	bool		m_isLocked{ false };
	bool		m_firstUpdate{ true };
};
