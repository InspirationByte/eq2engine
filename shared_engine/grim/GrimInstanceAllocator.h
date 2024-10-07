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
#include "GrimSynchronizedPool.h"

#define DEFINE_GPU_INSTANCE_COMPONENT(ID, Name) \
	static constexpr const char* NAME = #Name; \
	static constexpr int IDENTIFIER = StringToHashConst(#Name); \
	static constexpr int COMPONENT_ID = ID; \
	static void InitPipeline(GRIMBaseSyncrhronizedPool& pool);

#define INIT_GPU_INSTANCE_COMPONENT(Name, UpdateShaderName) \
	void Name::InitPipeline(GRIMBaseSyncrhronizedPool& pool) { \
		pool.SetPipeline(g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>() \
			.ShaderName(UpdateShaderName).ShaderLayoutId(IDENTIFIER).End())); \
	}

class GRIMBaseComponentPool
{
public:
	virtual GRIMBaseSyncrhronizedPool& GetData() = 0;
	virtual void AddElem() = 0;
	virtual void InitPipeline() = 0;
};

// The instance manager basic implementation
class GRIMBaseInstanceAllocator
{
	friend class GRIMInstanceDebug;
public:
	static Threading::CEqMutex& GetMutex();

	GRIMBaseInstanceAllocator() = default;
	~GRIMBaseInstanceAllocator() = default;

	void			Initialize(const char* instanceComputeShaderName, int instancesReserve = 1000);
	void			Shutdown();

	void			FreeAll(bool dealloc = false, bool reserve = false);

	GPUBufferView	GetSingleInstanceIndexBuffer() const;
	IGPUBufferPtr	GetInstanceArchetypesBuffer() const { return m_archetypesBuffer; }
	IGPUBufferPtr	GetRootBuffer() const { return m_rootBuffer; }
	IGPUBufferPtr	GetDataPoolBuffer(int componentId) const;
	int				GetInstanceCountByArchetype(GRIMArchetype archetypeId) const;
	GRIMArchetype	GetInstanceArchetypeId(int instanceIdx) const { return m_instances[instanceIdx].archetype; }
	int				GetInstanceComponentIdx(int instanceIdx, int componentId) const { return m_instances[instanceIdx].root.components[componentId]; }
	uint			GetBufferUpdateToken() const { return m_buffersUpdated; }

	int				GetInstanceSlotsCount() const { return m_instances.numElem(); }
	int				GetInstanceCount() const { return m_instances.numElem() - m_freeIndices.numElem(); }

	// syncs instance buffers with GPU and updates roots buffer
	void			SyncInstances(IGPUCommandRecorder* cmdRecorder);

	// changes instance archetype (in case of body group changes etc)
	void			SetArchetype(GRIMInstanceRef instanceRef, GRIMArchetype newArchetype);

	// destroys instance and it's components
	void			FreeInstance(GRIMInstanceRef instanceRef);

protected:
	void			Construct();

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
		InstRoot		root;
		GRIMArchetype	archetype{ GRIM_INVALID_ARCHETYPE };		// usually hash of the model name
	};

	IGPUBufferPtr			m_rootBuffer;
	IGPUBufferPtr			m_singleInstIndexBuffer;
	IGPUBufferPtr			m_archetypesBuffer;			// per-instance archetype buffer
	IGPUComputePipelinePtr	m_updateRootPipeline;
	IGPUComputePipelinePtr	m_updateIntPipeline;

	Array<GRIMInstanceRef>	m_tempInstances{ PP_SL };

	Array<Instance>			m_instances{ PP_SL };
	Array<int>				m_freeIndices{ PP_SL };
	Set<int>				m_updated{ PP_SL };
	GRIMBaseComponentPool*	m_componentPools[GRIM_INSTANCE_MAX_COMPONENTS]{ nullptr };
	uint					m_buffersUpdated{ 0 };

	Map<GRIMArchetype, int>	m_archetypeInstCounts{ PP_SL };	// TODO: replace with refcount in GRIMBaseRenderer

	int						m_reservedInsts{ 0 };
};

// Instance component data storage
template<typename T>
class GRIMInstComponentPool : public GRIMBaseComponentPool
{
public:
	using TYPE = T;
	GRIMInstComponentPool()
		: m_dataPool(T::NAME, PP_SL)
	{
	}

