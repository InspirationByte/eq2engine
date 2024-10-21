//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "../RenderWorker.h"
#include "WGPUBuffer.h"
#include "WGPURenderAPI.h"


CWGPUBuffer::~CWGPUBuffer()
{
	wgpuBufferRelease(m_rhiBuffer);
	m_rhiBuffer = nullptr;
}

CWGPUBuffer::CWGPUBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label)
{
	int wgpuUsageFlags = 0;

	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM) wgpuUsageFlags |= WGPUBufferUsage_Uniform;
	if (bufferUsageFlags & BUFFERUSAGE_VERTEX)	wgpuUsageFlags |= WGPUBufferUsage_Vertex;
	if (bufferUsageFlags & BUFFERUSAGE_INDEX)	wgpuUsageFlags |= WGPUBufferUsage_Index;
	if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)wgpuUsageFlags |= WGPUBufferUsage_Indirect;
	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)	wgpuUsageFlags |= WGPUBufferUsage_Storage;

	if (bufferUsageFlags & BUFFERUSAGE_READ)	wgpuUsageFlags |= WGPUBufferUsage_MapRead;
	if (bufferUsageFlags & BUFFERUSAGE_WRITE)	wgpuUsageFlags |= WGPUBufferUsage_MapWrite;
	if (bufferUsageFlags & BUFFERUSAGE_COPY_SRC) wgpuUsageFlags |= WGPUBufferUsage_CopySrc;
	if (bufferUsageFlags & BUFFERUSAGE_COPY_DST) wgpuUsageFlags |= WGPUBufferUsage_CopyDst;

	const int sizeInBytes = bufferInfo.elementSize * bufferInfo.elementCapacity;
	const int writeDataSize = (bufferInfo.dataSize + 3) & ~3;
	const bool hasData = bufferInfo.data && bufferInfo.dataSize;

	m_bufSize = (sizeInBytes + 3) & ~3;
	m_usageFlags = bufferUsageFlags;

	WGPUBufferDescriptor rhiBufferDesc = {};
	rhiBufferDesc.usage = wgpuUsageFlags;
	rhiBufferDesc.size = m_bufSize;
	rhiBufferDesc.mappedAtCreation = hasData;
	rhiBufferDesc.label = _WSTR(label);

	m_rhiBuffer = wgpuDeviceCreateBuffer(CWGPURenderAPI::Instance.GetWGPUDevice(), &rhiBufferDesc);

	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer %s", label);

	if (!m_rhiBuffer)
		return;

	if (rhiBufferDesc.mappedAtCreation)
	{
		wgpuBufferAddRef(m_rhiBuffer);

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

Future<BufferMapData> CWGPUBuffer::Lock(int lockOfs, int sizeToLock, int flags)
{
	struct LockContext
	{
		Promise<BufferMapData> promise;
		BufferMapData			data;
		WGPUBuffer				buffer;
		int						flags;
	};

	if (!m_rhiBuffer || lockOfs < 0 || lockOfs + sizeToLock > m_bufSize)
	{
		ASSERT_FAIL("Locking outside range");
		Promise<BufferMapData> errorPromise;
		errorPromise.SetError(-1, "Lock failure");
		return errorPromise.CreateFuture();
	}

	LockContext* context = PPNew LockContext;
	context->buffer = m_rhiBuffer;
	context->flags = m_usageFlags;

	BufferMapData& lockData = context->data;
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

		BufferMapData& lockData = context->data;
		if(context->flags & BUFFERUSAGE_WRITE)
			lockData.data = wgpuBufferGetMappedRange(context->buffer, lockData.offset, lockData.size);
		else
			lockData.data = const_cast<void*>(wgpuBufferGetConstMappedRange(context->buffer, lockData.offset, lockData.size));

		if(!lockData.data)
			context->promise.SetError(-1, "Failed to lock buffer, wrong usage?");
		else
			context->promise.SetResult(std::move(context->data));
	};

	wgpuBufferAddRef(m_rhiBuffer);
	wgpuBufferMapAsync(m_rhiBuffer, WGPUMapMode_Read, lockOfs, sizeToLock, callback, context);

	return context->promise.CreateFuture();
}

void CWGPUBuffer::Unlock()
{
	wgpuBufferUnmap(m_rhiBuffer);
	wgpuBufferRelease(m_rhiBuffer);
}
