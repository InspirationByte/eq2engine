//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include <d3dx10.h>

#include "DebugInterface.h"
#include "VertexBufferD3DX10.h"

CVertexBufferD3DX10::CVertexBufferD3DX10()
{
	m_nSize = 0;
	m_pVertexBuffer = NULL;
	m_nUsage = D3D10_USAGE_DEFAULT;
	m_bIsLocked = false;
	m_nStrideSize = -1;
	m_nNumVertices = 0;
}

long CVertexBufferD3DX10::GetSizeInBytes()
{
	return m_nSize;
}

int CVertexBufferD3DX10::GetVertexCount()
{
	return m_nNumVertices;
}

int CVertexBufferD3DX10::GetStrideSize()
{
	return m_nStrideSize;
}

extern ID3D10Device* pXDevice;

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferD3DX10::Lock(int lockOfs, int vertexCount, void** outdata, bool readOnly)
{
	bool dynamic = (m_nUsage == D3D10_USAGE_DYNAMIC);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer already locked! (You must unlock it first!)");
		return false;
	}

	if(vertexCount > m_nNumVertices && !dynamic)
	{
		MsgError("Static vertex buffer is not resizable, must be less or equal %d (%d)\n", m_nNumVertices, vertexCount);
		ASSERT(!"Static vertex buffer is not resizable. Debug it!\n");
		return false;
	}

	int nLockByteCount = m_nStrideSize*vertexCount;

	D3D10_MAP mapType = D3D10_MAP_WRITE_DISCARD;

	if(!dynamic)
	{
		if(readOnly)
			mapType = D3D10_MAP_READ;
		else
			mapType = D3D10_MAP_WRITE;
	}

	if(m_pVertexBuffer->Map(mapType, 0, outdata) == S_OK)
	{
		// add the lock offset
		*((ubyte**)outdata) += m_nStrideSize*lockOfs;

		m_bIsLocked = true;
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
		m_nSize = nLockByteCount;
		m_nNumVertices = vertexCount;
	}

	return true;
}

// unlocks buffer
void CVertexBufferD3DX10::Unlock()
{
	if(m_bIsLocked)
		m_pVertexBuffer->Unmap();
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}