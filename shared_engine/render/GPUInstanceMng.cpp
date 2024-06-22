//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GPUInstanceMng.h"

static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE				= 1536;
static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE_EXTEND		= 512;

void GPUBaseInstanceManager::Initialize()
{
	m_updatePipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName("InstanceUtils")
		.ShaderLayoutId(StringToHashConst("Root"))
		.End()
	);

	for (GPUInstPool* pool : m_componentPools)
	{
		pool->InitPipeline();
		ASSERT_MSG(pool->updatePipeline, "Failed to create instance update pipeline");
	}
}

void GPUBaseInstanceManager::Shutdown()
{
	m_updatePipeline = nullptr;
	m_buffer = nullptr;
	for (GPUInstPool* pool : m_componentPools)
	{
		pool->buffer = nullptr;
		pool->updatePipeline = nullptr;
		pool->freeIndices.clear(true);
		pool->updated.clear(true);
	}
}

int	GPUBaseInstanceManager::AllocInstance()
{
	const int instanceId = m_freeIndices.numElem() ? m_freeIndices.popBack() : m_instances.append({});
	memset(m_instances[instanceId].components, UINT_MAX, sizeof(InstRoot::components));

	return instanceId;
}

// destroys instance and it's components
void GPUBaseInstanceManager::FreeInstance(int instanceId)
{
	InstRoot& inst = m_instances[instanceId];
	for (int i = 0; i < GPUINST_MAX_COMPONENTS; ++i)
	{
		if (inst.components[i] == UINT_MAX)
			continue;

		auto it = m_componentPools.find(i);
		if (it.atEnd())
		{
			ASSERT_FAIL("Component free list map is not initialized properly");
			return;
		}

		// add this instance to freed list and invalidate ID
		(*it)->freeIndices.append(inst.components[i]);
		inst.components[i] = UINT_MAX;
	}
}

IGPUBufferPtr GPUBaseInstanceManager::GetDataPoolBuffer(int componentId) const
{
	auto it = m_componentPools.find(componentId);
	if (it.atEnd())
		return nullptr;

	return (*it)->buffer;
}

static int instGranulatedCapacity(int capacity)
{
	constexpr int granularity = GPU_INSTANCE_INITIAL_POOL_SIZE_EXTEND;
	return (capacity + granularity - 1) / granularity * granularity;
}

static void instRunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, IGPUBuffer* targetBuffer)
{
	IGPUComputePassRecorderPtr computePass = cmdRecorder->BeginComputePass("UpdateInstances");

	IGPUBindGroupPtr sourceIdxsAndDataGroup = g_renderAPI->CreateBindGroup(updatePipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.Buffer(0, idxsBuffer)
		.Buffer(1, dataBuffer)
		.End()
	);
	IGPUBindGroupPtr destPoolDataGroup = g_renderAPI->CreateBindGroup(updatePipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.Buffer(0, targetBuffer)
		.End()
	);

	computePass->SetPipeline(updatePipeline);
	computePass->SetBindGroup(0, sourceIdxsAndDataGroup);
	computePass->SetBindGroup(1, destPoolDataGroup);

	const int instacesPerWorkgroup = 64; // TODO: adjustable?

	computePass->DispatchWorkgroups(idxsCount / instacesPerWorkgroup);
	computePass->Complete();
}

static void instPrepareBuffers(IGPUCommandRecorder* cmdRecorder, Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
{
	const int updateBufferSize = updated.size() * sourceStride;
	elementIds.reserve(updated.size());
	elementIds.clear();

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
		memcpy(updateData, updInstPtr, sourceStride);
		updateData += sourceStride;
	}
	updated.clear();

	destIdxsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(elementIds[0]), elementIds.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdIdxs");
	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(updateDataStart, 1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");

	cmdRecorder->WriteBuffer(destIdxsBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);

}

void GPUBaseInstanceManager::SyncInstances(IGPUCommandRecorder* cmdRecorder)
{
	Array<int> elementIds(PP_SL);

	// update instance root buffer
	if (!m_buffer || m_instances.numElem() > (m_buffer->GetSize() / sizeof(InstRoot)))
	{
		m_updated.clear();

		// alloc (or re-create) new buffer and upload entire data
		const int allocInstBufferElems = max(instGranulatedCapacity(m_instances.numElem()), GPU_INSTANCE_INITIAL_POOL_SIZE);
		m_buffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(InstRoot), allocInstBufferElems), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");

		cmdRecorder->WriteBuffer(m_buffer, m_instances.ptr(), m_instances.numElem() * sizeof(InstRoot), 0);
	}
	else if(m_updated.size())
	{
		IGPUBufferPtr idxsBuffer;
		IGPUBufferPtr dataBuffer;
		instPrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(m_instances.ptr()), sizeof(InstRoot), idxsBuffer, dataBuffer);
		instRunUpdatePipeline(cmdRecorder, m_updatePipeline, idxsBuffer, elementIds.numElem(), dataBuffer, m_buffer);
	}

	// update instance components buffers
	for (GPUInstPool* pool : m_componentPools)
	{
		if (pool->updated.size() == 0)
			continue;

		const int elemSize = pool->stride;
		const ubyte* dataPtr = reinterpret_cast<const ubyte*>(pool->GetDataPtr());

		int maxUpdateIdx = 0;
		for (auto it = pool->updated.begin(); !it.atEnd(); ++it)
		{
			const int elemIdx = it.key();
			maxUpdateIdx = max(maxUpdateIdx, elemIdx);
		}

		if (!pool->buffer || maxUpdateIdx > (pool->buffer->GetSize() / elemSize))
		{
			pool->updated.clear();

			// alloc (or re-create) new buffer and upload entire data
			const int allocInstBufferElems = max(instGranulatedCapacity(maxUpdateIdx), GPU_INSTANCE_INITIAL_POOL_SIZE);
			pool->buffer = g_renderAPI->CreateBuffer(BufferInfo(elemSize, allocInstBufferElems), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");

			cmdRecorder->WriteBuffer(pool->buffer, dataPtr, maxUpdateIdx * elemSize, 0);
			continue;
		}

		// prepare update buffer
		IGPUBufferPtr idxsBuffer;
		IGPUBufferPtr dataBuffer;
		instPrepareBuffers(cmdRecorder, pool->updated, elementIds, dataPtr, elemSize, idxsBuffer, dataBuffer);
		instRunUpdatePipeline(cmdRecorder, pool->updatePipeline, idxsBuffer, elementIds.numElem(), dataBuffer, pool->buffer);
	}
}

