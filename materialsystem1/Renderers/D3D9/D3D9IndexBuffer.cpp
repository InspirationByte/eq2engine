//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9IndexBuffer.h"

#include "ShaderAPID3D9.h"

extern ShaderAPID3D9 s_shaderApi;

CD3D9IndexBuffer::CD3D9IndexBuffer()
{
	m_nIndices = 0;
	m_nIndexSize = 0;

	m_nInitialSize = 0;

	m_nUsage = 0;
	m_pIndexBuffer = 0;
	m_bIsLocked = false;
	m_bLockFail = false;
}

CD3D9IndexBuffer::~CD3D9IndexBuffer()
{
	if (m_pIndexBuffer)
		m_pIndexBuffer->Release();
}

void CD3D9IndexBuffer::ReleaseForRestoration()
{
	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	// dynamic VBO's uses a D3DPOOL_DEFAULT
	if(dynamic)
	{
		m_restore = PPNew IBRestoreData;

		int lock_size = m_nIndices*m_nIndexSize;
		m_restore->size = lock_size;
		m_restore->data = PPAlloc(lock_size);

		void *src = nullptr;

		if(m_pIndexBuffer->Lock(0, lock_size, &src, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK ) == D3D_OK)
		{
			memcpy(m_restore->data, src, m_restore->size);
			m_pIndexBuffer->Unlock();
			m_pIndexBuffer->Release();

			m_pIndexBuffer = nullptr;
		}
		else
		{
			ErrorMsg("Index buffer restoration failed.\n");
			exit(0);
		}

	}
}

void CD3D9IndexBuffer::Restore()
{
	if(!m_restore)
		return; // nothing to restore

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (s_shaderApi.m_pD3DDevice->CreateIndexBuffer(
		m_nInitialSize, m_nUsage, 
		m_nIndexSize == 2? D3DFMT_INDEX16 : D3DFMT_INDEX32, 
		dynamic? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_pIndexBuffer, nullptr) != D3D_OK)
	{
		ErrorMsg("Index buffer restoration failed on creation\n");
		exit(0);
	}

	void *dest = nullptr;

	if(m_pIndexBuffer->Lock(0, m_nInitialSize, &dest, dynamic? D3DLOCK_DISCARD : 0 ) == D3D_OK)
	{
		memcpy(dest, m_restore->data, m_restore->size);
		m_pIndexBuffer->Unlock();
	}

	PPFree(m_restore->data);
	delete m_restore;
}

int CD3D9IndexBuffer::GetSizeInBytes() const
{
	return m_nIndexSize * m_nIndices;
}

int CD3D9IndexBuffer::GetIndexSize() const
{
	return m_nIndexSize;
}

int CD3D9IndexBuffer::GetIndicesCount() const
{
	return m_nIndices;
}

// updates buffer without map/unmap operations which are slower
void CD3D9IndexBuffer::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
		return;

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer can't be updated while locked!");
		return;
	}

	if(offset+size > m_nIndices && !dynamic)
	{
		ASSERT(!"Update() with bigger size cannot be used on static vertex buffer!");
		return;
	}

	int nLockByteCount = size*m_nIndexSize;

	void* outData = nullptr;

	if(m_pIndexBuffer->Lock(offset*m_nIndexSize, nLockByteCount, &outData, (dynamic ? D3DLOCK_DISCARD : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
	{
		memcpy(outData, data, nLockByteCount);
		m_pIndexBuffer->Unlock();

		if(dynamic && discard && offset == 0)
			m_nIndices = size;
	}
}

// locks vertex buffer and gives to programmer buffer data
bool CD3D9IndexBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

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
void CD3D9IndexBuffer::Unlock()
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