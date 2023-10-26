//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "renderers/ShaderAPI_defs.h"
#include "../RenderWorker.h"
#include "WGPUBuffer.h"
#include "WGPURenderAPI.h"

void CWGPUBuffer::Init(const BufferInfo& bufferInfo, int wgpuUsage)
{
	const int sizeInBytes = bufferInfo.elementSize * bufferInfo.elementCapacity;
	const int writeDataSize = (bufferInfo.dataSize + 3) & ~3;
	m_bufSize = (sizeInBytes + 3) & ~3;

	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | wgpuUsage;
	desc.size = m_bufSize;
	desc.mappedAtCreation = false;
	if (bufferInfo.flags & BUFFER_FLAG_READ)
		desc.usage |= WGPUBufferUsage_MapRead;

	m_rhiBuffer = wgpuDeviceCreateBuffer(WGPURenderAPI::Instance.GetWGPUDevice(), &desc);

	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer");

	if (m_rhiBuffer && bufferInfo.data && bufferInfo.dataSize)
	{
		const void* data = bufferInfo.data;
		g_renderWorker.WaitForExecute("InitBuffer", [&]() {
			wgpuQueueWriteBuffer(WGPURenderAPI::Instance.GetWGPUQueue(), m_rhiBuffer, 0, data, writeDataSize);
			return 0;
		});
	}
}

void CWGPUBuffer::Update(void* data, int size, int offset)
{
	if (!m_rhiBuffer)
		return;

	const int writeDataSize = (size + 3) & ~3;

	// next commands in queue are not executed until data is uploded
	g_renderWorker.WaitForExecute("UpdateBuffer", [&]() {
		wgpuQueueWriteBuffer(WGPURenderAPI::Instance.GetWGPUQueue(), m_rhiBuffer, offset, data, writeDataSize);
		return 0;
	});
}

Future<BufferLockData> CWGPUBuffer::Lock(int lockOfs, int sizeToLock, int flags)
{
	struct LockContext
	{
		Promise<BufferLockData> promise;
		BufferLockData data;
		WGPUBuffer buffer;
	};

	if (!m_rhiBuffer || lockOfs < 0 || lockOfs + sizeToLock > m_bufSize)
	{
		ASSERT_FAIL("Locking outside range");
		Promise<BufferLockData> errorPromise;
		errorPromise.SetError(-1, "Lock failure");
		return errorPromise.CreateFuture();
	}

	LockContext* context = PPNew LockContext;
	context->buffer = m_rhiBuffer;

	BufferLockData& lockData = context->data;
	lockData.flags = flags;
	lockData.size = sizeToLock;
	lockData.offset = lockOfs;

	auto callback = [](WGPUBufferMapAsyncStatus status, void* userdata) {
		LockContext* context = reinterpret_cast<LockContext*>(userdata);
		defer{
			delete context;
		};

		if (status != WGPUBufferMapAsyncStatus_Success)
		{
			context->promise.SetError(status, "Failed to map buffer");
			return;
		}

		BufferLockData& lockData = context->data;
		lockData.data = wgpuBufferGetMappedRange(context->buffer, lockData.offset, lockData.size);
		context->promise.SetResult(std::move(context->data));
	};
	wgpuBufferMapAsync(m_rhiBuffer, WGPUMapMode_Read, lockOfs, sizeToLock, callback, context);

	return context->promise.CreateFuture();
}

void CWGPUBuffer::Unlock()
{
	wgpuBufferUnmap(m_rhiBuffer);
}

//-----------------------------------------------------

CWGPUVertexBuffer::CWGPUVertexBuffer(const BufferInfo& bufferInfo)
	: m_bufElemSize(bufferInfo.elementSize), m_bufElemCapacity(bufferInfo.elementCapacity)
{
	m_buffer.Init(bufferInfo, WGPUBufferUsage_Vertex);
}

void CWGPUVertexBuffer::Update(void* data, int size, int offset)
{
	const int ofsBytes = offset * m_bufElemSize;
	const int sizeBytes = size * m_bufElemSize;

	m_buffer.Update(data, sizeBytes, ofsBytes);
}

bool CWGPUVertexBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
{
	const int lockOfsBytes = lockOfs * m_bufElemSize;
	const int lockSizeBytes = sizeToLock * m_bufElemSize;

	CWGPUBuffer::LockFuture lockFuture = m_buffer.Lock(lockOfsBytes, lockSizeBytes, flags);

	lockFuture.AddCallback([this](FutureResult<BufferLockData> lockData) {
		if (lockData.IsError())
			return;
		m_lockData = *lockData;
	});
	lockFuture.Wait();
	return m_lockData.data != nullptr;
}

void CWGPUVertexBuffer::Unlock()
{
	ASSERT_MSG(m_lockData.data, "Buffer is not locked");
	m_buffer.Unlock();
}

//-----------------------------------------------------

CWGPUIndexBuffer::CWGPUIndexBuffer(const BufferInfo& bufferInfo)
	: m_bufElemSize(bufferInfo.elementSize), m_bufElemCapacity(bufferInfo.elementCapacity)
{
	m_buffer.Init(bufferInfo, WGPUBufferUsage_Index);
}

void CWGPUIndexBuffer::Update(void* data, int size, int offset)
{
	const int ofsBytes = offset * m_bufElemSize;
	const int sizeBytes = size * m_bufElemSize;

	m_buffer.Update(data, sizeBytes, ofsBytes);
}

bool CWGPUIndexBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
{
	const int lockOfsBytes = lockOfs * m_bufElemSize;
	const int lockSizeBytes = sizeToLock * m_bufElemSize;

	CWGPUBuffer::LockFuture lockFuture = m_buffer.Lock(lockOfsBytes, lockSizeBytes, flags);

	lockFuture.AddCallback([this](FutureResult<BufferLockData> lockData) {
		if (lockData.IsError())
			return;
		m_lockData = *lockData;
		});
	lockFuture.Wait();
	return m_lockData.data != nullptr;
}

void CWGPUIndexBuffer::Unlock()
{
	ASSERT_MSG(m_lockData.data, "Buffer is not locked");
	m_buffer.Unlock();
}