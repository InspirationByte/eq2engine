//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GPUInstanceMng.h"

DECLARE_CVAR(r_instFlush, "0", nullptr, 0);
DECLARE_CVAR(r_instCompFlush, "0", nullptr, 0);

static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE			= 1536;
static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE_EXTEND	= 512;

void GPUBaseInstanceManager::Initialize()
{
	if (m_updatePipeline)
		return;

	m_updatePipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName("InstanceUtils")
		.ShaderLayoutId(StringToHashConst("InstRoot"))
		.End()
	);

	m_buffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(InstRoot), GPU_INSTANCE_INITIAL_POOL_SIZE), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");
	m_singleInstIndexBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), GPU_INSTANCE_INITIAL_POOL_SIZE), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstIndices");

	{
		IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder("BufferFill");

		Array<int> elementIds(PP_SL);
		// TODO: arrayFill(N)
		for (int i = 0; i < GPU_INSTANCE_INITIAL_POOL_SIZE; ++i)
			elementIds.append(i);

		cmdRecorder->WriteBuffer(m_singleInstIndexBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);

		g_renderAPI->SubmitCommandBuffer(cmdRecorder->End());
	}

	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;
		pool->InitPipeline();
		ASSERT_MSG(pool->updatePipeline, "Failed to create instance update pipeline");

		// initialize buffer so game won't crash when no instances synced
		pool->buffer = g_renderAPI->CreateBuffer(BufferInfo(pool->stride, GPU_INSTANCE_INITIAL_POOL_SIZE), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");
	}
}

void GPUBaseInstanceManager::Shutdown()
{
	m_updatePipeline = nullptr;
	m_buffer = nullptr;	
	m_singleInstIndexBuffer = nullptr;

	m_updated.clear();
	m_instances.clear();
	m_freeIndices.clear();
	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		pool->buffer = nullptr;
		pool->updatePipeline = nullptr;
		pool->freeIndices.clear();
		pool->updated.clear();
		pool->ResetData();
		pool->updated.insert(0);
	}
}

int	GPUBaseInstanceManager::AllocInstance()
{
	int instanceId;
	{
		Threading::CScopedMutex m(m_mutex);
		instanceId = m_freeIndices.numElem() ? m_freeIndices.popBack() : m_instances.append({});
	}
	memset(m_instances[instanceId].components, 0, sizeof(InstRoot::components));
	m_updated.insert(instanceId);

	return instanceId;
}

// destroys instance and it's components
void GPUBaseInstanceManager::FreeInstance(int instanceId)
{
	if (!m_instances.inRange(instanceId))
		return;

	{
		Threading::CScopedMutex m(m_mutex);
		m_freeIndices.append(instanceId);
	}

	InstRoot& inst = m_instances[instanceId];
	for (int i = 0; i < GPUINST_MAX_COMPONENTS; ++i)
	{
		// skip invalid or defaults
		if (inst.components[i] == UINT_MAX)
			continue;

		// add this instance to freed list and invalidate ID
		if(inst.components[i] > 0)
		{
			Threading::CScopedMutex m(m_mutex);
			m_componentPools[i]->freeIndices.append(inst.components[i]);
		}
		inst.components[i] = UINT_MAX;
	}
}

IGPUBufferPtr GPUBaseInstanceManager::GetDataPoolBuffer(int componentId) const
{
	return m_componentPools[componentId]->buffer;
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
		Builder<BindGroupDesc>().GroupIndex(1)
		.Buffer(0, targetBuffer)
		.End()
	);

	computePass->SetPipeline(updatePipeline);
	computePass->SetBindGroup(0, sourceIdxsAndDataGroup);
	computePass->SetBindGroup(1, destPoolDataGroup);

	const float instacesPerWorkgroup = 64.0f; // TODO: adjustable?

	computePass->DispatchWorkgroups(ceil(idxsCount / instacesPerWorkgroup));
	computePass->Complete();
}

static void instPrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
{
	const int updatedCount = updated.size();
	const int updateBufferSize = updatedCount * sourceStride;

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
		memcpy(updateData, updInstPtr, sourceStride);
		updateData += sourceStride;
	}

	destIdxsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(elementIds[0]), elementIds.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdIdxs");
	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");

	cmdRecorder->WriteBuffer(destIdxsBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

void GPUBaseInstanceManager::SyncInstances(IGPUCommandRecorder* cmdRecorder)
{
	Threading::CScopedMutex m(m_mutex);

	Array<int> elementIds(PP_SL);

	// update instance root buffer
	// TODO: instead of re-creating buffer, create new separate buffer with it's instance pools
	if (r_instFlush.GetBool() || !m_buffer || m_instances.numElem() > (m_buffer->GetSize() / sizeof(InstRoot)))
	{
		// alloc (or re-create) new buffer and upload entire data
		const int allocInstBufferElems = max(instGranulatedCapacity(m_instances.numElem()), GPU_INSTANCE_INITIAL_POOL_SIZE);
		m_buffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(InstRoot), allocInstBufferElems), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");
		m_singleInstIndexBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), allocInstBufferElems), BUFFERUSAGE_VERTEX | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstIndices");

		cmdRecorder->WriteBuffer(m_buffer, m_instances.ptr(), m_instances.numElem() * sizeof(InstRoot), 0);

		// TODO: arrayFill(N)
		for(int i = 0; i < allocInstBufferElems; ++i)
			elementIds.append(i);
		cmdRecorder->WriteBuffer(m_singleInstIndexBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
	}
	else if(m_updated.size())
	{
		IGPUBufferPtr idxsBuffer;
		IGPUBufferPtr dataBuffer;
		instPrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(m_instances.ptr()), sizeof(InstRoot), idxsBuffer, dataBuffer);
		instRunUpdatePipeline(cmdRecorder, m_updatePipeline, idxsBuffer, m_updated.size(), dataBuffer, m_buffer);
	}
	m_updated.clear();

	// update instance components buffers
	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		const int elemSize = pool->stride;
		const int poolElems = pool->GetDataElems();
		const ubyte* poolDataPtr = reinterpret_cast<const ubyte*>(pool->GetDataPtr());

		if (r_instCompFlush.GetBool() || !pool->buffer || poolElems > (pool->buffer->GetSize() / elemSize))
		{
			// alloc (or re-create) new buffer and upload entire data
			const int allocInstBufferElems = max(instGranulatedCapacity(poolElems), GPU_INSTANCE_INITIAL_POOL_SIZE);
			pool->buffer = g_renderAPI->CreateBuffer(BufferInfo(elemSize, allocInstBufferElems), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstData");

			cmdRecorder->WriteBuffer(pool->buffer, poolDataPtr, poolElems * elemSize, 0);
		}
		else if (pool->updated.size())
		{
			// prepare update buffer
			IGPUBufferPtr idxsBuffer;
			IGPUBufferPtr dataBuffer;
			instPrepareBuffers(cmdRecorder, pool->updated, elementIds, poolDataPtr, elemSize, idxsBuffer, dataBuffer);
			instRunUpdatePipeline(cmdRecorder, pool->updatePipeline, idxsBuffer, pool->updated.size(), dataBuffer, pool->buffer);
		}
		pool->updated.clear();
	}

	r_instCompFlush.SetBool(false);
	r_instFlush.SetBool(false);
}
