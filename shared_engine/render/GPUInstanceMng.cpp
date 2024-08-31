//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances manager
//////////////////////////////////////////////////////////////////////////////////

#include <imgui.h>
#include "core/core_common.h"
#include "core/ConVar.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GPUInstanceMng.h"

static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE		= 3072;
static constexpr int GPU_INSTANCE_POOL_SIZE_EXTEND		= 1024;
static constexpr int GPU_INSTANCE_MAX_TEMP_INSTANCES	= 128;

static constexpr int GPU_INSTANCE_BUFFER_USAGE_FLAGS = BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST | BUFFERUSAGE_COPY_SRC;

GPUBaseInstanceManager::GPUBaseInstanceManager()
{
	// alloc default (zero) instance
	m_instances.append(Instance{});
	m_updated.insert(0);
}

void GPUBaseInstanceManager::Initialize(const char* instanceComputeShaderName, int instancesReserve)
{
	m_reservedInsts = instancesReserve;

	if (!m_updateRootPipeline)
	{
		m_updateRootPipeline = g_renderAPI->CreateComputePipeline(
			Builder<ComputePipelineDesc>()
			.ShaderName(instanceComputeShaderName)
			.ShaderLayoutId(StringToHashConst("InstRoot"))
			.End()
		);
	}

	if (!m_updateIntPipeline)
	{
		m_updateIntPipeline = g_renderAPI->CreateComputePipeline(
			Builder<ComputePipelineDesc>()
			.ShaderName(instanceComputeShaderName)
			.ShaderLayoutId(StringToHashConst("InstInt"))
			.End()
		);
	}

	m_instances.reserve(m_reservedInsts);

	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool || pool->updatePipeline)
			continue;

		pool->InitPipeline();
		pool->ReserveData(m_reservedInsts);
		ASSERT_MSG(pool->updatePipeline, "Failed to create instance update pipeline");
	}
}

void GPUBaseInstanceManager::Shutdown()
{
	FreeAll(true);

	m_updateRootPipeline = nullptr;
	m_updateIntPipeline = nullptr;

	m_rootBuffer = nullptr;
	m_archetypesBuffer = nullptr;
	m_singleInstIndexBuffer = nullptr;

	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		pool->buffer = nullptr;
		pool->updatePipeline = nullptr;
	}
}

void GPUBaseInstanceManager::FreeAll(bool dealloc, bool reserve)
{
	m_updated.clear(dealloc);
	m_freeIndices.clear(dealloc);
	m_tempInstances.clear(dealloc);
	m_archetypeInstCounts.clear(dealloc);
	m_instances.clear(dealloc);

	// alloc default (zero) instance
	m_instances.setNum(1);
	m_updated.insert(0);

	if (reserve)
		m_instances.reserve(m_reservedInsts);

	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		pool->freeIndices.clear(dealloc);
		pool->updated.clear(dealloc);
		pool->updated.insert(0);

		pool->ResetData();

		if(reserve)
			pool->ReserveData(m_reservedInsts);
	}
}

GPUBufferView GPUBaseInstanceManager::GetSingleInstanceIndexBuffer() const
{
	return GPUBufferView(m_singleInstIndexBuffer, sizeof(int));
}

int	GPUBaseInstanceManager::GetInstanceCountByArchetype(int archetypeId) const
{
	auto it = m_archetypeInstCounts.find(archetypeId);
	if (it.atEnd())
		return 0;

	return *it;
}

void GPUBaseInstanceManager::DbgRegisterArhetypeName(int archetypeId, const char* name)
{
#ifdef ENABLE_GPU_INSTANCE_DEBUG
	Threading::CScopedMutex m(m_mutex);
	if(m_archetypeNames.find(archetypeId).atEnd())
		m_archetypeNames.insert(archetypeId, name);
#endif
}

int	GPUBaseInstanceManager::AllocInstance(int archetype)
{
	Threading::CScopedMutex m(m_mutex);
	const int instanceId = m_freeIndices.numElem() ? m_freeIndices.popBack() : m_instances.append({});

	Instance& inst = m_instances[instanceId];
	inst.batchesFlags = 0;
	inst.archetype = archetype;
	memset(&inst.root.components, 0, sizeof(inst.root.components));

	m_updated.insert(instanceId);

	++m_archetypeInstCounts[archetype];

	return instanceId;
}

