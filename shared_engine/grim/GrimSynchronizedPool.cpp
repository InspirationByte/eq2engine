//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: Synchronized slotted buffer
//////////////////////////////////////////////////////////////////////////////////


#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimSynchronizedPool.h"

void GRIMBaseSyncrhronizedPool::RunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, IGPUBuffer* targetBuffer)
{
	IGPUComputePassRecorderPtr computePass = cmdRecorder->BeginComputePass("UpdateInstances");

	IGPUBindGroupPtr sourceIdxsAndDataGroup = g_renderAPI->CreateBindGroup(updatePipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.Buffer(0, idxsBuffer)
		.Buffer(1, dataBuffer)
		.End()
	);
	IGPUBindGroupPtr destPoolDataGroup = g_renderAPI->CreateBindGroup(updatePipeline,
		Builder<BindGroupDesc>().GroupIndex(1)
		.Buffer(0, targetBuffer)
		.End()
	);

	computePass->SetPipeline(updatePipeline);
	computePass->SetBindGroup(0, sourceIdxsAndDataGroup);
	computePass->SetBindGroup(1, destPoolDataGroup);

	IVector2D workGroups = CalcWorkSize(idxsCount);
	computePass->DispatchWorkgroups(workGroups.x, workGroups.y);
	computePass->Complete();
}

// prepares data buffer
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

	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

// prepare data and indices
void GRIMBaseSyncrhronizedPool::PrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
{
	ASSERT(elemSize <= sourceStride);

	const int updatedCount = updated.size();
	const int updateBufferSize = updatedCount * elemSize;

	elementIds.reserve(updatedCount + 1);
	elementIds.clear();

	// always insert count as first element (sourceCount)
	elementIds.append(updatedCount);

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (auto it = updated.begin(); !it.atEnd(); ++it)
	{
		const int elemIdx = it.key();
		elementIds.append(elemIdx);

		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	destIdxsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(elementIds[0]), elementIds.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdIdxs");
	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");

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

GRIMBaseSyncrhronizedPool::GRIMBaseSyncrhronizedPool(const char* name)
	: m_name(name)
{
}

void GRIMBaseSyncrhronizedPool::Init(int extraBufferFlags, int initialSize, int gpuItemsGranularity)
{
	m_extraBufferFlags = extraBufferFlags;
	m_initialSize = initialSize;
	m_gpuItemsGranularity = gpuItemsGranularity;
}

void GRIMBaseSyncrhronizedPool::SetPipeline(IGPUComputePipelinePtr updatePipeline)
{
	m_updatePipeline = updatePipeline;
}

bool GRIMBaseSyncrhronizedPool::SyncImpl(IGPUCommandRecorder* cmdRecorder, const void* dataPtr, int stride)
{
	if (!m_updatePipeline)
	{
		ASSERT_FAIL("GRIMSyncrhronizedPool <%s> is not initialized", m_name.ToCString());
		return false;
	}

	const int currentNumSlots = NumSlots();
	const int oldBufferElems = m_buffer ? m_buffer->GetSize() / stride : 0;

	bool buffersUpdated = false;

	// update instance root buffer
	// TODO: instead of re-creating buffer, create new separate buffer with it's instance pools
	if (!m_buffer || currentNumSlots > oldBufferElems)
	{
		// alloc (or re-create) new buffer and upload entire data
		const int allocInstBufferElems = max(GetGranulatedCapacity(currentNumSlots, m_gpuItemsGranularity), m_initialSize);

		const int bufferFlags = m_extraBufferFlags | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST | BUFFERUSAGE_COPY_SRC;

		IGPUBufferPtr oldBuffer = m_buffer;
		m_buffer = g_renderAPI->CreateBuffer(BufferInfo(stride, allocInstBufferElems), bufferFlags, m_name);

		if (oldBufferElems > 0)
			cmdRecorder->CopyBufferToBuffer(oldBuffer, 0, m_buffer, 0, oldBufferElems * stride);
		else if (!oldBuffer)
		{
			// don't wast� time on running pipeline and upload directly to GPU
			cmdRecorder->WriteBuffer(m_buffer, dataPtr, currentNumSlots * stride, 0);

			{
				Threading::CScopedMutex m(m_mutex);
				m_updated.clear();
			}
			return true;
		}

		buffersUpdated = true;
	}

	{
		Threading::CScopedMutex m(m_mutex);
		if (m_updated.size())
		{
			Array<int> elementIds(PP_SL);

			IGPUBufferPtr idxsBuffer;
			IGPUBufferPtr dataBuffer;
			PrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(dataPtr), stride, stride, idxsBuffer, dataBuffer);
			RunUpdatePipeline(cmdRecorder, m_updatePipeline, idxsBuffer, m_updated.size(), dataBuffer, m_buffer);
		}
		m_updated.clear();
	}

	return buffersUpdated;
}

void GRIMBaseSyncrhronizedPool::SetUpdated(int idx)
{
	m_updated.insert(idx);
}