	GRIMSyncrhronizedPool<T>& GetDataPool() { return m_dataPool; }

	GRIMBaseSyncrhronizedPool& GetData() override { return m_dataPool; }
	void AddElem() override { m_dataPool.Add(T{}); }
	void InitPipeline() override { T::InitPipeline(m_dataPool); }

protected:
	GRIMSyncrhronizedPool<T> m_dataPool;
};

//-----------------------------------------------------
// Below is a template part of Instance manager. Use it

// Component-based instance manager
template<typename ... Components>
class GRIMInstanceAllocator : public GRIMBaseInstanceAllocator
{
public:
	template<typename TComp>
	using Pool = GRIMInstComponentPool<TComp>;

	GRIMInstanceAllocator();

	// creates new empty instance with allocated components
	template<typename ...TComps>
	int 			AddInstance(int archetype)
	{
		const int instanceId = AllocInstance(archetype);
		AllocInstanceComponents<TComps...>(instanceId);
		return instanceId;
	}

	// creates new temporary empty instance with allocated components
	template<typename ...TComps>
	int 			AddTempInstance(int archetype)
	{
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
	Pool<TComp>&	GetComponentPool() { return std::get<Pool<TComp>>(m_componentPoolsStorage); }

protected:
	using POOL_STORAGE = std::tuple<Pool<Components>...>;

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

class GRIMInstanceDebug
{
public:
	static void DrawUI(GRIMBaseInstanceAllocator& instMngBase, ArrayCRef<EqStringRef> archetypeNames);
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
	const uint32 inPoolIdx = inst.components[First::COMPONENT_ID];

	// don't update invalid or default
	if (inPoolIdx == UINT_MAX || inPoolIdx == 0)
	{
		SetInternal(inst, values...);
		return; // Instance was not allocated with specified component, so skipping
	}

	Pool<First>& compPool = GetComponentPool<First>();
	if (!memcmp(&compPool.GetDataPool().GetData()[inPoolIdx], &firstVal, sizeof(First)))
	{
		SetInternal(inst, values...);
		return;
	}

	compPool.GetDataPool().Update(inPoolIdx, firstVal);
	SetInternal(inst, values...);
}

template<typename...Ts>
template<typename...TComps>
inline void GRIMInstanceAllocator<Ts...>::Set(int instanceId, const TComps&... values)
{
	if (instanceId == -1)
		return;
	Threading::CScopedMutex m(GetMutex());
	SetInternal(m_instances[instanceId].root, values...);
}

// creates new empty instance with allocated components
template<typename...Ts>
template<typename...TComps>
inline void GRIMInstanceAllocator<Ts...>::AllocInstanceComponents(int instanceId)
{
	InstRoot& inst = m_instances[instanceId].root;
	{
		Threading::CScopedMutex m(GetMutex());
		([&]{
			Pool<TComps>& compPool = GetComponentPool<TComps>();
			inst.components[TComps::COMPONENT_ID] = compPool.GetDataPool().Add(TComps{});
		} (), ...);
	}
}

template<typename...Ts>
template<typename TComp>
void GRIMInstanceAllocator<Ts...>::Add(int instanceId)
{
	if (instanceId == -1)
		return;

	InstRoot& inst = m_instances[instanceId].root;
	if (inst.components[TComp::COMPONENT_ID] > 0 && inst.components[TComp::COMPONENT_ID] != UINT_MAX)
		return;

	Pool<TComp>& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedMutex m(GetMutex());
		inst.components[TComp::COMPONENT_ID] = compPool.GetDataPool().Add(TComp{});
		m_updated.insert(instanceId);
	}
}

template<typename...Ts>
template<typename TComp>
void GRIMInstanceAllocator<Ts...>::Remove(int instanceId)
{
	if (instanceId == -1)
		return;

	InstRoot& inst = m_instances[instanceId].root;
	if (inst.components[TComp::COMPONENT_ID] == 0 || inst.components[TComp::COMPONENT_ID] == UINT_MAX)
		return;

	Pool<TComp>& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedMutex m(GetMutex());
		compPool.GetDataPool().Remove(inst.components[TComp::COMPONENT_ID]);
		inst.components[TComp::COMPONENT_ID] = 0; // change to default
		m_updated.insert(instanceId);
	}
}