int GPUBaseInstanceManager::AllocTempInstance(int archetype)
{
	if (m_tempInstances.numElem() >= GPU_INSTANCE_MAX_TEMP_INSTANCES)
		return -1;

	const int instIdx = AllocInstance(archetype);
	m_tempInstances.append(instIdx);
	return instIdx;
}

// sets batches that are drrawn with particular instance
void GPUBaseInstanceManager::SetBatches(int instanceId, uint batchesFlags)
{
	if (!m_instances.inRange(instanceId))
		return;

	Instance& inst = m_instances[instanceId];
	inst.batchesFlags = batchesFlags;
}

// destroys instance and it's components
void GPUBaseInstanceManager::FreeInstance(int instanceId)
{
	if (!m_instances.inRange(instanceId))
		return;

	Instance& inst = m_instances[instanceId];
	InstRoot& root = inst.root;
	{
		Threading::CScopedMutex m(m_mutex);
		m_freeIndices.append(instanceId);

		--m_archetypeInstCounts[inst.archetype];
	}

	inst.archetype = -1;
	inst.batchesFlags = 0;

	for (int i = 0; i < GPUINST_MAX_COMPONENTS; ++i)
	{
		// skip invalid or defaults
		if (root.components[i] == UINT_MAX)
			continue;

		// add this instance to freed list and invalidate ID
		if(root.components[i] > 0)
		{
			Threading::CScopedMutex m(m_mutex);
			m_componentPools[i]->freeIndices.append(root.components[i]);
		}
		root.components[i] = UINT_MAX;
	}

	// update roots and archetypes
	m_updated.insert(instanceId);
}

IGPUBufferPtr GPUBaseInstanceManager::GetDataPoolBuffer(int componentId) const
{
	ASSERT_MSG(m_componentPools[componentId], "GPUInstanceManager was not created with component ID = %d", componentId);
	return m_componentPools[componentId]->buffer;
}

static int instGranulatedCapacity(int capacity)
{
	constexpr int granularity = GPU_INSTANCE_POOL_SIZE_EXTEND;
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

	const int instancesPerWorkgroup = 64; // TODO: adjustable?
	const float rcpInstacesPerWorkgroup = 1.0f / instancesPerWorkgroup;

	computePass->DispatchWorkgroups(ceil(idxsCount * rcpInstacesPerWorkgroup));
	computePass->Complete();
}

