#include "core/core_common.h"

#include "renderers/ShaderAPI_defs.h"
#include "WGPUBuffer.h"
#include "WGPURenderAPI.h"

extern WGPURenderAPI s_renderApi;

void CEqWGPUBuffer::Init(const BufferInfo& bufferInfo, int wgpuUsage)
{
	m_bufSize = bufferInfo.elementSize * bufferInfo.elementCapacity;

	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | wgpuUsage;
	desc.size = m_bufSize;
	desc.mappedAtCreation = false;
	if (bufferInfo.flags & BUFFER_FLAG_READ)
		desc.usage |= WGPUBufferUsage_MapRead;

	m_rhiBuffer = wgpuDeviceCreateBuffer(s_renderApi.GetWGPUDevice(), &desc);

	if (bufferInfo.data && bufferInfo.dataSize)
		wgpuQueueWriteBuffer(s_renderApi.GetWGPUQueue(), m_rhiBuffer, 0, bufferInfo.data, min(bufferInfo.dataSize, m_bufSize));
}

void CEqWGPUBuffer::Update(void* data, int size, int offset)
{
	// next commands in queue are not executed until data is uploded
	wgpuQueueWriteBuffer(s_renderApi.GetWGPUQueue(), m_rhiBuffer, offset, data, min(size, m_bufSize));
}

Future<BufferLockData> CEqWGPUBuffer::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
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

void CEqWGPUBuffer::Unlock()
{
	wgpuBufferUnmap(m_rhiBuffer);
}