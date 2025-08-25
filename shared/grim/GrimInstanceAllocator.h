//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances allocator
//////////////////////////////////////////////////////////////////////////////////

/*
Example of use:

	1. Define your instance components:
		// in header:
		struct InstTransform {
			DEFINE_GPU_INSTANCE_COMPONENT(GPUINST_PROP_ID_TRANSFORM, InstTransform);

			Quaternion	orientation{ qidentity };
			Vector3D	position{ vec3_zero };
			float		boundingSphere{ 1.0f };
		};

		struct InstScale {
			DEFINE_GPU_INSTANCE_COMPONENT(GPUINST_PROP_ID_SCALE, InstScale);
			...
		};

		// in source file:
		INIT_GPU_INSTANCE_COMPONENT(InstTransform, "InstanceShaderName")
		INIT_GPU_INSTANCE_COMPONENT(InstScale, "InstanceShaderName")

	2. Define instancer type
		using DemoInstanceAllocator = GRIMInstanceAllocator<InstTransform>;

	3. Declare instancer and initialize
		static DemoInstanceAllocator g_instanceMng;
		...
		s_instanceAlloc.Initialize();

	4. Object management
		// creates instance
		int instanceID = g_instanceMng.AddInstance<InstTransform>();

		// updates instance data
		InstTransform myTransform{...};
		InstScale myScale{...};
		g_instanceMng.Set(instanceID, myTransform, myScale);

		// remove instance from manager
		g_instanceMng.FreeInstance(instanceID);

	5. Drawing objects
		// before any drawing and after objects are updated
		s_instanceAlloc.SyncInstances(cmdBuffer);

		// when performing draw calls
		BindGroupDesc instBindGroup = Builder<BindGroupDesc>()
			.GroupIndex(bindGroupIdx)
			.Buffer(0, g_instanceMng.GetRootBuffer())	// ROOTs are always required
			.Buffer(1, g_instanceMng.GetDataPoolBuffer(InstTransform::COMPONENT_ID))
			.Buffer(2, g_instanceMng.GetDataPoolBuffer(InstScale::COMPONENT_ID))

		rendPassRecorder->SetVertexBuffer(instBufferIdx, instancesBuffer);
		rendPassRecorder->SetBindGroup(bindGroupIdx, instBindGroup);
*/

#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimDefs.h"
#include "GrimComponentPool.h"

class GRIMBaseComponentPool;

static constexpr int GRIM_DEFAULT_INST_INITIAL_POOL_SIZE = 3072;
static constexpr int GRIM_DEFAULT_INST_POOL_SIZE_EXTEND = 1024;


// The instance manager basic implementation
class GRIMBaseInstanceAllocator
{
	friend class GRIMInstanceDebug;
public:
	GRIMBaseInstanceAllocator() = default;
	~GRIMBaseInstanceAllocator() = default;

	void			Initialize(const char* instanceComputeShaderName, int instancesReserve = 1000);
	void			Shutdown();

	void			FreeAll(bool dealloc = false, bool reserve = false);

	GPUBufferView	GetSingleInstanceIndexBuffer() const;

	IGPUBufferPtr	GetRootBuffer() const { return m_rootBuffer; }
	IGPUBufferPtr	GetInstanceArchetypesBuffer() const { return m_archetypesBuffer; }
	IGPUBufferPtr	GetInstanceGroupMaskBuffer() const { return m_groupMaskBuffer; }

	uint			GetBufferUpdateToken() const { return m_buffersUpdated; }

	int				GetInstanceSlotsCount() const { return m_instances.numElem(); }
	int				GetInstanceCount() const { return m_instances.numElem() - m_freeIndices.numElem() - 1; }
	int				GetInstanceCount(GRIMArchetype archetypeId) const;

	GRIMArchetype	GetInstanceArchetypeId(int instanceIdx) const;
	int				GetInstanceGroupMask(int instanceIdx) const;
	bool			GetInstanceIsSync(int instanceIdx) const;
	int				GetInstanceComponentIdx(int instanceIdx, int componentId) const;

	// syncs instance buffers with GPU and updates roots buffer
	void			SyncInstances(IGPUCommandRecorder* cmdRecorder);

	// changes instance archetype (in case of body group changes etc)
	void			SetArchetype(GRIMInstanceRef instanceRef, GRIMArchetype newArchetype);
	void			SetGroupMask(GRIMInstanceRef instanceRef, int groupMask);

