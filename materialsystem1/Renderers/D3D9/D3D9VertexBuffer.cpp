//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9VertexBuffer.h"

#include "ShaderAPID3D9.h"

extern ShaderAPID3D9 s_shaderApi;

CD3D9VertexBuffer::CD3D9VertexBuffer()
{
	m_nSize = 0;
	m_pVertexBuffer = nullptr;
	m_nUsage = 0;
	m_bIsLocked = false;
	m_nStrideSize = -1;
	m_nNumVertices = 0;

	m_flags = 0;

	m_nInitialSize = 0;
}

CD3D9VertexBuffer::~CD3D9VertexBuffer()
{
	if (m_pVertexBuffer)
		m_pVertexBuffer->Release();
}

void CD3D9VertexBuffer::ReleaseForRestoration()
{
	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	// dynamic VBO's uses a D3DPOOL_DEFAULT
	if(dynamic)
	{
		m_restore = PPNew VBRestoreData;

		int lock_size = m_nStrideSize*m_nNumVertices;
		m_restore->size = lock_size;
		m_restore->data = PPAlloc(lock_size);

		void *src = nullptr;

		if(m_pVertexBuffer->Lock(0, lock_size, &src, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK ) == D3D_OK)
		{
			memcpy(m_restore->data, src, m_restore->size);

			m_pVertexBuffer->Unlock();
			m_pVertexBuffer->Release();

			m_pVertexBuffer = nullptr;
		}
		else
		{
			ErrorMsg("Vertex buffer restoration failed.\n");
			exit(0);
		}

	}
}

void CD3D9VertexBuffer::Restore()
{
	if(!m_restore)
		return; // nothing to restore

	bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (s_shaderApi.m_pD3DDevice->CreateVertexBuffer(
		m_nInitialSize, m_nUsage, 0, dynamic? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_pVertexBuffer, nullptr) != D3D_OK)
	{
		ErrorMsg("Vertex buffer restoration failed on creation\n");
		exit(0);
	}

	void *dest = nullptr;

	if(m_pVertexBuffer->Lock(0, m_nInitialSize, &dest, dynamic? D3DLOCK_DISCARD : 0 ) == D3D_OK)
	{
		memcpy(dest, m_restore->data, m_restore->size);
		m_pVertexBuffer->Unlock();
	}

	PPFree(m_restore->data);
	delete m_restore;
	m_restore = nullptr;
}

int CD3D9VertexBuffer::GetSizeInBytes() const
{
	return m_nSize;
}

int CD3D9VertexBuffer::GetVertexCount() const
{
	return m_nNumVertices;
}

int CD3D9VertexBuffer::GetStrideSize() const
{
	return m_nStrideSize;
}

// updates buffer without map/unmap operations which are slower
void CD3D9VertexBuffer::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	{
		const HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
			return;
	}

	const bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC);

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

	const int lockOffset = offset * m_nStrideSize;
	const int lockSize = size * m_nStrideSize;

	void* outData = nullptr;
	if(m_pVertexBuffer->Lock(lockOffset, lockSize, &outData, (dynamic ? D3DLOCK_DISCARD : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
	{
		memcpy(outData, data, lockSize);
		m_pVertexBuffer->Unlock();

		if(dynamic && discard && offset == 0)
			m_nNumVertices = size;
	}
}

// locks vertex buffer and gives to programmer buffer data
bool CD3D9VertexBuffer::Lock(int lockOfs, int vertexCount, void** outdata, bool readOnly)
{
	{
		const HRESULT hr = s_shaderApi.m_pD3DDevice->TestCooperativeLevel();

		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
			return false;
	}

	const bool dynamic = (m_nUsage & D3DUSAGE_DYNAMIC) != 0;

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

	const int lockOffset = lockOfs * m_nStrideSize;
	const int lockSize = vertexCount * m_nStrideSize;

	if(m_pVertexBuffer->Lock(lockOfs*m_nStrideSize, lockSize, outdata, (dynamic ? D3DLOCK_DISCARD : 0) | (readOnly ? D3DLOCK_READONLY : 0) | D3DLOCK_NOSYSLOCK ) == D3D_OK)
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
		m_nSize = lockSize;
		m_nNumVertices = vertexCount;
	}

	return true;
}

// unlocks buffer
void CD3D9VertexBuffer::Unlock()
{
	if(m_bIsLocked)
		m_pVertexBuffer->Unlock();
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}