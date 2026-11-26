//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK mesh: triangles with material batches sent to and from Blender
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
	payload.WriteObj(colors);
}

// an int count and the items; the count is checked against the bytes left
template<typename T>
static bool BlinkMeshReadArray(IFileStream& payload, Array<T>& out)
{
	int count = 0;
	if (payload.GetSize() - payload.Tell() < static_cast<VSSize>(sizeof(count)))
		return false;
	payload.Read(&count, 1, sizeof(count));

	if (count < 0 || static_cast<int64>(count) * static_cast<int64>(sizeof(T)) > payload.GetSize() - payload.Tell())
		return false;

	out.setNum(count);
	if (count)
		payload.Read(out.ptr(), count, sizeof(T));
	return true;
}

bool BlinkMesh::Read(const KVSection& params, IFileStream& payload, EqString& error)
{
	batches.clear();
	colors.clear();

	if (!BlinkMeshReadArray(payload, positions) || !BlinkMeshReadArray(payload, normals)
		|| !BlinkMeshReadArray(payload, texcoords) || !BlinkMeshReadArray(payload, indices))
	{
		error = "the payload is shorter than its arrays";
		return false;
	}

	// colors are optional
	if (payload.Tell() < payload.GetSize() && !BlinkMeshReadArray(payload, colors))
	{
		error = "the payload is shorter than its arrays";
		return false;
	}

	const int numVertices = positions.numElem();
	if (normals.numElem() != numVertices || texcoords.numElem() != numVertices
		|| (colors.numElem() && colors.numElem() != numVertices))
	{
		error = "the vertex arrays differ in size";
		return false;
	}

	if (indices.numElem() % 3)
	{
		error = "the index count is not a multiple of 3";
		return false;
	}

	for (const int index : indices)
	{
		if (index < 0 || index >= numVertices)
		{
			error = EqString::Format("index %d is out of %d vertices", index, numVertices);
			return false;
		}
	}

	// the batches follow each other and cover all the indices
	int endIndex = 0;
	for (const KVSection& sec : params.Keys("batch"))
	{
		const char* material = "";
		int firstIndex = -1;
		int indexCount = -1;
		sec.Get("material").GetValues(material);
		sec.Get("firstIndex").GetValues(firstIndex);
		sec.Get("indexCount").GetValues(indexCount);

		if (firstIndex != endIndex || indexCount < 0 || indexCount % 3 || firstIndex + indexCount > indices.numElem())
		{
			error = EqString::Format("batch '%s' does not follow the previous one", material);
			return false;
		}

		Batch& batch = batches.append();
		batch.material = material;
		batch.firstIndex = firstIndex;
		endIndex = firstIndex + indexCount;
	}

	if (endIndex != indices.numElem())
	{
		error = "the batches do not cover the indices";
		return false;
	}

	return true;
}
