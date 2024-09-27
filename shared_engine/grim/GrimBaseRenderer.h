//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GRIM – GPU-driven Rendering and Instance Manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "GrimDefs.h"
#include "GrimSynchronizedPool.h"
#include "render/ComputeSort.h"
#include "materialsystem1/IMatSysShader.h"
#include "materialsystem1/IMaterial.h"
#include "materialsystem1/RenderDefs.h"

class GRIMBaseInstanceAllocator;
class CEqStudioGeom;
class ComputeSortShader;
using ComputeSortShaderPtr = CRefPtr<ComputeSortShader>;
struct RenderPassContext;

struct GRIMRenderState
{
	IGPUBufferPtr	drawInvocationsBuffer;
	IGPUBufferPtr	instanceIdsBuffer;

	// TODO: renderer UID to validate
};

class GRIMBaseRenderer : public IShaderMeshInstanceProvider
{
	template<typename T>
	using Pool = GRIMSyncrhronizedPool<T>;

public:
	GRIMBaseRenderer(GRIMBaseInstanceAllocator& allocator);

	virtual void	Init();
	virtual void	Shutdown();

	GRIMArchetype	CreateStudioDrawArchetype(const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags = 0, int materialGroupIdx = 0, ArrayCRef<IGPUBufferPtr> extraVertexBuffers = nullptr);
	GRIMArchetype	CreateDrawArchetype(const GRIMArchetypeDesc& desc);
	void			DestroyDrawArchetype(GRIMArchetype id);

	void			SyncArchetypes(IGPUCommandRecorder* cmdRecorder);

	void			PrepareDraw(IGPUCommandRecorder* cmdRecorder, GRIMRenderState& renderState, int maxNumberOfObjects = -1);
	void			Draw(const GRIMRenderState& renderState, const RenderPassContext& renderCtx);

protected:

	struct IntermediateState;
	struct GPUIndexedBatch;
	struct GPULodInfo;
	struct GPULodList;
	struct GPUInstanceBound;
	struct GPUDrawInfo;
	struct GPUInstanceInfo;

	virtual void	VisibilityCullInstances_Compute(IntermediateState& intermediate) = 0;
	virtual void	VisibilityCullInstances_Software(IntermediateState& intermediate) = 0;

	void			SortInstances_Compute(IntermediateState& intermediate);
	void			SortInstances_Software(IntermediateState& intermediate);

	void			UpdateInstanceBounds_Compute(IntermediateState& intermediate);
	void			UpdateInstanceBounds_Software(IntermediateState& intermediate);

	void			UpdateIndirectInstances_Compute(IntermediateState& intermediate);
	void			UpdateIndirectInstances_Software(IntermediateState& intermediate);

	void			InitDrawArchetype(GRIMArchetype slot, const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags, int materialGroupIdx, ArrayCRef<IGPUBufferPtr> extraVertexBuffers);
	void			InitDrawArchetype(GRIMArchetype slot, const GRIMArchetypeDesc& desc);
	void			DestroyPendingArchetypes();

	struct PendingDesc
	{
		enum Type {
			TYPE_STUDIO,
			TYPE_GRIM
		};

		GRIMArchetypeDesc desc;
		struct GRIMStudioDesc {
			const CEqStudioGeom* geom;
			IVertexFormat* vertFormat;
			uint bodyGroupFlags;
			int materialGroupIdx;
		} egfDesc;

		Array<IGPUBufferPtr>	extraVertexBuffers{ PP_SL };
		GRIMArchetype			slot{GRIM_INVALID_ARCHETYPE};
		Type					type{};
	};
	Array<PendingDesc>			m_pendingArchetypes{ PP_SL };
	Array<GRIMArchetype>		m_pendingDeletion{ PP_SL };

	GRIMBaseInstanceAllocator&	m_instAllocator;
	SlottedArray<GPUDrawInfo>	m_drawInfos{ PP_SL };
	Pool<GPUIndexedBatch>		m_drawBatchs{ "GPUIndexedBatch", PP_SL };
	Pool<GPULodInfo>			m_drawLodInfos{ "GPULodInfo", PP_SL };
	Pool<GPULodList>			m_drawLodsList{ "GPULodList", PP_SL };

	ComputeSortShaderPtr		m_sortShader;

	IGPUComputePipelinePtr		m_instCalcBoundsPipeline;
	IGPUComputePipelinePtr		m_instPrepareDrawIndirect;
	// TODO: culling pipeline must be external
	IGPUComputePipelinePtr		m_cullInstancesPipeline;

	IGPUBindGroupPtr			m_cullBindGroup0;
	IGPUBindGroupPtr			m_updateBindGroup0;
};

struct GRIMBaseRenderer::GPUIndexedBatch
{
	int		next{ -1 };		// next index in buffer pointing to GPUIndexedBatch

	int		indexCount{ 0 };
	int		firstIndex{ 0 };
	int		cmdIdx{ -1 };
};

struct GRIMBaseRenderer::GPULodInfo
{
	int		next{ -1 };			// next index in buffer pointing to GPULodInfo

	int		firstBatch{ -1 };	//  item index in buffer pointing to GPUIndexedBatch
	float	distance{ 0.0f };
};

struct GRIMBaseRenderer::GPULodList
{
	int		firstLodInfo = -1; // item index in buffer pointing to GPULodInfo
};

// this is only here for software reference impl
struct GRIMBaseRenderer::GPUInstanceBound
{
	int		first{ 0 };
	int		last{ 0 };
	int		archIdx{ -1 };
	int		lodIndex{ -1 };
};

struct GRIMBaseRenderer::IntermediateState
{
	GRIMRenderState&		renderState;
	int						maxNumberOfObjects{ 0 };

	IGPUCommandRecorderPtr	cmdRecorder;

	IGPUBufferPtr			sortedInstanceIds;
	IGPUBufferPtr			instanceInfosBuffer;
	IGPUBufferPtr			drawInstanceBoundsBuffer;

	// Software only
	Array<GPUInstanceInfo>	instanceInfos{ PP_SL };
	Array<GPUInstanceBound>	drawInstanceBounds{ PP_SL };
};

struct GRIMBaseRenderer::GPUInstanceInfo
{
	static constexpr int ARCHETYPE_BITS = 24;
	static constexpr int LOD_BITS = 8;

	static constexpr int ARCHETYPE_MASK = (1 << ARCHETYPE_BITS) - 1;
	static constexpr int LOD_MASK = (1 << LOD_BITS) - 1;

	int		instanceId;
	int		packedArchetypeId;
};

struct GRIMBaseRenderer::GPUDrawInfo
{
	using VertexBufferArray = FixedArray<IGPUBufferPtr, MAX_VERTEXSTREAM>;
	VertexBufferArray		vertexBuffers;
	IGPUBufferPtr			indexBuffer;
	IMaterialPtr			material;
	MeshInstanceFormatRef	meshInstFormat;
	EPrimTopology			primTopology{ PRIM_TRIANGLES };
	EIndexFormat			indexFormat{ 0 };
	int						batchIdx{ -1 };
	int						lodNumber{ -1 };
	int						instanceStreamId{ -1 };
	GRIMArchetype			ownerArchetype{ GRIM_INVALID_ARCHETYPE };
	bool					skinningSupport{ false };
};