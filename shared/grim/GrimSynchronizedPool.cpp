//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: Synchronized slotted buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimSynchronizedPool.h"

static constexpr int GPU_POOL_BUFFER_USAGE_FLAGS = BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST;

GRIMLock GRIMLock::EmptyLock = {};

GRIMResource::GRIMResource(Type type)
	: type(type)
	, buffer(nullptr)
{
}

GRIMResource::~GRIMResource()
{
	Reset();
}

void GRIMResource::Reset()
{
	if (type == BUFFER)
		buffer = nullptr;
	else if (type == TEXTURE)
		texture = nullptr;
}

int	GRIMResource::GetSize() const
{
	if (type == BUFFER)
		return buffer->GetSize();
	else if (type == TEXTURE)
		return texture->GetArraySize();

	ASSERT_FAIL("Invalid type %d", type);
	return 0;
}

GRIMResource::operator bool() const
{
	if (type == BUFFER)
		return buffer;
	else if (type == TEXTURE)
		return texture;
	ASSERT_FAIL("Invalid type %d", type);
	return false;
}

void GRIMBaseSyncrhronizedPool::RunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, const GRIMResource& targetData)
{
	IGPUComputePassRecorderPtr computePass = cmdRecorder->BeginComputePass("UpdateInstances");

	Builder<BindGroupDesc> bindGroupDesc;
	bindGroupDesc.GroupIndex(0)
		.Buffer(StringIdConst24("sourceInfo"), idxsBuffer)
		.Buffer(StringIdConst24("sourceData"), dataBuffer);

	if (targetData.GetType() == GRIMResource::BUFFER)
		bindGroupDesc.Buffer(StringIdConst24("destData"), targetData.Get<IGPUBuffer>());
	else if (targetData.GetType() == GRIMResource::TEXTURE)
		bindGroupDesc.StorageTexture(StringIdConst24("destData"), targetData.Get<ITexture>());

	IGPUBindGroupPtr bindGroup = g_renderAPI->CreateBindGroup(updatePipeline, bindGroupDesc.End());

	computePass->SetPipeline(updatePipeline);
	computePass->SetBindGroup(0, bindGroup);

	IVector2D workGroups = CalcWorkSize(idxsCount);
	computePass->DispatchWorkgroups(workGroups.x, workGroups.y);
	computePass->Complete();
}

// prepares data buffer
// automatically grows destDataBuffer
void GRIMBaseSyncrhronizedPool::PrepareDataBuffer(IGPUCommandRecorder* cmdRecorder, ArrayCRef<int> elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destDataBuffer)
{
	ArrayCRef<int> elementIdArray = ArrayCRef(elementIds.ptr()+1, elementIds.numElem()-1);
	const int updateBufferSize = elementIdArray.numElem() * elemSize;

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (const int elemIdx : elementIdArray)
	{
		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	const BufferInfo reqDataBufferInfo(1, updateBufferSize);

	if (!destDataBuffer || destDataBuffer->GetSize() < reqDataBufferInfo.GetBufferSize())
		destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), GPU_POOL_BUFFER_USAGE_FLAGS, "InstUpdData");

	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

