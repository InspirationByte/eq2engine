//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK meshes of EGF models
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "egf/model.h"
#include "BlinkMesh.h"
#include "BlinkStudio.h"

void BlinkMakeStudioMesh(const studioHdr_t& studio, ArrayCRef<IMaterialPtr> materials, int bodyGroupFlags, int lod, BlinkMesh& mesh)
{
	lod = clamp(lod, 0, MAX_MODEL_LODS - 1);
	for (int i = 0; i < studio.numBodyGroups; ++i)
	{
		if (!(bodyGroupFlags & (1 << i)))
			continue;

		const studioLodModel_t* lodModel = studio.pLodModel(studio.pBodyGroups(i)->lodModelIndex);
		uint8 meshGroupIdx = EGF_INVALID_IDX;
		for (int bodyGroupLod = lod; bodyGroupLod >= 0 && meshGroupIdx == EGF_INVALID_IDX; --bodyGroupLod)
			meshGroupIdx = lodModel->meshIndexes[bodyGroupLod];

		if (meshGroupIdx == EGF_INVALID_IDX)
			continue;

		const studioMeshGroupDesc_t* meshGroup = studio.pMeshGroupDesc(meshGroupIdx);
		for (int j = 0; j < meshGroup->numMeshes; ++j)
		{
			const studioMeshDesc_t* studioMesh = meshGroup->pMesh(j);
			const int materialIdx = studioMesh->materialIndex;
			if (materials.inRange(materialIdx) && materials[materialIdx])
				mesh.BeginBatch(materials[materialIdx]->GetName());
			else
				mesh.BeginBatch(materialIdx >= 0 && materialIdx < studio.numMaterials ? studio.pMaterial(materialIdx)->materialname : "error");

			const int firstVertex = mesh.positions.numElem();
			const bool hasNormals = studioMesh->vertexType & STUDIO_VERTFLAG_TBN;
			for (int k = 0; k < studioMesh->numVertices; ++k)
			{
				const studioVertexPosUv_t* vertex = studioMesh->pPosUvs(k);
				mesh.positions.append(vertex->point);
				mesh.texcoords.append(vertex->texCoord);
				mesh.normals.append(hasNormals ? studioMesh->pTBNs(k)->normal : vec3_zero);
			}

			// every second strip triangle is flipped
			const uint32* indices = studioMesh->pVertexIdx(0);
			const bool strip = studioMesh->primitiveType == STUDIO_PRIM_TRI_STRIP;
			const int numIndices = static_cast<int>(studioMesh->numIndices);
			for (int k = 0; k + 2 < numIndices; k += strip ? 1 : 3)
			{
				const uint32 a = indices[k], b = indices[k + 1], c = indices[k + 2];
				if (a == b || a == c || b == c)
					continue;

				const bool flip = strip && (k % 2);
				mesh.indices.append(firstVertex + static_cast<int>(flip ? c : a));
				mesh.indices.append(firstVertex + static_cast<int>(b));
				mesh.indices.append(firstVertex + static_cast<int>(flip ? a : c));
			}
		}
	}
}