// prepares data buffer
static void instPrepareDataBuffer(IGPUCommandRecorder* cmdRecorder, ArrayCRef<int> elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destDataBuffer)
{
	const int updatedCount = elementIds.numElem();
	const int updateBufferSize = updatedCount * elemSize;

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (const int elemIdx : elementIds)
	{
		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

// prepare data and indices
static void instPrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
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

void GPUBaseInstanceManager::SyncInstances(IGPUCommandRecorder* cmdRecorder)
{
	PROF_EVENT_F();

	bool buffersUpdatedThisFrame = false;

	Threading::CScopedMutex m(m_mutex);

	Array<int> elementIds(PP_SL);

	{
		const int oldBufferElems = m_rootBuffer ? m_rootBuffer->GetSize() / sizeof(InstRoot) : 0;

		// update instance root buffer
		// TODO: instead of re-creating buffer, create new separate buffer with it's instance pools
		if (!m_rootBuffer || m_instances.numElem() > oldBufferElems)
		{
			// alloc (or re-create) new buffer and upload entire data
			const int allocInstBufferElems = max(instGranulatedCapacity(m_instances.numElem()), GPU_INSTANCE_INITIAL_POOL_SIZE);

			// update roots, bloody roots
			{
				IGPUBufferPtr sourceBuffer = m_rootBuffer;
				m_rootBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(InstRoot), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstData");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_rootBuffer, 0, oldBufferElems * sizeof(InstRoot));
			}

			// update archetypes
			{
				IGPUBufferPtr sourceBuffer = m_archetypesBuffer;
				m_archetypesBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstArchetypes");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_archetypesBuffer, 0, oldBufferElems * sizeof(int));
			}

			// update single instances idxs
			{
				m_singleInstIndexBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), allocInstBufferElems + 1), BUFFERUSAGE_VERTEX | GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstIndices");

				elementIds.append(allocInstBufferElems);
				for (int i = 0; i < allocInstBufferElems; ++i)
					elementIds.append(i);
				cmdRecorder->WriteBuffer(m_singleInstIndexBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
			}
			buffersUpdatedThisFrame = true;
		}

		if (m_updated.size())
		{
			IGPUBufferPtr idxsBuffer;
			IGPUBufferPtr dataBuffer;
			instPrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(m_instances.ptr()), sizeof(Instance), sizeof(InstRoot), idxsBuffer, dataBuffer);
			instRunUpdatePipeline(cmdRecorder, m_updateRootPipeline, idxsBuffer, m_updated.size(), dataBuffer, m_rootBuffer);

			// update archetypes data
			instPrepareDataBuffer(cmdRecorder, ArrayCRef(elementIds.ptr()+1, elementIds.numElem()-1), reinterpret_cast<const ubyte*>(m_instances.ptr()) + offsetOf(Instance, archetype), sizeof(Instance), sizeof(int), dataBuffer);
			instRunUpdatePipeline(cmdRecorder, m_updateIntPipeline, idxsBuffer, elementIds.numElem()-1, dataBuffer, m_archetypesBuffer);
		}
		m_updated.clear();
	}

	// update instance components buffers
	for (GPUInstPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		const int elemSize = pool->stride;
		const int poolElems = pool->GetDataElems();
		const ubyte* poolDataPtr = reinterpret_cast<const ubyte*>(pool->GetDataPtr());

		const int oldBufferElems = pool->buffer ? pool->buffer->GetSize() / elemSize : 0;

		if (!pool->buffer || poolElems > oldBufferElems)
		{
			// alloc (or re-create) new buffer and upload entire data
			const int allocInstBufferElems = max(instGranulatedCapacity(poolElems), GPU_INSTANCE_INITIAL_POOL_SIZE);

			IGPUBufferPtr sourceBuffer = pool->buffer;
			pool->buffer = g_renderAPI->CreateBuffer(BufferInfo(elemSize, allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstData");
			buffersUpdatedThisFrame = true;

			// copy old contents of buffer
			if (oldBufferElems > 0)
				cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, pool->buffer, 0, oldBufferElems * elemSize);
		}
		
		if (pool->updated.size())
		{
			// prepare update buffer
			IGPUBufferPtr idxsBuffer;
			IGPUBufferPtr dataBuffer;
			instPrepareBuffers(cmdRecorder, pool->updated, elementIds, poolDataPtr, elemSize, elemSize, idxsBuffer, dataBuffer);
			instRunUpdatePipeline(cmdRecorder, pool->updatePipeline, idxsBuffer, pool->updated.size(), dataBuffer, pool->buffer);
		}
		pool->updated.clear();
	}

	for (int tempInstIdx : m_tempInstances)
		FreeInstance(tempInstIdx);

	m_tempInstances.clear();

	if(buffersUpdatedThisFrame)
		++m_buffersUpdated;
}

void GPUInstanceManagerDebug::DrawUI(GPUBaseInstanceManager& instMngBase)
{
#ifdef IMGUI_ENABLED
	ImGui::Text("Instances: %d", instMngBase.m_instances.numElem());
	ImGui::Text("Archetypes: %d", instMngBase.m_archetypeInstCounts.size());
	ImGui::Text("Buffer ref updates: %u", instMngBase.m_buffersUpdated);

	Array<int> sortedArchetypes(PP_SL);
	int maxInst = 0;
	for (auto it = instMngBase.m_archetypeInstCounts.begin(); !it.atEnd(); ++it)
	{
		maxInst = max(maxInst, *it);
		sortedArchetypes.append(it.key());
	}

	arraySort(sortedArchetypes, [&](const int archA, const int archB) {
		return instMngBase.m_archetypeInstCounts[archB] - instMngBase.m_archetypeInstCounts[archA];
	});

	EqString instName;
	for (const int archetypeId : sortedArchetypes)
	{
		const int instCount = instMngBase.m_archetypeInstCounts[archetypeId];
		const auto nameIt = instMngBase.m_archetypeNames.find(archetypeId);
		if (!nameIt.atEnd())
			instName = *nameIt;
		else
			instName = EqString::Format("%d", archetypeId);

		EqString str = EqString::Format("%s : %d", instName, instCount);
		ImGui::ProgressBar(instCount / (float)maxInst, ImVec2(0.f, 0.f), str);
	}
#endif // IMGUI_ENABLED
}