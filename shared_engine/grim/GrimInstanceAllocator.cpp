//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances allocator
//////////////////////////////////////////////////////////////////////////////////

#include <imgui.h>
#include "core/core_common.h"
#include "core/ConVar.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimInstanceAllocator.h"

static constexpr int GPU_INSTANCE_INITIAL_POOL_SIZE		= 3072;
static constexpr int GPU_INSTANCE_POOL_SIZE_EXTEND		= 1024;
static constexpr int GPU_INSTANCE_MAX_TEMP_INSTANCES	= 128;

static constexpr int GPU_INSTANCE_BUFFER_USAGE_FLAGS = BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST | BUFFERUSAGE_COPY_SRC;

static int instGranulatedCapacity(int capacity)
{
	return GRIMBaseSyncrhronizedPool::GetGranulatedCapacity(capacity, GPU_INSTANCE_POOL_SIZE_EXTEND);
}

//---------------------------------------------------------------------

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
	m_archetypeInstCounts.clear(dealloc);
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

GPUBufferView GRIMBaseInstanceAllocator::GetSingleInstanceIndexBuffer() const
{
	return GPUBufferView(m_singleInstIndexBuffer, sizeof(int));
}

int	GRIMBaseInstanceAllocator::GetInstanceCountByArchetype(GRIMArchetype archetypeId) const
{
	auto it = m_archetypeInstCounts.find(archetypeId);
	if (it.atEnd())
		return 0;

	return *it;
}

void GRIMBaseInstanceAllocator::DbgRegisterArchetypeName(GRIMArchetype archetypeId, const char* name)
{
#ifdef ENABLE_GPU_INSTANCE_DEBUG
	Threading::CScopedMutex m(m_mutex);
	if(m_archetypeNames.find(archetypeId).atEnd())
		m_archetypeNames.insert(archetypeId, name);
#endif
}

GRIMInstanceRef	GRIMBaseInstanceAllocator::AllocInstance(GRIMArchetype archetypeId)
{
	Threading::CScopedMutex m(m_mutex);
	const GRIMInstanceRef instanceRef = m_freeIndices.numElem() ? m_freeIndices.popBack() : m_instances.append({});

	if (archetypeId != GRIM_INVALID_ARCHETYPE)
	{
		auto it = m_archetypeInstCounts.find(archetypeId);
		if (it.atEnd())
			it = m_archetypeInstCounts.insert(archetypeId, 0);
		++(*it);
	}

	Instance& inst = m_instances[instanceRef];
	inst.archetype = archetypeId;
	memset(&inst.root.components, 0, sizeof(inst.root.components));

	m_updated.insert(instanceRef);
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
		Threading::CScopedMutex m(m_mutex);

		Instance& inst = m_instances[instanceRef];
		const GRIMArchetype oldArchetype = inst.archetype;
		inst.archetype = newArchetype;
		m_updated.insert(instanceRef);

		if(oldArchetype != GRIM_INVALID_ARCHETYPE)
		{
			auto oldIt = m_archetypeInstCounts.find(oldArchetype);
			if (!oldIt.atEnd())
				--(*oldIt);
		}

		if (newArchetype != GRIM_INVALID_ARCHETYPE)
		{
			auto newIt = m_archetypeInstCounts.find(newArchetype);
			if (newIt.atEnd())
				newIt = m_archetypeInstCounts.insert(newArchetype, 0);
			++(*newIt);
		}
	}
}

// destroys instance and it's components
void GRIMBaseInstanceAllocator::FreeInstance(GRIMInstanceRef instanceRef)
{
	if (!m_instances.inRange(instanceRef))
		return;

	Threading::CScopedMutex m(m_mutex);

	Instance& inst = m_instances[instanceRef];
	InstRoot& root = inst.root;
	{
		m_freeIndices.append(instanceRef);

		if (inst.archetype != GRIM_INVALID_ARCHETYPE)
		{
			auto it = m_archetypeInstCounts.find(inst.archetype);
			if (!it.atEnd())
			{
				--(*it);
				ASSERT_MSG(*it >= 0, "Archetype counter is invalid (%d)", *it);
			}
		}
	}

	inst.archetype = GRIM_INVALID_ARCHETYPE;

	for (int i = 0; i < GRIM_INSTANCE_MAX_COMPONENTS; ++i)
	{
		// skip invalid or defaults
		if (root.components[i] == UINT_MAX)
			continue;

		// add this instance to freed list and invalidate ID
		if(root.components[i] > 0)
		{
			Threading::CScopedMutex m(m_mutex);
			m_componentPools[i]->GetData().Remove(root.components[i]);
		}
		root.components[i] = UINT_MAX;
	}

	// update roots and archetypes
	{
		Threading::CScopedMutex m(m_mutex);
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
				m_archetypesBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(GRIMArchetype), allocInstBufferElems), GPU_INSTANCE_BUFFER_USAGE_FLAGS, "InstArchetypes");

				if (oldBufferElems > 0)
					cmdRecorder->CopyBufferToBuffer(sourceBuffer, 0, m_archetypesBuffer, 0, oldBufferElems * sizeof(GRIMArchetype));
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
			GRIMBaseSyncrhronizedPool::PrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(m_instances.ptr()), sizeof(Instance), sizeof(InstRoot), idxsBuffer, dataBuffer);
			GRIMBaseSyncrhronizedPool::RunUpdatePipeline(cmdRecorder, m_updateRootPipeline, idxsBuffer, m_updated.size(), dataBuffer, m_rootBuffer);

			// update archetypes data
			const int numInstArchetypes = elementIds.numElem()-1;
			const ubyte* archetypesSrcAddr = reinterpret_cast<const ubyte*>(m_instances.ptr()) + offsetOf(Instance, archetype);
			GRIMBaseSyncrhronizedPool::PrepareDataBuffer(cmdRecorder, ArrayCRef(elementIds.ptr()+1, numInstArchetypes), archetypesSrcAddr, sizeof(Instance), sizeof(GRIMArchetype), dataBuffer);
			GRIMBaseSyncrhronizedPool::RunUpdatePipeline(cmdRecorder, m_updateIntPipeline, idxsBuffer, numInstArchetypes, dataBuffer, m_archetypesBuffer);
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

void GRIMInstanceDebug::DrawUI(GRIMBaseInstanceAllocator& instMngBase)
{
#ifdef IMGUI_ENABLED
	ImGui::Text("Instances: %d", instMngBase.m_instances.numElem());
	ImGui::Text("Archetypes: %d", instMngBase.m_archetypeInstCounts.size());
	ImGui::Text("Buffer ref updates: %u", instMngBase.m_buffersUpdated);

	Array<GRIMArchetype> sortedArchetypes(PP_SL);
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
	for (const GRIMArchetype archetypeId : sortedArchetypes)
	{
		const int instCount = instMngBase.m_archetypeInstCounts[archetypeId];
		const auto nameIt = instMngBase.m_archetypeNames.find(archetypeId);
		if (!nameIt.atEnd())
			instName = *nameIt;
		else
			instName = EqString::Format("%d", archetypeId);

		EqString str = EqString::Format("[%d] %d %s", instCount, archetypeId, instName);
		ImGui::ProgressBar(instCount / (float)maxInst, ImVec2(0.f, 0.f), str);
	}
#endif // IMGUI_ENABLED
}