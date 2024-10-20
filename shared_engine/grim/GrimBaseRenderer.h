//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GRIM � GPU-driven Rendering and Instance Manager
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
	int				groupMaskInclude{ (int)COM_UINT_MAX };
	int				groupMaskExclude{ 0 };
	int				overrideLodIdx{ -1 };

	// TODO: renderer UID to validate
	BitArray		visibleArchetypes{PP_SL, 128};
};

class GRIMBaseRenderer : public IShaderMeshInstanceProvider
{
	template<typename T>
	using Pool = GRIMSyncrhronizedPool<T>;

	friend class GRIMInstanceDebug;

public:
	GRIMBaseRenderer(GRIMBaseInstanceAllocator& allocator);

	virtual void	Init();
	virtual void	Shutdown();

	GRIMArchetype	CreateStudioDrawArchetype(const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags = 0, int materialGroupIdx = 0, ArrayCRef<IGPUBufferPtr> extraVertexBuffers = nullptr, uint extraLayoutBits = 0);
	GRIMArchetype	CreateDrawArchetype(const GRIMArchetypeDesc& desc);
	void			DestroyDrawArchetype(GRIMArchetype id);

	void			SyncArchetypes(IGPUCommandRecorder* cmdRecorder);

	void			PrepareDraw(IGPUCommandRecorder* cmdRecorder, GRIMRenderState& renderState, int maxNumberOfObjects = -1);
	void			Draw(const GRIMRenderState& renderState, const RenderPassContext& renderCtx);

protected:

	struct IntermediateState;
	struct DrawInfo;
	struct ArchetypeInfo;

	struct GPUIndexedBatch;
	struct GPULodInfo;
	struct GPULodList;
	struct GPUInstanceBound;
	struct GPUInstanceInfo;

	IVector2D		VisCalcWorkSize(int length) const;

	bool			IsSync() const;
	void			DbgValidate() const;
	EqStringRef		DbgGetArchetypeName(GRIMArchetype archetypeId) const;
	void			DbgInvalidateAllData();

	void			FilterInstances_Compute(IntermediateState& intermediate);
	void			FilterInstances_Software(IntermediateState& intermediate);

	virtual void	VisibilityCullInstances_Compute(IntermediateState& intermediate) = 0;
	virtual void	VisibilityCullInstances_Software(IntermediateState& intermediate) = 0;

	void			SortInstances_Compute(IntermediateState& intermediate);
	void			SortInstances_Software(IntermediateState& intermediate);

	void			UpdateInstanceBounds_Compute(IntermediateState& intermediate);
	void			UpdateInstanceBounds_Software(IntermediateState& intermediate);

	void			UpdateIndirectInstances_Compute(IntermediateState& intermediate);
	void			UpdateIndirectInstances_Software(IntermediateState& intermediate);

	void			InitDrawArchetype(GRIMArchetype slot, const CEqStudioGeom* geom, IVertexFormat* vertFormat, uint bodyGroupFlags, int materialGroupIdx, ArrayCRef<IGPUBufferPtr> extraVertexBuffers, uint extraLayoutBits);
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
		uint					extraLayoutBits{ 0 };
		GRIMArchetype			slot{GRIM_INVALID_ARCHETYPE};
		Type					type{};
	};
	Array<PendingDesc>			m_pendingArchetypes{ PP_SL };
	Array<GRIMArchetype>		m_pendingDeletion{ PP_SL };

	GRIMBaseInstanceAllocator&	m_instAllocator;
	SlottedArray<DrawInfo>		m_drawInfos{ PP_SL };
	Pool<GPUIndexedBatch>		m_drawBatchs{ "GPUIndexedBatch", PP_SL };
	Pool<GPULodInfo>			m_drawLodInfos{ "GPULodInfo", PP_SL };
	Pool<GPULodList>			m_drawLodsList{ "GPULodList", PP_SL };

#ifdef GRIM_INSTANCES_DEBUG_ENABLED
	BitArray					m_dbgHiddenArchetypes{ PP_SL };
	BitArray					m_dbgLastVisibleArchetypes{ PP_SL };
	int							m_dbgStatsDrawCalls{ 0 };
	int							m_dbgStatsDrawInfos{ 0 };
#endif

	ComputeSortShaderPtr		m_sortShader;

	IGPUComputePipelinePtr		m_instCalcBoundsPipeline;
	IGPUComputePipelinePtr		m_instPrepareDrawIndirectPipeline;
	IGPUComputePipelinePtr		m_filterInstancesPipeline;
	IGPUComputePipelinePtr		m_filterCalcWorkGroupsPipeline;
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
	GPUBufferView			filteredInstanceInfosBuffer;
	GPUBufferView			filteredInstanceCountBuffer;
	IGPUBufferPtr			culledInstanceInfosBuffer;
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

struct GRIMBaseRenderer::ArchetypeInfo : RefCountedObject<ArchetypeInfo>
{
	using VertexBufferArray = FixedArray<IGPUBufferPtr, MAX_VERTEXSTREAM>;
	VertexBufferArray		vertexBuffers;
	IGPUBufferPtr			indexBuffer;
	MeshInstanceFormatRef	meshInstFormat;
	EIndexFormat			indexFormat{ 0 };
	EqString				name;
	int						instanceStreamId{ -1 };
	bool					skinningSupport{ false };
};

struct GRIMBaseRenderer::DrawInfo
{
	ArchetypeInfo::PTR_T	archetypeInfo;
	IMaterialPtr			material;
	EPrimTopology			primTopology{ PRIM_TRIANGLES };
	int						batchIdx{ -1 };
	int						lodNumber{ -1 };
	GRIMArchetype			ownerArchetype{ GRIM_INVALID_ARCHETYPE };
};

class GRIMInstanceDebug
{
public:
	static void 			DrawUI(GRIMBaseRenderer& renderer);
	static GRIMArchetype	GetHighlightArchetype();
	static EqString			GetInstanceDebugText(GRIMBaseRenderer& renderer, int instanceId);
};