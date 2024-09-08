#pragma once
#include "materialsystem1/renderers/ShaderAPICaps.h"
#include "materialsystem1/RenderDefs.h"

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

enum EPrimTopology : int;
enum EIndexFormat : int;

using GRIMArchetype = int;
using GRIMInstanceRef = int;

static constexpr GRIMArchetype	GRIM_INVALID_ARCHETYPE = -1;
static constexpr int			GRIM_MAX_INSTANCE_LODS = 8;
static constexpr int			GRIM_INSTANCE_MAX_VERTEX_STREAMS = MAX_VERTEXSTREAM - 1;
static constexpr int			GRIM_INSTANCE_MAX_COMPONENTS = 8;

struct GRIMArchetypeDesc
{
	struct Batch
	{
		IMaterialPtr	material;
		EPrimTopology	primTopology{};
		int			firstIndex{ 0 };
		int			indexCount{ 0 };
		
	};

	struct LodInfo
	{
		int			firstBatch{ 0 };
		int			batchCount{ 0 };
		float		distance{ 0.0f };
	};

	IGPUBufferPtr			vertexBuffers[GRIM_INSTANCE_MAX_VERTEX_STREAMS];
	IGPUBufferPtr			indexBuffer;
	MeshInstanceFormatRef	meshInstanceFormat;
	Array<Batch>			batches{ PP_SL };
	Array<LodInfo>			lods{ PP_SL };

	EIndexFormat			indexFormat;
};