// prepare data and indices
// automatically grows destIdxsBuffer & destDataBuffer
void GRIMBaseSyncrhronizedPool::PrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
{
	ASSERT(elemSize <= sourceStride);

	const int updatedCount = updated.size();
	elementIds.reserve(updatedCount + 1);
	elementIds.clear();

	// always insert count as first element (sourceCount)
	elementIds.append(updatedCount);

	for (auto it = updated.begin(); !it.atEnd(); ++it)
		elementIds.append(it.key());

	const int updateBufferSize = elementIds.numElem() * elemSize;

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (const int elemIdx : ArrayCRef(elementIds.ptr() + 1, elementIds.numElem() - 1))
	{
		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	const BufferInfo reqIdxsBufferInfo(sizeof(elementIds[0]), elementIds.numElem());
	const BufferInfo reqDataBufferInfo(1, updateBufferSize);

	if(!destIdxsBuffer || destIdxsBuffer->GetSize() < reqIdxsBufferInfo.GetBufferSize())
		destIdxsBuffer = g_renderAPI->CreateBuffer(reqIdxsBufferInfo, GPU_POOL_BUFFER_USAGE_FLAGS, "InstUpdIdxs");

	if (!destDataBuffer || destDataBuffer->GetSize() < reqDataBufferInfo.GetBufferSize())
		destDataBuffer = g_renderAPI->CreateBuffer(reqDataBufferInfo, GPU_POOL_BUFFER_USAGE_FLAGS, "InstUpdData");

	cmdRecorder->WriteBuffer(destIdxsBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

int GRIMBaseSyncrhronizedPool::GetGranulatedCapacity(int capacity, int granularity)
{
	return (capacity + granularity - 1) / granularity * granularity;
}

IVector2D GRIMBaseSyncrhronizedPool::CalcWorkSize(int length)
{
	constexpr int GPUSYNC_GROUP_SIZE = 256;
	constexpr int GPUSYNC_MAX_DIM_GROUPS = 1024;
	constexpr int GPUSYNC_MAX_DIM_THREADS = (GPUSYNC_GROUP_SIZE * GPUSYNC_MAX_DIM_GROUPS);

	IVector2D result;
	if (length <= GPUSYNC_MAX_DIM_THREADS)
	{
		result.x = (length - 1) / GPUSYNC_GROUP_SIZE + 1;
		result.y = 1;
	}
	else
	{
		result.x = GPUSYNC_MAX_DIM_GROUPS;
		result.y = (length - 1) / GPUSYNC_MAX_DIM_THREADS + 1;
	}

	return result;
}

GRIMBaseSyncrhronizedPool::GRIMBaseSyncrhronizedPool(GRIMResource::Type type, const char* name)
	: m_name(name)
	, m_gpuData(type)
{
}

void GRIMBaseSyncrhronizedPool::InitBuffer(int extraBufferFlags, int initialSize, int gpuItemsGranularity)
{
	ASSERT_MSG(GetType() == GRIMResource::BUFFER, "Trying to inititialize BUFFER resource while it's not");

	m_extraResourceFlags = extraBufferFlags;
	m_initialSize = initialSize;
	m_gpuItemsGranularity = gpuItemsGranularity;
}

void GRIMBaseSyncrhronizedPool::InitTexture(ETextureFormat format, IVector2D textureSize, int extraTextureFlags, int initialArraySize, int gpuItemsGranularity)
{
	ASSERT_MSG(GetType() == GRIMResource::TEXTURE, "Trying to inititialize TEXTURE resource while it's not");

	m_texFormat = format;
	m_texSize = textureSize;

	m_extraResourceFlags = extraTextureFlags;
	m_initialSize = initialArraySize;
	m_gpuItemsGranularity = gpuItemsGranularity;
}

void GRIMBaseSyncrhronizedPool::SetPipeline(IGPUComputePipelinePtr updatePipeline)
{
	m_updatePipeline = updatePipeline;
}

bool GRIMBaseSyncrhronizedPool::SyncImpl(IGPUCommandRecorder* cmdRecorder, const void* dataPtr, int stride, GRIMLock& lock, IGPUBufferPtr& updDataBuffer, IGPUBufferPtr& updIdxsBuffer)
{
	if (!m_updatePipeline)
	{
		ASSERT_FAIL("GRIMSyncrhronizedPool <%s> is not initialized", m_name.ToCString());
		return false;
	}

	const int currentNumSlots = NumSlots();
	int oldInstElems = m_gpuData ? m_gpuData.GetSize() : 0;
	if (m_gpuData.GetType() == GRIMResource::BUFFER)
		oldInstElems /= stride;

	bool buffersUpdated = false;

	// update instance root buffer
	// TODO: instead of re-creating buffer, create new separate buffer with it's instance pools
	if (!m_gpuData || currentNumSlots > oldInstElems)
	{
		// alloc (or re-create) new buffer and upload entire data
		const int allocInstElems = max(GetGranulatedCapacity(currentNumSlots, m_gpuItemsGranularity), m_initialSize);

		if (m_gpuData.GetType() == GRIMResource::BUFFER)
		{
			IGPUBufferPtr oldBuffer(m_gpuData.Get<IGPUBuffer>());

			const int bufferFlags = m_extraResourceFlags | BUFFERUSAGE_COPY_SRC | GPU_POOL_BUFFER_USAGE_FLAGS;
			IGPUBufferPtr newBuffer = g_renderAPI->CreateBuffer(BufferInfo(stride, allocInstElems), bufferFlags, m_name);
			m_gpuData.Set(newBuffer.Ptr());

			if (oldInstElems > 0 && oldBuffer)
			{
				// copy old buffer data to new one, and still run update pipeline later below
				// effectively updating old data and adding new data
				cmdRecorder->CopyBufferToBuffer(oldBuffer, 0, newBuffer, 0, oldInstElems * stride);
			}
			else
			{
				// don't waste time on running pipeline and upload everything directly to GPU
				// since buffer is brand new
				cmdRecorder->WriteBuffer(newBuffer, dataPtr, currentNumSlots * stride, 0);

				lock.LockWrite();
				m_updated.clear();
				lock.UnlockWrite();

				return true;
			}
		}
		else if (m_gpuData.GetType() == GRIMResource::TEXTURE)
		{
			ITexturePtr oldTexture(m_gpuData.Get<ITexture>());

			const int textureFlags = m_extraResourceFlags | TEXFLAG_STORAGE | TEXFLAG_COPY_DST | TEXFLAG_COPY_SRC;
			ITexturePtr newTexture = g_renderAPI->CreateRenderTarget(TextureDesc(EqString::Format("%s_%d", m_name, allocInstElems), textureFlags, m_texFormat, m_texSize.x, m_texSize.y, allocInstElems));
			m_gpuData.Set(newTexture.Ptr());

			if (oldInstElems > 0 && oldTexture)
			{
				// copy previous texture data and then run pipeline to update items in it
				cmdRecorder->CopyTextureToTexture(TextureCopyInfo{ oldTexture }, TextureCopyInfo{ newTexture }, oldTexture->GetSize());
			}
		}
		
		buffersUpdated = true;
	}

	lock.LockRead();
	if (m_updated.size())
	{
		PrepareBuffers(cmdRecorder, m_updated, m_syncElementIds, reinterpret_cast<const ubyte*>(dataPtr), stride, stride, updIdxsBuffer, updDataBuffer);
		RunUpdatePipeline(cmdRecorder, m_updatePipeline, updIdxsBuffer, m_updated.size(), updDataBuffer, m_gpuData);
	}
	lock.UnlockRead();

	lock.LockWrite();
	m_updated.clear();
	lock.UnlockWrite();

	return buffersUpdated;
}

void GRIMBaseSyncrhronizedPool::SetUpdated(int idx)
{
	m_updated.insert(idx);
}