#include "core/core_common.h"

#include "renderers/ShaderAPI_defs.h"
#include "WGPUBuffer.h"
#include "WGPURenderAPI.h"

extern WGPURenderAPI s_renderApi;

void CEqWGPUBufferImpl::Init(const BufferInfo& bufferInfo, int wgpuUsage)
{
	m_bufferSize = bufferInfo.elementSize * bufferInfo.elementCapacity;

	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | wgpuUsage;
	desc.size = m_bufferSize;
	desc.mappedAtCreation = false;

	// TODO: label

	// if (bufferInfo.accessFlags & BUFFER_FLAG_READ)
	// 	desc.usage |= WGPUBufferUsage_CopySrc;

	m_buffer = wgpuDeviceCreateBuffer(s_renderApi.GetWGPUDevice(), &desc);

	if (bufferInfo.data && bufferInfo.dataSize)
		wgpuQueueWriteBuffer(s_renderApi.GetWGPUQueue(), m_buffer, 0, bufferInfo.data, min(bufferInfo.dataSize, m_bufferSize));
}

void CEqWGPUBufferImpl::Update(void* data, int size, int offset, bool discard)
{
	// next commands in queue are not executed until data is uploded
	wgpuQueueWriteBuffer(s_renderApi.GetWGPUQueue(), m_buffer, offset, data, min(size, m_bufferSize));
}

bool CEqWGPUBufferImpl::Lock(int lockOfs, int sizeToLock, void** outdata, bool readBack)
{
	WGPUMapMode_Read;
	return false;
}

void CEqWGPUBufferImpl::Unlock()
{

}