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

	int64		GetSize() const { return m_bufSize; }

	void		Update(const void* data, int64 size, int64 offset);
	MapFuture	Lock(int lockOfs, int sizeToLock, int flags);
	void		Unlock();

	nvrhi::IBuffer*			GetNVRHIBufferHandle() const { return m_rhiBuffer; }
	nvrhi::ResourceStates	GetNVRHIResourceStates() const;

	bool					IsNeedsTrackingState() const { return m_needsTrackingState; }
	void					OnUpdated();

	int						GetUsageFlags() const { return m_usageFlags; }

	const char*				GetDbgName() const { return m_dbgName; }
private:
	EqString	m_dbgName;

	nvrhi::BufferHandle	m_rhiBuffer{ nullptr };
	int64		m_bufSize{ 0 };
	int			m_usageFlags{ 0 };
	bool		m_isLocked{ false };
	bool		m_needsTrackingState{ true };
};
