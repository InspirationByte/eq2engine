//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 vertex buffer implementation
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>

#include "core/core_common.h"
#include "D3D11VertexBuffer.h"

CVertexBufferD3DX10::~CVertexBufferD3DX10()
{
	if (m_buffer)
		m_buffer->Release();
}

int CVertexBufferD3DX10::GetSizeInBytes() const
{
	return m_size;
}

int CVertexBufferD3DX10::GetVertexCount() const
{
	return m_numVertices;
}

int CVertexBufferD3DX10::GetStrideSize() const
{
	return m_strideSize;
}

extern ID3D10Device* pXDevice;

// updates buffer without map/unmap operations which are slower
void CVertexBufferD3DX10::Update(void* data, int size, int offset, bool discard)
{
	ASSERT_FAIL("UNIMPLEMENTED");
}

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferD3DX10::Lock(int lockOfs, int vertexCount, void** outdata, bool readOnly)
{
	const bool dynamic = (m_usage == D3D10_USAGE_DYNAMIC);

	if(m_isLocked)
	{
		ASSERT(!"Vertex buffer already locked! (You must unlock it first!)");
		return false;
	}

	if(vertexCount > m_numVertices && !dynamic)
	{
		MsgError("Static vertex buffer is not resizable, must be less or equal %d (%d)\n", m_numVertices, vertexCount);
		ASSERT_FAIL("Static vertex buffer is not resizable. Debug it!\n");
		return false;
	}

	const int lockByteCount = m_strideSize*vertexCount;

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
		*((ubyte**)outdata) += m_strideSize*lockOfs;

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
	{
		m_size = lockByteCount;
		m_numVertices = vertexCount;
	}

	return true;
}

// unlocks buffer
void CVertexBufferD3DX10::Unlock()
{
	if(m_isLocked)
		m_buffer->Unmap();
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_isLocked = false;
}