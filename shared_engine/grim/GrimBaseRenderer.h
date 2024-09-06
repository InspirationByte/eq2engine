//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: GRIM – GPU-driven Rendering and Instance Manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/SlottedArray.h"
#include "GrimDefs.h"
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
	Vector3D		viewPos;
	Volume			frustum;

	IGPUBufferPtr	drawInvocationsBuffer;
	IGPUBufferPtr	instanceIdsBuffer;

	// TODO: renderer UID to validate
};

class GRIMBaseRenderer : public IShaderMeshInstanceProvider
{
public:
	GRIMBaseRenderer(GRIMBaseInstanceAllocator& allocator);

	void			Init();
	void			Shutdown();

	GRIMArchetype	CreateDrawArchetypeEGF(const CEqStudioGeom& geom, IVertexFormat* vertFormat, uint bodyGroupFlags = 0, int materialGroupIdx = 0);
	GRIMArchetype	CreateDrawArchetype(const GRIMArchetypeCreateDesc& desc);
	void			DestroyDrawArchetype(GRIMArchetype id);

	void			PrepareDraw(IGPUCommandRecorder* cmdRecorder, GRIMRenderState& renderState, int maxNumberOfObjects);
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

	GRIMBaseInstanceAllocator&		m_instAllocator;
	SlottedArray<GPUDrawInfo>		m_drawInfos{ PP_SL };	// must be in sync with batchs
	SlottedArray<GPUIndexedBatch>	m_drawBatchs{ PP_SL };
	SlottedArray<GPULodInfo>		m_drawLodInfos{ PP_SL };
	SlottedArray<GPULodList>		m_drawLodsList{ PP_SL };

	IGPUBufferPtr					m_drawBatchsBuffer;
	IGPUBufferPtr					m_drawLodInfosBuffer;
	IGPUBufferPtr					m_drawLodsListBuffer;

	ComputeSortShaderPtr			m_sortShader;

	IGPUComputePipelinePtr			m_instCalcBoundsPipeline;
	IGPUComputePipelinePtr			m_instPrepareDrawIndirect;
	// TODO: culling pipeline must be external
	IGPUComputePipelinePtr			m_cullInstancesPipeline;

	IGPUBindGroupPtr				m_cullBindGroup0;
	IGPUBindGroupPtr				m_updateBindGroup0;
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

	int		firstBatch{ 0 };	//  item index in buffer pointing to GPUIndexedBatch
	float	distance{ 0.0f };
};

struct GRIMBaseRenderer::GPULodList
{
	int		firstLodInfo; // item index in buffer pointing to GPULodInfo
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
	GRIMRenderState			renderState;
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
	IGPUBufferPtr			vertexBuffers[MAX_VERTEXSTREAM - 1];
	IGPUBufferPtr			indexBuffer;
	IMaterialPtr			material;
	MeshInstanceFormatRef	meshInstFormat;
	EPrimTopology			primTopology{ PRIM_TRIANGLES };
	EIndexFormat			indexFormat{ 0 };
	int						batchIdx{ -1 };
};