//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances manager
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
		using DemoGPUInstanceManager = GPUInstanceManager<InstTransform>;

	3. Declare instancer and initialize
		static DemoGPUInstanceManager g_instanceMng;
		...
		s_instanceMng.Initialize();

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
		s_instanceMng.SyncInstances(cmdBuffer);

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

#ifndef _RETAIL
#define ENABLE_GPU_INSTANCE_DEBUG
#endif

constexpr int GPUINST_MAX_COMPONENTS = 8;

#define DEFINE_GPU_INSTANCE_COMPONENT(ID, Name) \
	static constexpr const char* NAME = #Name; \
	static constexpr int IDENTIFIER = StringToHashConst(#Name); \
	static constexpr int COMPONENT_ID = ID; \
	static void InitPipeline(GPUInstPool& pool);

#define INIT_GPU_INSTANCE_COMPONENT(Name, UpdateShaderName) \
	void Name::InitPipeline(GPUInstPool& pool) { \
	pool.updatePipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>() \
		.ShaderName(UpdateShaderName).ShaderLayoutId(IDENTIFIER).End()); \
	}

struct GPUInstPool
{
	GPUInstPool(int stride) : stride(stride) {}

	virtual const char* GetName() const = 0;
	virtual const void*	GetDataPtr() const = 0;
	virtual int			GetDataElems() const = 0;
	virtual void		ResetData() = 0;
	virtual void		ReserveData(int count) = 0;
	virtual void		InitPipeline() = 0;

	Array<int>			freeIndices{ PP_SL };
	Set<int>			updated{ PP_SL };
	IGPUBufferPtr		buffer;
	IGPUComputePipelinePtr	updatePipeline;
	int					stride{ 0 };
};

// The instance manager basic implementation
class GPUBaseInstanceManager
{
	friend class GPUInstanceManagerDebug;
public:
	GPUBaseInstanceManager();
	~GPUBaseInstanceManager() = default;

	void			Initialize(const char* instanceComputeShaderName, int instancesReserve = 1000);
	void			Shutdown();

	void			FreeAll(bool dealloc = false, bool reserve = false);

	GPUBufferView	GetSingleInstanceIndexBuffer() const;
	IGPUBufferPtr	GetInstanceArchetypesBuffer() const { return m_archetypesBuffer; }
	IGPUBufferPtr	GetRootBuffer() const { return m_rootBuffer; }
	IGPUBufferPtr	GetDataPoolBuffer(int componentId) const;
	int				GetInstanceCountByArchetype(int archetypeId) const;
	int				GetInstanceArchetypeId(int instanceIdx) const { return m_instances[instanceIdx].archetype; }

	int				GetInstanceSlotsCount() const { return m_instances.numElem(); }

	// syncs instance buffers with GPU and updates roots buffer
	void			SyncInstances(IGPUCommandRecorder* cmdRecorder);

	// sets batches that are drrawn with particular instance
	void			SetBatches(int instanceId, uint batchesFlags);

	// destroys instance and it's components
	void			FreeInstance(int instanceId);

	void			DbgRegisterArhetypeName(int archetypeId, const char* name);

protected:
	int				AllocInstance(int archetype);
	int				AllocTempInstance(int archetype);

	struct InstRoot
	{
		uint32	components[GPUINST_MAX_COMPONENTS]{ 0 };
	};

	struct Instance
	{
		InstRoot	root;
		int			archetype{ -1 };		// usually hash of the model name
		uint		batchesFlags{ 0 };		// bit flags of drawn batches
	};

	Threading::CEqMutex		m_mutex;
	IGPUBufferPtr			m_rootBuffer;
	IGPUBufferPtr			m_singleInstIndexBuffer;
	IGPUBufferPtr			m_archetypesBuffer;			// per-instance archetype buffer
	IGPUComputePipelinePtr	m_updateRootPipeline;
	IGPUComputePipelinePtr	m_updateIntPipeline;

	Array<int>				m_tempInstances{ PP_SL };

	Array<Instance>			m_instances{ PP_SL };
	Array<int>				m_freeIndices{ PP_SL };
	Set<int>				m_updated{ PP_SL };
	GPUInstPool*			m_componentPools[GPUINST_MAX_COMPONENTS]{ nullptr };
	uint					m_buffersUpdated{ 0 };

	Map<int, int>			m_archetypeInstCounts{ PP_SL };
#ifdef ENABLE_GPU_INSTANCE_DEBUG
	Map<int, EqString>		m_archetypeNames{ PP_SL };
#endif

	int						m_reservedInsts{ 0 };
};

//-----------------------------------------------------
// Below is a template part of Instance manager. Use it

// Instance component data storage
template<typename T>
struct GPUInstDataPool : public GPUInstPool
{
	using TYPE = T;

	GPUInstDataPool() : GPUInstPool(sizeof(T)) {}

	const char*	GetName() const override { return T::NAME; }
	const void*	GetDataPtr() const override { return data.ptr(); };
	int			GetDataElems() const override { return data.numElem(); }
	void		ResetData() override { data.setNum(1); }		// reset data to have default element only
	void		InitPipeline() { T::InitPipeline(*this); }
	void		ReserveData(int count) override { data.reserve(count); }