	// destroys instance and it's components
	void			FreeInstance(GRIMInstanceRef instanceRef);

protected:
	void			Construct();
	void			DbgInvalidateAllData();

	// TODO: instance refs
	// in this way we can use data of the referenced intance on new instance with different
	// archetypes, useful for adding body groups (as they use different archetype id)
	// int			AllocInstanceRef(int archetype, int refInstanceId);
	GRIMInstanceRef	AllocInstance(GRIMArchetype archetype);
	GRIMInstanceRef	AllocTempInstance(GRIMArchetype archetype);

	struct InstRoot
	{
		uint32	components[GRIM_INSTANCE_MAX_COMPONENTS]{ 0 };
	};

	struct Instance
	{
		enum EUpdateFlags
		{
			UPD_ROOT		= (1 << 0),
			UPD_ARCHETYPE	= (1 << 1),
			UPD_GROUPMASK	= (1 << 2),
			UPD_ALL			= 0xff
		};

		InstRoot		root;
		GRIMArchetype	archetype{ GRIM_INVALID_ARCHETYPE };
		GRIMArchetype	lastSyncArchetype{ GRIM_INVALID_ARCHETYPE };	// only needed for ref counting
		uint			groupMask{ COM_UINT_MAX };
		int				updateFlags{ UPD_ALL };
	};

	mutable Threading::CEqReadWriteLock		m_rwLock;
	IGPUBufferPtr			m_rootBuffer;
	IGPUBufferPtr			m_archetypesBuffer;
	IGPUBufferPtr			m_groupMaskBuffer;

	IGPUBufferPtr			m_singleInstIndexBuffer;

	IGPUComputePipelinePtr	m_updateRootPipeline;
	IGPUComputePipelinePtr	m_updateIntPipeline;

	Array<GRIMInstanceRef>	m_tempInstances{ PP_SL };

	Array<Instance>			m_instances{ PP_SL };
	Array<int>				m_freeIndices{ PP_SL };
	Set<int>				m_updated{ PP_SL };
	BitArray				m_syncInstances{ PP_SL };

	Array<int>				m_instSyncArchetypes{ PP_SL };
	Array<int>				m_instSyncRoots{ PP_SL };
	Array<int>				m_instSyncGroupMask{ PP_SL };

	GRIMBaseComponentPool*	m_componentPools[GRIM_INSTANCE_MAX_COMPONENTS]{ nullptr };
	uint					m_buffersUpdated{ 0 };

	Map<GRIMArchetype, int>	m_archetypeRefCount{ PP_SL };

	int						m_reservedInsts{ 0 };
};

//-----------------------------------------------------
// Below is a template part of Instance manager. Use it

// Component-based instance manager
template<typename ... Components>
class GRIMInstanceAllocator : public GRIMBaseInstanceAllocator
{
public:
	GRIMInstanceAllocator();

	// creates new empty instance with allocated components
	template<typename ...TComps>
	int 			AddInstance(int archetype)
	{
		Threading::CScopedWriteLocker m(m_rwLock);
		const int instanceId = AllocInstance(archetype);
		AllocInstanceComponents<TComps...>(instanceId);
		return instanceId;
	}

	// creates new temporary empty instance with allocated components
	template<typename ...TComps>
	int 			AddTempInstance(int archetype)
	{
		Threading::CScopedWriteLocker m(m_rwLock);
		const int instanceId = AllocTempInstance(archetype);
		AllocInstanceComponents<TComps...>(instanceId);
		return instanceId;
	}

	// sets component values on instance
	template<typename...TComps>
	void			Set(int instanceId, const TComps&... values);

	// adds new component
	template<typename TComp>
	void			Add(int instanceId);

	// removes existing component
	template<typename TComp>
	void			Remove(int instanceId);

	template<typename TComp>
	bool			Has(int instanceId) const;

	template<typename TComp>
	auto&			GetComponentPool() { return std::get<typename TComp::POOL_T>(m_componentPoolsStorage); }

protected:
	using POOL_STORAGE = std::tuple<typename Components::POOL_T...>;

	template<typename ...TComps>
	void 			AllocInstanceComponents(int instanceId);

	// sets component values on instance
	template<typename First, typename...Rest>
	void			SetInternal(InstRoot& inst, const First& firstVal, const Rest&... values);

	// sets component values on instance
	void			SetInternal(InstRoot& inst) {}

	template<std::size_t... Is>
	void			InitPools(std::index_sequence<Is...>);

	POOL_STORAGE	m_componentPoolsStorage;
};

