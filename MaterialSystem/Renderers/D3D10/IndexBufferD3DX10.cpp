//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include <d3dx10.h>

#include "DebugInterface.h"
#include "IndexBufferD3DX10.h"

CIndexBufferD3DX10::CIndexBufferD3DX10()
{
	m_nIndices = 0;
	m_nIndexSize = 0;
	m_pIndexBuffer = 0;
	m_bIsLocked = false;

	m_nUsage = D3D10_USAGE_DEFAULT;
}

int8 CIndexBufferD3DX10::GetIndexSize()
{
	return m_nIndexSize;
}

int CIndexBufferD3DX10::GetIndicesCount()
{
	return m_nIndices;
}

extern ID3D10Device* pXDevice;

// locks vertex buffer and gives to programmer buffer data
bool CIndexBufferD3DX10::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	bool dynamic = (m_nUsage == D3D10_USAGE_DYNAMIC);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer already locked! (You must unlock it first!)");
		return false;
	}

	if(sizeToLock > m_nIndices && !dynamic)
	{
		MsgError("Static index buffer is not resizable, must be less or equal %d (%d)\n", m_nIndices, sizeToLock);
		ASSERT(!"Static index buffer is not resizable. Debug it!\n");
		return false;
	}

	int nLockByteCount = m_nIndexSize*sizeToLock;

	D3D10_MAP mapType = D3D10_MAP_WRITE_DISCARD;

	if(!dynamic)
	{
		if(readOnly)
			mapType = D3D10_MAP_READ;
		else
			mapType = D3D10_MAP_WRITE;
	}

	if(m_pIndexBuffer->Map(mapType, 0, outdata) == S_OK)
	{
		// add the lock offset
		*((ubyte**)outdata) += m_nIndexSize*lockOfs;

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
		//elemCount = nLockByteCount;
		m_nIndices = sizeToLock;
	}

	return true;
}

// unlocks buffer
void CIndexBufferD3DX10::Unlock()
{
	if(m_bIsLocked)
		m_pIndexBuffer->Unmap();
	else
		ASSERT(!"Index buffer is not locked!");

	m_bIsLocked = false;
}