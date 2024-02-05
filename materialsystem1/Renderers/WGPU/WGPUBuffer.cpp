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
	const bool hasData = bufferInfo.data && bufferInfo.dataSize;
	m_bufSize = (sizeInBytes + 3) & ~3;

	WGPUBufferDescriptor desc = {};
	desc.usage = wgpuUsage;
	desc.size = m_bufSize;
	desc.mappedAtCreation = true; // TEMPORARY FIX FOR VULKAN ON NVIDIA
	// desc.mappedAtCreation = hasData;
	desc.label = label;

	m_usageFlags = desc.usage;
	m_rhiBuffer = wgpuDeviceCreateBuffer(CWGPURenderAPI::Instance.GetWGPUDevice(), &desc);

	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer");

	if (!m_rhiBuffer)
		return;

	if (desc.mappedAtCreation)
	{
		wgpuBufferReference(m_rhiBuffer);

		if (hasData)
		{
			void* outData = wgpuBufferGetMappedRange(m_rhiBuffer, 0, m_bufSize);
			ASSERT_MSG(outData, "Buffer mapped range is NULL");
			memcpy(outData, bufferInfo.data, writeDataSize);
		}

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
		BufferLockData			data;
		WGPUBuffer				buffer;
		int						flags;
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
	context->flags = m_usageFlags;

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
		if(context->flags & BUFFERUSAGE_WRITE)
			lockData.data = wgpuBufferGetMappedRange(context->buffer, lockData.offset, lockData.size);
		else
			lockData.data = const_cast<void*>(wgpuBufferGetConstMappedRange(context->buffer, lockData.offset, lockData.size));

		if(!lockData.data)
			context->promise.SetError(-1, "Failed to lock buffer, wrong usage?");
		else
			context->promise.SetResult(std::move(context->data));
	};

	wgpuBufferReference(m_rhiBuffer);
	wgpuBufferMapAsync(m_rhiBuffer, WGPUMapMode_Read, lockOfs, sizeToLock, callback, context);

	return context->promise.CreateFuture();
}

void CWGPUBuffer::Unlock()
{
	wgpuBufferUnmap(m_rhiBuffer);
	wgpuBufferRelease(m_rhiBuffer);
}
