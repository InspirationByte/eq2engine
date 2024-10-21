//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances allocator
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimInstanceAllocator.h"

using namespace Threading;


static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE		= 3072;
static constexpr int GPU_INSTANCE_POOL_SIZE_EXTEND		= 1024;
static constexpr int GPU_INSTANCE_MAX_TEMP_INSTANCES	= 128;

static constexpr int GPU_INSTANCE_BUFFER_USAGE_FLAGS = BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST | BUFFERUSAGE_COPY_SRC;

static int instGranulatedCapacity(int capacity)
{
	return GRIMBaseSyncrhronizedPool::GetGranulatedCapacity(capacity, GPU_INSTANCE_POOL_SIZE_EXTEND);
}

//---------------------------------------------------------------------

Threading::CEqMutex& GRIMBaseInstanceAllocator::GetMutex()
{
	static CEqMutex s_grimAllocMutex;
	return s_grimAllocMutex;
}

void GRIMBaseInstanceAllocator::Construct()
{
	// alloc default (zero) instance
	m_instances.append();
	m_updated.insert(0);

	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		// alloc default (zero) instance
		pool->AddElem();
	}
}

void GRIMBaseInstanceAllocator::Initialize(const char* instanceComputeShaderName, int instancesReserve)
{
	m_reservedInsts = instancesReserve;

	if (!m_updateRootPipeline)
	{
		m_updateRootPipeline = g_renderAPI->CreateComputePipeline(
			Builder<ComputePipelineDesc>()
			.ShaderName(instanceComputeShaderName)
			.ShaderLayoutId(StringIdConst24("InstRoot"))
			.End()
		);
	}

	if (!m_updateIntPipeline)
	{
		m_updateIntPipeline = g_renderAPI->CreateComputePipeline(
			Builder<ComputePipelineDesc>()
			.ShaderName(instanceComputeShaderName)
			.ShaderLayoutId(StringIdConst24("InstInt"))
			.End()
		);
	}

	m_instances.reserve(m_reservedInsts);

	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool || pool->GetData().IsValid())
			continue;

		GRIMBaseSyncrhronizedPool& data = pool->GetData();
		data.Init(0, GPU_INSTANCE_INITIAL_POOL_SIZE, GPU_INSTANCE_POOL_SIZE_EXTEND);
		data.Reserve(m_reservedInsts);

		pool->InitPipeline();
		ASSERT_MSG(data.IsValid(), "Failed to create instance update pipeline");
	}
}

void GRIMBaseInstanceAllocator::Shutdown()
{
	FreeAll(true);

	m_updateRootPipeline = nullptr;
	m_updateIntPipeline = nullptr;

	m_rootBuffer = nullptr;
	m_archetypesBuffer = nullptr;
	m_groupMaskBuffer = nullptr;
	m_singleInstIndexBuffer = nullptr;

	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool)
			continue;
		pool->GetData().SetPipeline(nullptr);
	}
}

void GRIMBaseInstanceAllocator::FreeAll(bool dealloc, bool reserve)
{
#ifdef ENABLE_GPU_INSTANCE_DEBUG
	m_archetypeNames.clear(dealloc);
#endif
	m_updated.clear(dealloc);
	m_freeIndices.clear(dealloc);
	m_tempInstances.clear(dealloc);
	m_archetypeRefCount.clear(dealloc);
	m_instances.clear(dealloc);

	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool)
			continue;
		pool->GetData().Clear(dealloc);
	}

	// alloc default (zero) instance
	m_instances.setNum(1);
	m_updated.insert(0);

	if (reserve)
		m_instances.reserve(m_reservedInsts);

	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool)
			continue;

		// alloc default (zero) instance
		pool->AddElem();

		if(reserve)
			pool->GetData().Reserve(m_reservedInsts);
	}
}

void GRIMBaseInstanceAllocator::DbgInvalidateAllData()
{
	CScopedMutex m(GetMutex());
	for(int i = 0; i < m_instances.numElem(); ++i)
	{
		m_instances[i].updateFlags |= Instance::UPD_ALL;
		m_updated.insert(i);
	}
	
	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if(!pool)
			continue;
		for(int i = 0; i < pool->GetData().NumSlots(); ++i)
			pool->GetData().SetUpdated(i);
	}
	m_buffersUpdated = 0;
}

GPUBufferView GRIMBaseInstanceAllocator::GetSingleInstanceIndexBuffer() const
{
	return GPUBufferView(m_singleInstIndexBuffer, sizeof(int));
}

int	GRIMBaseInstanceAllocator::GetInstanceCount(GRIMArchetype archetypeId) const
{
	auto it = m_archetypeRefCount.find(archetypeId);
	if (it.atEnd())
		return 0;
	return *it;
}

GRIMArchetype GRIMBaseInstanceAllocator::GetInstanceArchetypeId(int instanceIdx) const
{
	return m_instances[instanceIdx].archetype;
}

int GRIMBaseInstanceAllocator::GetInstanceGroupMask(int instanceIdx) const
{
	return m_instances[instanceIdx].groupMask;
}

