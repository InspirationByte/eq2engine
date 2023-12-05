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


CWGPUBuffer::~CWGPUBuffer()
{
	Terminate();
}

void CWGPUBuffer::Init(const BufferInfo& bufferInfo, int wgpuUsage, const char* label)
{
	const int sizeInBytes = bufferInfo.elementSize * bufferInfo.elementCapacity;
	const int writeDataSize = (bufferInfo.dataSize + 3) & ~3;
	m_bufSize = (sizeInBytes + 3) & ~3;

	WGPUBufferDescriptor desc = {};
	desc.usage = wgpuUsage;
	desc.size = m_bufSize;
	desc.mappedAtCreation = bufferInfo.data && bufferInfo.dataSize;
	desc.label = label;

	m_usageFlags = desc.usage;
	m_rhiBuffer = wgpuDeviceCreateBuffer(CWGPURenderAPI::Instance.GetWGPUDevice(), &desc);

	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer");

	if (m_rhiBuffer && bufferInfo.data && bufferInfo.dataSize)
	{
		wgpuBufferReference(m_rhiBuffer);
		void* outData = wgpuBufferGetMappedRange(m_rhiBuffer, 0, m_bufSize);
		ASSERT_MSG(outData, "Buffer mapped range is NULL");
		memcpy(outData, bufferInfo.data, writeDataSize);

		g_renderWorker.Execute("UnmapBuffer", [buffer = m_rhiBuffer]() {
			wgpuBufferUnmap(buffer);
			wgpuBufferRelease(buffer);
			return 0;
		});
	}
}

void CWGPUBuffer::Terminate()
{
	wgpuBufferRelease(m_rhiBuffer);
	m_rhiBuffer = nullptr;
}

void CWGPUBuffer::Update(const void* data, int64 size, int64 offset)
{
	if (!m_rhiBuffer)
		return;

	if (!size)
		return;

	const int64 writeDataSize = (size + 3) & ~3;

	// next commands in queue are not executed until data is uploded
	g_renderWorker.WaitForExecute("UpdateBuffer", [&]() {
		wgpuQueueWriteBuffer(CWGPURenderAPI::Instance.GetWGPUQueue(), m_rhiBuffer, offset, data, writeDataSize);
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
