//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#include "renderers/ShaderAPI_defs.h"
#include "WGPUBuffer.h"
#include "WGPURenderAPI.h"

void CWGPUBuffer::Init(const BufferInfo& bufferInfo, int wgpuUsage)
{
	m_bufSize = bufferInfo.elementSize * bufferInfo.elementCapacity;

	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | wgpuUsage;
	desc.size = m_bufSize;
	desc.mappedAtCreation = false;
	if (bufferInfo.flags & BUFFER_FLAG_READ)
		desc.usage |= WGPUBufferUsage_MapRead;

	m_rhiBuffer = wgpuDeviceCreateBuffer(WGPURenderAPI::Instance.GetWGPUDevice(), &desc);

	if (bufferInfo.data && bufferInfo.dataSize)
		wgpuQueueWriteBuffer(WGPURenderAPI::Instance.GetWGPUQueue(), m_rhiBuffer, 0, bufferInfo.data, min(bufferInfo.dataSize, m_bufSize));
}

void CWGPUBuffer::Update(void* data, int size, int offset)
{
	// next commands in queue are not executed until data is uploded
	wgpuQueueWriteBuffer(WGPURenderAPI::Instance.GetWGPUQueue(), m_rhiBuffer, offset, data, min(size, m_bufSize));
}

Future<BufferLockData> CWGPUBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
{
	struct LockContext
	{
		Promise<BufferLockData> promise;
		BufferLockData data;
		WGPUBuffer buffer;
	};

	if (lockOfs < 0 || lockOfs + sizeToLock > m_bufSize)
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