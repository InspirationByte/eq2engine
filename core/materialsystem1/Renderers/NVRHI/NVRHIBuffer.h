//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "NVRHIRenderDefs.h"
#include "renderers/IGPUBuffer.h"
#include "ResourcePool.h"

struct BufferInfo;

class CNVRHIBuffer : public IGPUBuffer
{
public:
	DECLARE_RENDER_RESOURCE(CNVRHIBuffer);
	
	static nvrhi::ResourceStates	GetNVRHIResourceStates(int usageFlags);

	~CNVRHIBuffer();
	CNVRHIBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label);

	int64					GetSize() const { return m_bufSize; }

	void					Update(const void* data, int64 size, int64 offset);
	MapFuture				Lock(int lockOfs, int sizeToLock, int flags);
	void					Unlock();

	nvrhi::IBuffer*			GetNVRHIBufferHandle() const { return m_rhiBuffer; }

	bool					IsNeedsTrackingState() const { return m_needsTrackingState; }
	void					OnUpdated();

	int						GetUsageFlags() const { return m_usageFlags; }

	const char*				GetDbgName() const { return m_dbgName; }
private:
	EqString	m_dbgName;

	nvrhi::BufferHandle	m_rhiBuffer{ nullptr };
	int64		m_bufSize{ 0 };
	int			m_usageFlags{ 0 };
	int			m_transientHeapIdx{ -1 };
	bool		m_isLocked{ false };
	bool		m_needsTrackingState{ true };
};
