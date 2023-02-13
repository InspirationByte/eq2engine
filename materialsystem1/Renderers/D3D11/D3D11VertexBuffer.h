//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 vertex buffer implementation
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexBuffer.h"

class CVertexBufferD3DX10 : public IVertexBuffer
{
	friend class	ShaderAPID3DX10;
public:
	~CVertexBufferD3DX10();

	int				GetSizeInBytes() const;
	int				GetVertexCount() const;
	int				GetStrideSize() const;

	// updates buffer without map/unmap operations which are slower
	void			Update(void* data, int size, int offset, bool discard = true);

	// locks vertex buffer and gives to programmer buffer data
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void			Unlock();

	// sets vertex buffer flags
	void			SetFlags( int flags )	{ m_flags = flags; }
	int				GetFlags() const		{ return m_flags; }

protected:
	ID3D10Buffer*	m_buffer{ nullptr };
	int				m_flags{ 0 };
	int				m_size{ 0 };
	int				m_numVertices{ 0 };
	int				m_strideSize{ -1 };
	D3D10_USAGE		m_usage{ D3D10_USAGE_DEFAULT };

	bool			m_isLocked{ false };
};
