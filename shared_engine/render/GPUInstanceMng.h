//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GPU Instances manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

/*
class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

// TODO: drawable concept for indirect drawing

struct DrawableDesc
{
	struct BatchDesc
	{
		IMaterialPtr	material;
		EPrimTopology	primitiveType{ (EPrimTopology)0 };
		int				firstIndex{ 0 };
		int				numIndices{ 0 };
		int				firstVertex{ 0 };
		int				numVertices{ 0 };
	};

	using VertexBufferArray = FixedArray<GPUBufferView, MAX_VERTEXSTREAM>;

	MeshInstanceFormatRef	meshInstFormat;
	VertexBufferArray		vertexBuffers;
	GPUBufferView			indexBuffer;
	Array<BatchDesc>		batches{ PP_SL };
};

struct DrawableInstanceList
{
	using InstanceRef = int;
	using BufferArray = FixedArray<RenderBufferInfo, 8>;

	DrawableDesc*		drawable;
	int					batchIndex{ 0 };

	GPUBufferView		indirectBuffer;
	GPUBufferView		instanceRefsBuffer;
	BufferArray			storageUniformBuffers;

	Array<InstanceRef>	instanceRefs{ PP_SL };
};
*/

constexpr int GPUINST_MAX_COMPONENTS = 8;

// instance component data
template<int COMP_ID, int IDENT>
struct GPUInstComponent
{
	static constexpr int COMPONENT_ID = COMP_ID;
	static constexpr int IDENTIFIER = IDENT;
};

struct GPUInstPool
{
	GPUInstPool(int stride) : stride(stride) {}

	virtual const void*	GetDataPtr() const = 0;
	virtual void		InitPipeline() = 0;

	Array<int>		freeIndices{ PP_SL };
	Set<int>		updated{ PP_SL };
	IGPUBufferPtr	buffer;
	IGPUComputePipelinePtr	updatePipeline;
	int				stride{ 0 };
};

// Instance component data storage
template<typename T>
struct GPUInstDataPool : public GPUInstPool
{
	using TYPE = T;

	GPUInstDataPool() : GPUInstPool(sizeof(T)) {}

	const void*		GetDataPtr() const override { return data.ptr(); };
	void			InitPipeline() { T::InitPipeline(*this); }

	Array<T> 		data{ PP_SL };
};

class GPUBaseInstanceManager
{
public:
	void				Initialize();
	void				Shutdown();

	IGPUBufferPtr		GetRootBuffer() const { return m_buffer; }
	IGPUBufferPtr		GetDataPoolBuffer(int componentId) const;

	// syncs instance buffers with GPU and updates roots buffer
	void				SyncInstances(IGPUCommandRecorder* cmdRecorder);

	// destroys instance and it's components
	void				FreeInstance(int instanceId);

protected:
	int					AllocInstance();

	struct InstRoot
	{
		uint32	components[GPUINST_MAX_COMPONENTS]{ UINT_MAX };
	};

	IGPUBufferPtr				m_buffer;
	IGPUComputePipelinePtr		m_updatePipeline;
	Array<InstRoot>				m_instances{ PP_SL };
	Array<int>					m_freeIndices{ PP_SL };
	Set<int>					m_updated{ PP_SL };
	Map<uint8, GPUInstPool*>	m_componentPools{ PP_SL };
};


// Instance buffers layout:
template<typename ... Components>
class GPUInstanceManager : public GPUBaseInstanceManager
{
public:
	template<typename TComp>
	using Pool = GPUInstDataPool<TComp>;

	GPUInstanceManager();

	// creates new empty instance with allocated components
	template<typename ...TComps>
	int 			AddInstance();

	template<typename TComp>
	void			Set(int instanceId, const TComp& value);
protected:

	using POOL_STORAGE = std::tuple<Pool<Components>...>;

	template<typename TComp>
	Pool<TComp>		GetComponentPool() { return std::get<Pool<TComp>>(m_componentPoolsStorage); }

	template<std::size_t... Is>
	void			InitPool(std::index_sequence<Is...>);

	POOL_STORAGE	 m_componentPoolsStorage;
};

//-------------------------------

template<typename...Ts>
template<std::size_t... Is>
void GPUInstanceManager<Ts...>::InitPool(std::index_sequence<Is...>)
{
	(
		m_componentPools.insert(std::tuple_element_t<Is, POOL_STORAGE>::TYPE::COMPONENT_ID, &std::get<Is>(m_componentPoolsStorage))
	, ...);
}

template<typename...Ts>
inline GPUInstanceManager<Ts...>::GPUInstanceManager()
{
	InitPool(std::index_sequence_for<Ts...>{});
}

template<typename...Ts>
template<typename TComp>
inline void GPUInstanceManager<Ts...>::Set(int instanceId, const TComp& value)
{
	InstRoot& inst = m_instances[instanceId];

	// FIXME: might be slowwww
	const uint32 inPoolIdx = inst.components[TComp::COMPONENT_ID];
	if (inPoolIdx == UINT_MAX)
	{
		ASSERT_FAIL("Instance was not allocated with specified component");
		return;
	}

	Pool<TComp>& compPool = GetComponentPool<TComp>();
	compPool.data[inPoolIdx] = value;
	compPool.updated.insert(inPoolIdx);
}

// creates new empty instance with allocated components
template<typename...Ts>
template<typename...TComps>
inline int GPUInstanceManager<Ts...>::AddInstance()
{
	const int instanceId = AllocInstance();
	InstRoot& inst = m_instances[instanceId];

	memset(inst.components, UINT_MAX, sizeof(inst.components));

	([&]
	{
		Pool<TComps>& compPool = GetComponentPool<TComps>();
		inst.components[TComps::COMPONENT_ID] = compPool.freeIndices.numElem() ? compPool.freeIndices.popBack() : compPool.data.append({});
	} (), ...);
}