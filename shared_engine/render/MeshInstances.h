//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Mesh instance pool
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/renderers/ShaderAPI_defs.h"
#include "materialsystem1/RenderDefs.h"

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

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
	Array<BatchDesc>		batches;
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

	Array<InstanceRef>	instanceRefs;
};
