//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK mesh: triangles with material batches sent to Blender
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "BlinkMesh.h"

void BlinkMesh::BeginBatch(const char* material)
{
	Batch& batch = batches.append();
	batch.material = material;
	batch.firstIndex = indices.numElem();
}

void BlinkMesh::Write(KVSection& out, IFileStream& payload) const
{
	for (int i = 0; i < batches.numElem(); ++i)
	{
		const Batch& batch = batches[i];
		const int endIndex = i + 1 < batches.numElem() ? batches[i + 1].firstIndex : indices.numElem();

		KVSection& sec = out.CreateSection("batch");
		sec.SetKey("material", batch.material.ToCString());
		sec.SetKey("firstIndex", batch.firstIndex);
		sec.SetKey("indexCount", endIndex - batch.firstIndex);
	}

	payload.WriteObj(positions);
	payload.WriteObj(normals);
	payload.WriteObj(texcoords);
	payload.WriteObj(indices);
}
