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

extern ShaderAPID3D9 s_renderApi;

CD3D9VertexBuffer::RestoreData::~RestoreData()
{
	PPFree(data);
}

CD3D9VertexBuffer::~CD3D9VertexBuffer()
{
	SAFE_DELETE(m_restore);
	if (m_rhiBuffer)
		m_rhiBuffer->Release();
}

#pragma optimize("", off)

void CD3D9VertexBuffer::ReleaseForRestoration()
{
	if (m_restore)
		return;

	const bool dynamic = (m_bufUsage & D3DUSAGE_DYNAMIC) != 0;

	// dynamic VBO's uses a D3DPOOL_DEFAULT
	if (!dynamic)
		return;

	SAFE_DELETE(m_restore);
	m_restore = PPNew RestoreData;

	const int lockSize = m_bufElemSize * m_bufElemCapacity;
	m_restore->size = lockSize;
	m_restore->data = PPAlloc(lockSize);

	void* src = nullptr;
	if (FAILED(m_rhiBuffer->Lock(0, lockSize, &src, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK)))
	{
		SAFE_DELETE(m_restore);
		m_rhiBuffer->Release();
		m_rhiBuffer = nullptr;

		ASSERT_FAIL("Vertex buffer storage failed.");
		return;
	}

	memcpy(m_restore->data, src, m_restore->size);
	m_rhiBuffer->Unlock();

	m_rhiBuffer->Release();
	m_rhiBuffer = nullptr;
}

void CD3D9VertexBuffer::Restore()
{
	if (!m_restore)
		return;

	const bool dynamic = (m_bufUsage & D3DUSAGE_DYNAMIC) != 0;
	const D3DPOOL pool = dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
	if (FAILED(s_renderApi.m_pD3DDevice->CreateVertexBuffer(m_bufInitialSize, m_bufUsage, 0, pool, &m_rhiBuffer, nullptr)))
	{
		ASSERT_FAIL("Vertex buffer restoration failed.");
	}

	void* dest = nullptr;
	if (FAILED(m_rhiBuffer->Lock(0, m_restore->size, &dest, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK)))
	{
		m_rhiBuffer->Release();
		m_rhiBuffer = nullptr;
		SAFE_DELETE(m_restore);
		return;
	}

	memcpy(dest, m_restore->data, m_restore->size);
	m_rhiBuffer->Unlock();

	SAFE_DELETE(m_restore);
}

void CD3D9VertexBuffer::Update(void* data, int size, int offset)
{
	if (m_restore)
		return;

	{
		const HRESULT hr = s_renderApi.m_pD3DDevice->TestCooperativeLevel();
		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
			return;
	}

	const bool dynamic = (m_bufUsage & D3DUSAGE_DYNAMIC);

	if (m_lockFlags)
	{
		ASSERT_FAIL("Buffer can't be updated while locked!");
		return;
	}

	if (offset < 0 || !dynamic && offset + size > m_bufElemCapacity)
	{
		ASSERT_FAIL("Update() with bigger size cannot be used on static buffer!");
		return;
	}

	const int lockByteCount = size * m_bufElemSize;
	const int lockByteOffset = offset * m_bufElemSize;

	const DWORD lockFlags = D3DLOCK_NOSYSLOCK
		| (lockByteOffset ? 0 : D3DLOCK_DISCARD);

	void* outData = nullptr;
	if (FAILED(m_rhiBuffer->Lock(lockByteOffset, lockByteCount, &outData, lockFlags)))
		return;

	memcpy(outData, data, lockByteCount);
	m_rhiBuffer->Unlock();

	if (dynamic && offset == 0)
		m_bufElemCapacity = offset + size;
}

// locks vertex buffer and gives to programmer buffer data
bool CD3D9VertexBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
{
	ASSERT_MSG(flags != 0, "Lock flags are invalid - specify flags from BUFFER_FLAGS");

	if (m_restore)
		return false;

	{
		const HRESULT hr = s_renderApi.m_pD3DDevice->TestCooperativeLevel();
		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
			return false;
	}

	const bool dynamic = (m_bufUsage & D3DUSAGE_DYNAMIC);
	const bool write = (flags & BUFFER_FLAG_WRITE);
	const bool readBack = (flags & BUFFER_FLAG_READ);
	const int lockByteCount = sizeToLock * m_bufElemSize;
	const int lockByteOffset = lockOfs * m_bufElemSize;

	if(m_lockFlags)
	{
		ASSERT_FAIL("Vertex buffer already locked! (You must unlock it first!)");
		return false;
	}

	if (lockOfs < 0 || (!dynamic && write || readBack) && lockOfs + sizeToLock > m_bufElemCapacity)
	{
		ASSERT_FAIL("locking outside buffer size range");
		return false;
	}

	const int lockOffset = lockOfs * m_bufElemSize;
	const int lockSize = sizeToLock * m_bufElemSize;

	const DWORD lockFlags = D3DLOCK_NOSYSLOCK
		| (readBack ? 0 : D3DLOCK_DISCARD)
		| (write ? 0 : D3DLOCK_READONLY);

	if(FAILED(m_rhiBuffer->Lock(lockOffset, lockSize, outdata, lockFlags)))
	{
		ASSERT_FAIL("Couldn't lock vertex buffer!");
		return false;
	}

	if (dynamic && write)
		m_bufElemCapacity = lockOfs + sizeToLock;

	return true;
}

// unlocks buffer
void CD3D9VertexBuffer::Unlock()
{
	if (!m_lockFlags)
	{
		ASSERT_FAIL("Vertex buffer is not locked!");
		return;
	}

	m_rhiBuffer->Unlock();
	m_lockFlags = 0;
}