//-------------------------------

template<typename...Ts>
template<std::size_t... Is>
void GRIMInstanceAllocator<Ts...>::InitPools(std::index_sequence<Is...>)
{
	([&] {
		using POOL_TYPE = std::tuple_element_t<Is, POOL_STORAGE>;
		m_componentPools[POOL_TYPE::TYPE::COMPONENT_ID] = &std::get<Is>(m_componentPoolsStorage);
	}(), ...);
}

template<typename...Ts>
inline GRIMInstanceAllocator<Ts...>::GRIMInstanceAllocator()
{
	InitPools(std::index_sequence_for<Ts...>{});
	Construct();
}

template<typename...Ts>
template<typename First, typename...Rest>
inline void GRIMInstanceAllocator<Ts...>::SetInternal(InstRoot& inst, const First& firstVal, const Rest&... values)
{
	using Pool = typename First::POOL_T;
	const uint32 inPoolIdx = inst.components[First::COMPONENT_ID];

	// don't update invalid or default
	if (inPoolIdx == COM_UINT_MAX || inPoolIdx == 0)
	{
		SetInternal(inst, values...);
		return; // Instance was not allocated with specified component, so skipping
	}

	Pool& compPool = GetComponentPool<First>();
	if (!memcmp(&compPool.Get(inPoolIdx), &firstVal, sizeof(First)))
	{
		SetInternal(inst, values...);
		return;
	}

	compPool.Update(inPoolIdx, firstVal);
	SetInternal(inst, values...);
}

template<typename...Ts>
template<typename...TComps>
inline void GRIMInstanceAllocator<Ts...>::Set(int instanceId, const TComps&... values)
{
	if (instanceId == -1)
		return;
	Threading::CScopedReadLocker m(m_rwLock);
	SetInternal(m_instances[instanceId].root, values...);
}

// creates new empty instance with allocated components
template<typename...Ts>
template<typename...TComps>
inline void GRIMInstanceAllocator<Ts...>::AllocInstanceComponents(int instanceId)
{
	InstRoot& inst = m_instances[instanceId].root;
	([&]{
		using Pool = typename TComps::POOL_T;
		Pool& compPool = GetComponentPool<TComps>();
		inst.components[TComps::COMPONENT_ID] = compPool.Add(TComps{});
	} (), ...);

	// UPD_ROOT is already set
}

template<typename...Ts>
template<typename TComp>
void GRIMInstanceAllocator<Ts...>::Add(int instanceId)
{
	using Pool = typename TComp::POOL_T;

	if (instanceId == -1)
		return;

	Instance& inst = m_instances[instanceId];
	InstRoot& root = inst.root;
	{
		Threading::CScopedReadLocker m(m_rwLock);
		if (root.components[TComp::COMPONENT_ID] > 0 && root.components[TComp::COMPONENT_ID] != COM_UINT_MAX)
			return;
	}

	Pool& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedWriteLocker m(m_rwLock);
		root.components[TComp::COMPONENT_ID] = compPool.Add(TComp{});

		inst.updateFlags |= Instance::UPD_ROOT;
		m_updated.insert(instanceId);
	}
}

template<typename...Ts>
template<typename TComp>
void GRIMInstanceAllocator<Ts...>::Remove(int instanceId)
{
	using Pool = typename TComp::POOL_T;

	if (instanceId == -1)
		return;

	Instance& inst = m_instances[instanceId];
	InstRoot& root = inst.root;
	{
		Threading::CScopedReadLocker m(m_rwLock);
		if (root.components[TComp::COMPONENT_ID] == 0 || root.components[TComp::COMPONENT_ID] == COM_UINT_MAX)
			return;
	}

	Pool& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedWriteLocker m(m_rwLock);
		compPool.Remove(root.components[TComp::COMPONENT_ID]);
		root.components[TComp::COMPONENT_ID] = 0; // change to default

		inst.updateFlags |= Instance::UPD_ROOT;
		m_updated.insert(instanceId);
	}
}

template<typename...Ts>
template<typename TComp>
bool GRIMInstanceAllocator<Ts...>::Has(int instanceId) const
{
	if (instanceId == -1)
		return false;

	Threading::CScopedReadLocker m(m_rwLock);
	const Instance& inst = m_instances[instanceId];
	const InstRoot& root = inst.root;
	if (root.components[TComp::COMPONENT_ID] == 0 || root.components[TComp::COMPONENT_ID] == COM_UINT_MAX)
		return false;
	return true;
}

