//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>

#include "core/core_common.h"
#include "D3D11IndexBuffer.h"

CIndexBufferD3DX10::~CIndexBufferD3DX10()
{
	if (m_buffer)
		m_buffer->Release();
}

// updates buffer without map/unmap operations which are slower
void CIndexBufferD3DX10::Update(void* data, int size, int offset, bool discard)
{
	ASSERT_FAIL("Unimplemented");
}

// locks vertex buffer and gives to programmer buffer data
bool CIndexBufferD3DX10::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	const bool dynamic = (m_usage == D3D10_USAGE_DYNAMIC);
	if(m_isLocked)
	{
		ASSERT_FAIL("Vertex buffer already locked");
		return false;
	}

	if(sizeToLock > m_numIndices && !dynamic)
	{
		MsgError("Static index buffer is not resizable, must be less or equal %d (%d)\n", m_numIndices, sizeToLock);
		ASSERT_FAIL("Static index buffer is not resizable");
		return false;
	}

	const int nLockByteCount = m_indexSize * sizeToLock;

	D3D10_MAP mapType = D3D10_MAP_WRITE_DISCARD;
	if(!dynamic)
	{
		if(readOnly)
			mapType = D3D10_MAP_READ;
		else
			mapType = D3D10_MAP_WRITE;
	}

	if(m_buffer->Map(mapType, 0, outdata) == S_OK)
	{
		// add the lock offset
		*((ubyte**)outdata) += m_indexSize * lockOfs;
		m_isLocked = true;
	}
	else
	{
		if(!dynamic && !readOnly)
			ASSERT(!"D3D10 can't map D3D10_USAGE_DEFAULT buffer with D3D10_MAP_WRITE_DISCARD!");

		ASSERT(!"Couldn't lock vertex buffer!");
		return false;
	}

	if(dynamic)
		m_numIndices = sizeToLock;

	return true;
}

// unlocks buffer
void CIndexBufferD3DX10::Unlock()
{
	if(m_isLocked)
		m_buffer->Unmap();
	else
		ASSERT(!"Index buffer is not locked!");

	m_isLocked = false;
}