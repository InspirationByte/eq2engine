//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 index buffer impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IIndexBuffer.h"

class CIndexBufferD3DX10 : public IIndexBuffer
{
	friend class			ShaderAPID3DX10;
public:
	~CIndexBufferD3DX10();


	int						GetSizeInBytes() const { return m_indexSize * m_numIndices; }
	int						GetIndexSize() const { return m_indexSize; }
	int						GetIndicesCount() const { return m_numIndices; }

	// updates buffer without map/unmap operations which are slower
	void					Update(void* data, int size, int offset, bool discard = true);

	// locks index buffer and gives to programmer buffer data
	bool					Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void					Unlock();

protected:
	ID3D10Buffer*			m_buffer{ nullptr };
	D3D10_USAGE				m_usage{ D3D10_USAGE_DEFAULT };

	int						m_numIndices{ 0 };
	int						m_indexSize{ 0 };
	bool					m_isLocked{ false };
};