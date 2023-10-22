#pragma once
#include "renderers/IVertexBuffer.h"
#include "renderers/IIndexBuffer.h"

class CWGPUVertexBuffer : public IVertexBuffer
{
public:
	CWGPUVertexBuffer(int stride)
		: m_lockData(nullptr), m_stride(stride) {}

	// returns size in bytes
	int			GetSizeInBytes() const { return 0; }

	// returns vertex count
	int			GetVertexCount() const { return 0; }

	// retuns stride size
	int			GetStrideSize() const { return m_stride; }

	// updates buffer without map/unmap operations which are slower
	void		Update(void* data, int size, int offset, bool discard = true)
	{
	}

	// locks vertex buffer and gives to programmer buffer data
	bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
	{
		*outdata = PPAlloc(sizeToLock*m_stride);
		return true;
	}

	// unlocks buffer
	void		Unlock() {PPFree(m_lockData); m_lockData = nullptr;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() const {return 0;}

	void*		m_lockData;
	int			m_stride;
};

class CWGPUIndexBuffer : public IIndexBuffer
{
public:
	CWGPUIndexBuffer(int stride)
		: m_lockData(nullptr), m_stride(stride) {}

	int			GetSizeInBytes() const { return 0; }

	// returns index size
	int			GetIndexSize() const {return m_stride;}

	// returns index count
	int			GetIndicesCount()  const {return 0;}

	// updates buffer without map/unmap operations which are slower
	void		Update(void* data, int size, int offset, bool discard = true)
	{
	}

	// locks vertex buffer and gives to programmer buffer data
	bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
	{
		*outdata = malloc(sizeToLock*m_stride);
		return true;
	}

	// unlocks buffer
	void		Unlock() {free(m_lockData); m_lockData = nullptr;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() const {return 0;}

	void*		m_lockData;
	int			m_stride;
};