bool GRIMBaseInstanceAllocator::GetInstanceIsSync(int instanceIdx) const
{
	return m_syncInstances[instanceIdx];
}

int GRIMBaseInstanceAllocator::GetInstanceComponentIdx(int instanceIdx, int componentId) const
{
	return m_instances[instanceIdx].root.components[componentId];
}

GRIMInstanceRef	GRIMBaseInstanceAllocator::AllocInstance(GRIMArchetype archetypeId)
{
	CScopedMutex m(GetMutex());
	const GRIMInstanceRef instanceRef = m_freeIndices.numElem() ? m_freeIndices.popBack() : m_instances.append({});

	if (archetypeId != GRIM_INVALID_ARCHETYPE)
	{
		auto it = m_archetypeRefCount.find(archetypeId);
		if (it.atEnd())
			it = m_archetypeRefCount.insert(archetypeId, 0);
		++(*it);
	}

	if (m_instances.numElem() + 1 > m_syncInstances.numBits())
		m_syncInstances.resize(m_instances.numElem() + 1);

	Instance& inst = m_instances[instanceRef];
	inst.archetype = archetypeId;
	inst.updateFlags |= Instance::UPD_ALL;
	memset(&inst.root.components, 0, sizeof(inst.root.components));

	m_updated.insert(instanceRef);
	m_syncInstances.setFalse(instanceRef);

	return instanceRef;
}

int GRIMBaseInstanceAllocator::AllocTempInstance(int archetype)
{
	if (m_tempInstances.numElem() >= GPU_INSTANCE_MAX_TEMP_INSTANCES)
		return -1;

	const GRIMInstanceRef instRef = AllocInstance(archetype);
	m_tempInstances.append(instRef);
	return instRef;
}

// sets batches that are drrawn with particular instance
void GRIMBaseInstanceAllocator::SetArchetype(GRIMInstanceRef instanceRef, GRIMArchetype newArchetype)
{
	if (!m_instances.inRange(instanceRef))
		return;

	{
		CScopedMutex m(GetMutex());

		Instance& inst = m_instances[instanceRef];
		const GRIMArchetype oldArchetype = inst.archetype;
		if (oldArchetype != newArchetype)
		{
			inst.archetype = newArchetype;
			inst.updateFlags |= Instance::UPD_ARCHETYPE;

			m_updated.insert(instanceRef);
			m_syncInstances.setFalse(instanceRef);

			if (oldArchetype != GRIM_INVALID_ARCHETYPE)
			{
				auto oldIt = m_archetypeRefCount.find(oldArchetype);
				if (!oldIt.atEnd())
					--(*oldIt);
			}

			if (newArchetype != GRIM_INVALID_ARCHETYPE)
			{
				auto newIt = m_archetypeRefCount.find(newArchetype);
				if (newIt.atEnd())
					newIt = m_archetypeRefCount.insert(newArchetype, 0);
				++(*newIt);
			}
		}
	}
}

void GRIMBaseInstanceAllocator::SetGroupMask(GRIMInstanceRef instanceRef, int groupMask)
{
	if (!m_instances.inRange(instanceRef))
		return;

	{
		CScopedMutex m(GetMutex());

		Instance& inst = m_instances[instanceRef];
		if(inst.groupMask != groupMask)
		{
			inst.groupMask = groupMask;
			inst.updateFlags |= Instance::UPD_GROUPMASK;

			m_updated.insert(instanceRef);
			m_syncInstances.setFalse(instanceRef);
		}
	}
}

// destroys instance and it's components
void GRIMBaseInstanceAllocator::FreeInstance(GRIMInstanceRef instanceRef)
{
	if (!m_instances.inRange(instanceRef))
		return;

	CScopedMutex m(GetMutex());

	Instance& inst = m_instances[instanceRef];
	InstRoot& root = inst.root;
	{
		m_freeIndices.append(instanceRef);

		if (inst.archetype != GRIM_INVALID_ARCHETYPE)
		{
			auto it = m_archetypeRefCount.find(inst.archetype);
			if (!it.atEnd())
			{
				--(*it);
				ASSERT_MSG(*it >= 0, "Archetype counter is invalid (%d)", *it);
			}
		}
	}

	inst.archetype = GRIM_INVALID_ARCHETYPE;
	inst.updateFlags |= Instance::UPD_ARCHETYPE;

	for (int i = 0; i < GRIM_INSTANCE_MAX_COMPONENTS; ++i)
	{
		// skip invalid or defaults
		if (root.components[i] == COM_UINT_MAX)
			continue;

		// add this instance to freed list and invalidate ID
		if(root.components[i] > 0)
		{
			CScopedMutex m(GetMutex());
			m_componentPools[i]->GetData().Remove(root.components[i]);
		}
		root.components[i] = COM_UINT_MAX;
	}

	// update roots and archetypes
	{
		CScopedMutex m(GetMutex());
		m_updated.insert(instanceRef);
	}
}

IGPUBufferPtr GRIMBaseInstanceAllocator::GetDataPoolBuffer(int componentId) const
{
	ASSERT_MSG(m_componentPools[componentId], "GPUInstanceManager was not created with component ID = %d", componentId);
	return m_componentPools[componentId]->GetData().GetBuffer();
}