	Array<T> 	data{ PP_SL };
};

// Component-based instance manager
template<typename ... Components>
class GPUInstanceManager : public GPUBaseInstanceManager
{
public:
	template<typename TComp>
	using Pool = GPUInstDataPool<TComp>;

	GPUInstanceManager();

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

protected:
	using POOL_STORAGE = std::tuple<Pool<Components>...>;

	template<typename ...TComps>
	void 			AllocInstanceComponents(int instanceId);

	// sets component values on instance
	template<typename First, typename...Rest>
	void			SetInternal(InstRoot& inst, const First& firstVal, const Rest&... values);

	// sets component values on instance
	void			SetInternal(InstRoot& inst) {}

	template<typename TComp>
	Pool<TComp>&	GetComponentPool() { return std::get<Pool<TComp>>(m_componentPoolsStorage); }

	template<std::size_t... Is>
	void			InitPool(std::index_sequence<Is...>);

	POOL_STORAGE	m_componentPoolsStorage;
};

class GPUInstanceManagerDebug
{
public:
	static void DrawUI(GPUBaseInstanceManager& instMngBase);
};

//-------------------------------

template<typename...Ts>
template<std::size_t... Is>
void GPUInstanceManager<Ts...>::InitPool(std::index_sequence<Is...>)
{
	([&] {
		using POOL_TYPE = std::tuple_element_t<Is, POOL_STORAGE>;
		POOL_TYPE& pool = std::get<Is>(m_componentPoolsStorage);
		pool.data.append(typename POOL_TYPE::TYPE{}); // add default value
		pool.updated.insert(0);
		m_componentPools[POOL_TYPE::TYPE::COMPONENT_ID] = &pool;
	}(), ...);
}

template<typename...Ts>
inline GPUInstanceManager<Ts...>::GPUInstanceManager()
{
	InitPool(std::index_sequence_for<Ts...>{});
}

template<typename...Ts>
template<typename First, typename...Rest>
inline void GPUInstanceManager<Ts...>::SetInternal(InstRoot& inst, const First& firstVal, const Rest&... values)
{
	const uint32 inPoolIdx = inst.components[First::COMPONENT_ID];

	// don't update invalid or default
	if (inPoolIdx == UINT_MAX || inPoolIdx == 0)
	{
		SetInternal(inst, values...);
		return; // Instance was not allocated with specified component, so skipping
	}

	Pool<First>& compPool = GetComponentPool<First>();
	if (!memcmp(&compPool.data[inPoolIdx], &firstVal, sizeof(First)))
	{
		SetInternal(inst, values...);
		return;
	}

	{
		Threading::CScopedMutex m(m_mutex);
		compPool.data[inPoolIdx] = firstVal;
		compPool.updated.insert(inPoolIdx);
	}
	SetInternal(inst, values...);
}

template<typename...Ts>
template<typename...TComps>
inline void GPUInstanceManager<Ts...>::Set(int instanceId, const TComps&... values)
{
	if (instanceId == -1)
		return;

	InstRoot& inst = m_instances[instanceId].root;
	SetInternal(inst, values...);
}

// creates new empty instance with allocated components
template<typename...Ts>
template<typename...TComps>
inline void GPUInstanceManager<Ts...>::AllocInstanceComponents(int instanceId)
{
	InstRoot& inst = m_instances[instanceId].root;
	{
		Threading::CScopedMutex m(m_mutex);
		([&]{
			Pool<TComps>& compPool = GetComponentPool<TComps>();
			const int inPoolIdx = compPool.freeIndices.numElem() ? compPool.freeIndices.popBack() : compPool.data.append(TComps{});
			inst.components[TComps::COMPONENT_ID] = inPoolIdx;
			compPool.updated.insert(inPoolIdx);
		} (), ...);
	}
}

template<typename...Ts>
template<typename TComp>
void GPUInstanceManager<Ts...>::Add(int instanceId)
{
	if (instanceId == -1)
		return;

	InstRoot& inst = m_instances[instanceId].root;
	if (inst.components[TComp::COMPONENT_ID] > 0 && inst.components[TComp::COMPONENT_ID] != UINT_MAX)
		return;

	Pool<TComp>& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedMutex m(m_mutex);
		const int inPoolIdx = compPool.freeIndices.numElem() ? compPool.freeIndices.popBack() : compPool.data.append(TComp{});
		inst.components[TComp::COMPONENT_ID] = inPoolIdx;
		compPool.updated.insert(inPoolIdx);

		m_updated.insert(instanceId);
	}
}

template<typename...Ts>
template<typename TComp>
void GPUInstanceManager<Ts...>::Remove(int instanceId)
{
	if (instanceId == -1)
		return;

	InstRoot& inst = m_instances[instanceId].root;
	if (inst.components[TComp::COMPONENT_ID] == 0 || inst.components[TComp::COMPONENT_ID] == UINT_MAX)
		return;

	Pool<TComp>& compPool = GetComponentPool<TComp>();
	{
		Threading::CScopedMutex m(m_mutex);
		compPool.freeIndices.append(inst.components[TComp::COMPONENT_ID]);
		inst.components[TComp::COMPONENT_ID] = 0; // change to default
		m_updated.insert(instanceId);
	}
}
