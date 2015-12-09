//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration//////////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>
#include <d3dx9.h>

#include "DebugInterface.h"
#include "IndexBufferD3DX9.h"

CIndexBufferD3DX9::CIndexBufferD3DX9()
{
	m_nIndices = 0;
	m_nIndexSize = 0;

	m_nInitialSize = 0;

	m_nUsage = 0;
	m_pIndexBuffer = 0;
	m_bIsLocked = false;
	m_bLockFail = false;

	m_pRestore	= NULL;
}

void CIndexBufferD3DX9::ReleaseForRestoration()
{
	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	// dynamic VBO's uses a D3DPOOL_DEFAULT
	if(dynamic)
	{
		m_pRestore = new iborestoredata_t;

		int lock_size = m_nIndices*m_nIndexSize;
		m_pRestore->size = lock_size;

		m_pRestore->data = PPAlloc(lock_size);

		void *src = NULL;

		if(m_pIndexBuffer->Lock(0, lock_size, &src, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK ) == D3D_OK)
		{
			memcpy(m_pRestore->data, src, m_pRestore->size);
			m_pIndexBuffer->Unlock();
			m_pIndexBuffer->Release();

			m_pIndexBuffer = NULL;
		}
		else
		{
			ErrorMsg("Index buffer restoration failed.\n");
			exit(0);
		}

	}
}

extern LPDIRECT3DDEVICE9 pXDevice;

void CIndexBufferD3DX9::Restore()
{
	if(!m_pRestore)
		return; // nothing to restore

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (pXDevice->CreateIndexBuffer(m_nInitialSize, m_nUsage, m_nIndexSize == 2? D3DFMT_INDEX16 : D3DFMT_INDEX32, dynamic? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_pIndexBuffer, NULL) != D3D_OK)
	{
		ErrorMsg("Index buffer restoration failed on creation\n");
		exit(0);
	}

	void *dest = NULL;

	if(m_pIndexBuffer->Lock(0, m_nInitialSize, &dest, dynamic? D3DLOCK_DISCARD : 0 ) == D3D_OK)
	{
		memcpy(dest, m_pRestore->data, m_pRestore->size);
		m_pIndexBuffer->Unlock();
	}

	PPFree(m_pRestore->data);
	delete m_pRestore;
}

int8 CIndexBufferD3DX9::GetIndexSize()
{
	return m_nIndexSize;
}

int CIndexBufferD3DX9::GetIndicesCount()
{
	return m_nIndices;
}

extern LPDIRECT3DDEVICE9 pXDevice;

// locks vertex buffer and gives to programmer buffer data
bool CIndexBufferD3DX9::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	HRESULT hr = pXDevice->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
	{
		//m_bLockFail = true;
		//m_bIsLocked = true;
		return false;
	}

	if(m_bIsLocked)
	{
		ASSERT(!"Index buffer already locked! (You must unlock it first!)");
		return false;
	}

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if(sizeToLock > m_nIndices && !dynamic)
	{
		MsgError("Static vertex buffer is not resizable, must be less or equal %d (%d)\n", m_nIndices, sizeToLock);
		ASSERT(!"Static vertex buffer is not resizable. Debug it!\n");
		return false;
	}

	//Msg("EQIB 'this' ptr: %p\n", this);

	if(m_pIndexBuffer->Lock(lockOfs*m_nIndexSize, sizeToLock*m_nIndexSize, outdata, (dynamic ? D3DLOCK_DISCARD : 0) | (readOnly ? D3DLOCK_READONLY : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
	{
		m_bIsLocked = true;
	}
	else 
	{
		ASSERT(!"Couldn't lock index buffer!");
		return false;
	}

	if(dynamic)
	{
		m_nIndices = sizeToLock;
	}

	return true;
}

// unlocks buffer
void CIndexBufferD3DX9::Unlock()
{
	if(m_bIsLocked)
	{
		if(!m_bLockFail)
			m_pIndexBuffer->Unlock();

		m_bLockFail = false;
	}
	else
		ASSERT(!"Index buffer is not locked!");

	m_bIsLocked = false;
}