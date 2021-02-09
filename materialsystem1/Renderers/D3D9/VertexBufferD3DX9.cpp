//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration//////////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>

#include "DebugInterface.h"
#include "VertexBufferD3DX9.h"

#include "ShaderAPID3DX9.h"

extern ShaderAPID3DX9 s_shaderApi;

CVertexBufferD3DX9::CVertexBufferD3DX9()
{
	m_nSize = 0;
	m_pVertexBuffer = NULL;
	m_nUsage = 0;
	m_bIsLocked = false;
	m_bLockFail = false;
	m_nStrideSize = -1;
	m_nNumVertices = 0;

	m_flags = 0;

	m_nInitialSize = 0;

	m_pRestore = NULL;
}

CVertexBufferD3DX9::~CVertexBufferD3DX9()
{
	if (m_pVertexBuffer)
		m_pVertexBuffer->Release();
}

void CVertexBufferD3DX9::ReleaseForRestoration()
{
	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	// dynamic VBO's uses a D3DPOOL_DEFAULT
	if(dynamic)
	{
		m_pRestore = new vborestoredata_t;

		int lock_size = m_nStrideSize*m_nNumVertices;
		m_pRestore->size = lock_size;

		m_pRestore->data = PPAlloc(lock_size);

		void *src = NULL;

		if(m_pVertexBuffer->Lock(0, lock_size, &src, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK ) == D3D_OK)
		{
			memcpy(m_pRestore->data, src, m_pRestore->size);

			m_pVertexBuffer->Unlock();
			m_pVertexBuffer->Release();

			m_pVertexBuffer = NULL;
		}
		else
		{
			ErrorMsg("Vertex buffer restoration failed.\n");
			exit(0);
		}

	}
}

void CVertexBufferD3DX9::Restore()
{
	if(!m_pRestore)
		return; // nothing to restore

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (s_shaderApi.m_pD3DDevice->CreateVertexBuffer(
		m_nInitialSize, m_nUsage, 0, dynamic? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_pVertexBuffer, NULL) != D3D_OK)
	{
		ErrorMsg("Vertex buffer restoration failed on creation\n");
		exit(0);
	}

	void *dest = NULL;

	if(m_pVertexBuffer->Lock(0, m_nInitialSize, &dest, dynamic? D3DLOCK_DISCARD : 0 ) == D3D_OK)
	{
		memcpy(dest, m_pRestore->data, m_pRestore->size);
		m_pVertexBuffer->Unlock();
	}

	PPFree(m_pRestore->data);
	delete m_pRestore;
	m_pRestore = NULL;
}

long CVertexBufferD3DX9::GetSizeInBytes()
{
	return m_nSize;
}

int CVertexBufferD3DX9::GetVertexCount()
{
	return m_nNumVertices;
}

int CVertexBufferD3DX9::GetStrideSize()
{
	return m_nStrideSize;
}

// updates buffer without map/unmap operations which are slower
void CVertexBufferD3DX9::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
		return;

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) > 0;

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer can't be updated while locked!");
		return;
	}

	if(offset+size > m_nNumVertices && !dynamic)
	{
		ASSERT(!"Update() with bigger size cannot be used on static vertex buffer!");
		return;
	}

	int nLockByteCount = size*m_nStrideSize;

	void* outData = NULL;

	if(m_pVertexBuffer->Lock(offset*m_nStrideSize, nLockByteCount, &outData, (dynamic ? D3DLOCK_DISCARD : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
	{
		memcpy(outData, data, nLockByteCount);
		m_pVertexBuffer->Unlock();

		if(dynamic && discard && offset == 0)
			m_nNumVertices = size;
	}
}

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferD3DX9::Lock(int lockOfs, int vertexCount, void** outdata, bool readOnly)
{
	HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
	{
		//m_bLockFail = true;
		//m_bIsLocked = true;
		return false;
	}

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

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

	if(m_pVertexBuffer->Lock(lockOfs*m_nStrideSize, nLockByteCount, outdata, (dynamic ? D3DLOCK_DISCARD : 0) | (readOnly ? D3DLOCK_READONLY : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
	{
		m_bIsLocked = true;
	}
	else
	{
		ASSERT(!"Couldn't lock vertex buffer!");
		return false;
	}

	if(dynamic)
	{
		m_nSize = nLockByteCount;
		m_nNumVertices = vertexCount;
	}

	
	//m_nStrideSize = nStrideSize;

	return true;
}

// unlocks buffer
void CVertexBufferD3DX9::Unlock()
{
	if(m_bIsLocked)
	{
		if(!m_bLockFail)
			m_pVertexBuffer->Unlock();

		m_bLockFail = false;
	}
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}