void GRIMBaseInstanceAllocator::SyncInstances(IGPUCommandRecorder* cmdRecorder)
{
	PROF_EVENT_F();

	bool buffersUpdatedThisFrame = false;

	CScopedMutex m(GetMutex());
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
				m_rootBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(InstRoot), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstRoot");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_rootBuffer, 0, oldBufferElems * sizeof(InstRoot));
			}

			// update archetypes
			{
				IGPUBufferPtr sourceBuffer = m_archetypesBuffer;
				m_archetypesBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GRIMArchetype), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "GRIMArchetype");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_archetypesBuffer, 0, oldBufferElems * sizeof(GRIMArchetype));
			}

			// update groupMask
			{
				IGPUBufferPtr sourceBuffer = m_groupMaskBuffer;
				m_groupMaskBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GRIMArchetype), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstGroupMask");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_groupMaskBuffer, 0, oldBufferElems * sizeof(GRIMArchetype));
			}

			// update single instances idxs
			{
				Array<int> elementIds(PP_SL);
				elementIds.reserve(allocInstBufferElems + 1);
				elementIds.append(allocInstBufferElems);
				for (int i = 0; i < allocInstBufferElems; ++i)
					elementIds.append(i);

				m_singleInstIndexBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(int), allocInstBufferElems + 1), BUFFERUSAGE_VERTEX | GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstIds");
				cmdRecorder->WriteBuffer(m_singleInstIndexBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
			}
			buffersUpdatedThisFrame = true;
		}

		if (m_updated.size())
		{
			Array<int> instUpdateArchetypes(PP_SL);
			Array<int> instUpdateRoots(PP_SL);
			Array<int> instUpdateGroupMask(PP_SL);
			instUpdateArchetypes.reserve(m_updated.size() + 1);
			instUpdateRoots.reserve(m_updated.size() + 1);
			instUpdateGroupMask.reserve(m_updated.size() + 1);

			for (auto it = m_updated.begin(); !it.atEnd(); ++it)
			{
				const int instanceIdx = it.key();
				const int updateFlags = m_instances[instanceIdx].updateFlags;

				if(updateFlags & Instance::UPD_ARCHETYPE)
					instUpdateArchetypes.append(instanceIdx);

				if (updateFlags & Instance::UPD_ROOT)
					instUpdateRoots.append(instanceIdx);

				if (updateFlags & Instance::UPD_GROUPMASK)
					instUpdateGroupMask.append(instanceIdx);

				m_instances[instanceIdx].updateFlags = 0;
				m_syncInstances.setTrue(instanceIdx);
			}

			const ubyte* srcAddr = reinterpret_cast<const ubyte*>(m_instances.ptr());
			const ubyte* rootSrcAddr = srcAddr + offsetOf(Instance, root);
			const ubyte* archetypesSrcAddr = srcAddr + offsetOf(Instance, archetype);
			const ubyte* groupMaskSrcAddr = srcAddr + offsetOf(Instance, groupMask);

			auto UpdateInstanceItems = [cmdRecorder](IGPUComputePipeline* pipeline, IGPUBuffer* targetBuffer, Array<int>& itemIds, const ubyte* items, int itemSize) {
				const int idxsCount = itemIds.numElem();
				if (!idxsCount)
					return;

				itemIds.insert(idxsCount, 0);
				IGPUBufferPtr idxsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(itemIds[0]), itemIds.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdIdxs");
				cmdRecorder->WriteBuffer(idxsBuffer, itemIds.ptr(), sizeof(itemIds[0]) * itemIds.numElem(), 0);

				IGPUBufferPtr dataBuffer;
				GRIMBaseSyncrhronizedPool::PrepareDataBuffer(cmdRecorder, itemIds, items, sizeof(Instance), itemSize, dataBuffer);
				GRIMBaseSyncrhronizedPool::RunUpdatePipeline(cmdRecorder, pipeline, idxsBuffer, idxsCount, dataBuffer, targetBuffer);
			};

			// update roots
			UpdateInstanceItems(m_updateRootPipeline, m_rootBuffer, instUpdateRoots, rootSrcAddr, sizeof(InstRoot));

			// update archetypes data
			UpdateInstanceItems(m_updateIntPipeline, m_archetypesBuffer, instUpdateArchetypes, archetypesSrcAddr, sizeof(GRIMArchetype));

			// update groupMask
			UpdateInstanceItems(m_updateIntPipeline, m_groupMaskBuffer, instUpdateGroupMask, groupMaskSrcAddr, sizeof(int));
		}
		m_updated.clear();
	}

	// update instance components buffers
	for (GRIMBaseComponentPool* pool : m_componentPools)
	{
		if (!pool)
			continue;
		
		if(pool->GetData().Sync(cmdRecorder))
			buffersUpdatedThisFrame = true;
	}

	for (int tempInstIdx : m_tempInstances)
		FreeInstance(tempInstIdx);

	m_tempInstances.clear();

	if(buffersUpdatedThisFrame)
		++m_buffersUpdated;
}
