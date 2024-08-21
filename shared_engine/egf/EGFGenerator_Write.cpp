//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Geometry Format Writer
//////////////////////////////////////////////////////////////////////////////////

#include <meshoptimizer.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "math/Utility.h"
#include "EGFGenerator.h"
#include "utils/AdjacentTriangles.h"

#include "dsm_loader.h"
#include "dsm_esm_loader.h"

using namespace SharedModel;

// EGF file buffer
#define FILEBUFFER_EQGF (16 * 1024 * 1024)

#define WRITE_RESERVE(type)				stream->Seek(sizeof(type), VS_SEEK_CUR)
#define WRITE_RESERVE_NUM(type, num)	stream->Seek(sizeof(type)*num, VS_SEEK_CUR)

#define WRITE_OFS						stream->Tell()		// write offset over header
#define WRITE_RELATIVE_OFS(obj)			(stream->Tell() - ((ubyte*)obj - (ubyte*)header))	// write offset over object

//---------------------------------------------------------------------------------------

struct StudioVertexData
{
	studioVertexPosUv_t	posUvs;
	studioVertexTBN_t	tbn;
	studioVertexColor_t	color;
	studioBoneWeight_t	boneWeights;
};

static bool CompareVertex(const StudioVertexData& v0, const StudioVertexData&v1)
{
	static constexpr const float VERT_MERGE_EPS = 0.001f;
	static constexpr const float WEIGHT_MERGE_EPS = 0.001f;

	if (vecSimilar(v0.posUvs.point, v1.posUvs.point, VERT_MERGE_EPS) &&
		vecSimilar(v0.posUvs.texCoord, v1.posUvs.texCoord, VERT_MERGE_EPS) &&
		vecSimilar(v0.tbn.normal, v1.tbn.normal, VERT_MERGE_EPS)) // FIXME: dot product?
	{
		// check each bone weight
		int numEqual = 0;
		for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; ++i)
		{
			bool match = false;
			for (int j = 0; j < MAX_MODEL_VERTEX_WEIGHTS; ++j)
			{
				if (v0.boneWeights.bones[i] == v1.boneWeights.bones[j] && fsimilar(v0.boneWeights.weight[i], v1.boneWeights.weight[j], WEIGHT_MERGE_EPS))
				{
					match = true;
					break;
				}
			}
			if(match)
				++numEqual;
		}
		if (numEqual == MAX_MODEL_VERTEX_WEIGHTS)
			return true;
	}

	return false;
}

static StudioVertexData MakeStudioVertex(const DSVertex& vert)
{
	StudioVertexData vertex;

	vertex.posUvs.point = vert.position;
	vertex.posUvs.texCoord = vert.texcoord;

	vertex.tbn.normal = vert.normal;
	vertex.tbn.binormal = vec3_zero;	// to be computed later
	vertex.tbn.tangent = vec3_zero;		// to be computed later

	// reset
	for(int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; ++i)
	{
		vertex.boneWeights.bones[i] = -1;
		vertex.boneWeights.weight[i] = 0.0f;
	}

	// assign bones and it's weights
	int weightCnt = 0;
	for(int i = 0; i < vert.weights.numElem(); ++i)
	{
		if (vert.weights[i].bone == -1)
			continue;

		vertex.boneWeights.bones[weightCnt] = vert.weights[i].bone;
		vertex.boneWeights.weight[weightCnt] = vert.weights[i].weight;
		weightCnt++;
	}
	vertex.boneWeights.numweights = weightCnt;
	vertex.color.color = vert.color.pack();

	ASSERT_MSG(weightCnt <= MAX_MODEL_VERTEX_WEIGHTS, "Too many weights on vertex (%d, max is %d)", vertex.boneWeights.numweights, MAX_MODEL_VERTEX_WEIGHTS);

	return vertex;
}

static void ApplyShapeKeyOnVertex( DSShapeKey* modShapeKey, const DSVertex& vert, StudioVertexData& destVert, float weight )
{
	for(DSShapeVert& shapeVert : modShapeKey->verts)
	{
		if(shapeVert.vertexId == vert.vertexId )
		{
			destVert.posUvs.point = lerp(destVert.posUvs.point, shapeVert.position, weight);
			destVert.tbn.normal = lerp(destVert.tbn.normal, shapeVert.normal, weight);
			return;
		}
	}
}

int CEGFGenerator::UsedMaterialIndex(const char* pszName)
{
	const int matIdx = GetMaterialIndex(pszName);
	ASSERT(matIdx != -1);

	GenMaterialDesc* material = &m_materials[matIdx];
	material->used++;

	return m_usedMaterials.addUnique(material);
}


// writes group
void CEGFGenerator::WriteGroup(studioHdr_t* header, IVirtualStream* stream, DSMesh* srcGroup, DSShapeKey* modShapeKey, float simplifyThreshold, studioMeshDesc_t* dstGroup)
{
	int vertexStreamsAvailableBits = STUDIO_VERTFLAG_POS_UV | STUDIO_VERTFLAG_TBN;

	// DSM groups to be generated indices and optimized here
	dstGroup->materialIndex = m_notextures ? -1 : UsedMaterialIndex(srcGroup->texture);

	// triangle list by default
	dstGroup->primitiveType = STUDIO_PRIM_TRIANGLES;
	dstGroup->numVertices = 0;
	dstGroup->numIndices = 0;

	Array<StudioVertexData> vertexList(PP_SL);
	Array<StudioVertexData> shapeVertsList(PP_SL);	// shape key verts
	Array<int32> indexList(PP_SL);

	vertexList.reserve(dstGroup->numVertices);
	shapeVertsList.reserve(dstGroup->numVertices);
	indexList.reserve(dstGroup->numVertices);

	for(const DSVertex& srcVertex : srcGroup->verts)
	{
		const StudioVertexData newVertex = MakeStudioVertex(srcVertex);

		// add vertex or point to existing vertex if found duplicate
		const int foundVertex = arrayFindIndexF(vertexList, [&](const StudioVertexData& vert) {
			return CompareVertex(vert, newVertex);
		});

		if(foundVertex == -1)
		{
			const int newIndex = vertexList.append(newVertex);
			indexList.append(newIndex);

			// modify vertex by shape key
			if( modShapeKey )
			{
				StudioVertexData& shapeVert = shapeVertsList.append();
				shapeVert = newVertex;
				ApplyShapeKeyOnVertex(modShapeKey, srcVertex, shapeVert, 1.0f);
			}
		}
		else
			indexList.append(foundVertex);

		if (srcVertex.weights.numElem())
			vertexStreamsAvailableBits |= STUDIO_VERTFLAG_BONEWEIGHT;
		else if (srcVertex.color.pack() != UINT_MAX)
			vertexStreamsAvailableBits |= STUDIO_VERTFLAG_COLOR;
	}

	if((float)indexList.numElem() / (float)3.0f != (int)indexList.numElem() / (int)3)
	{
		MsgError("Model group has invalid triangles!\n");
		return;
	}

	auto usedVertList = ArrayRef<StudioVertexData>(modShapeKey ? shapeVertsList : vertexList);

	// calculate rest of tangent space
	for(int32 i = 0; i < indexList.numElem(); i+=3)
	{
		Vector3D tangent;
		Vector3D binormal;
		Vector3D unormal;

		const int32 idx0 = indexList[i];
		const int32 idx1 = indexList[i+1];
		const int32 idx2 = indexList[i+2];

		ComputeTriangleTBN(	usedVertList[idx0].posUvs.point,usedVertList[idx1].posUvs.point, usedVertList[idx2].posUvs.point,
							usedVertList[idx0].posUvs.texCoord,usedVertList[idx1].posUvs.texCoord, usedVertList[idx2].posUvs.texCoord,
							unormal,
							tangent,
							binormal);

		const float triArea = 1.0f;// TriangleArea(usedVertList[idx0].point, usedVertList[idx1].point, usedVertList[idx2].point);

		//usedVertList[idx0].tbn.normal += unormal * fTriangleArea;
		usedVertList[idx0].tbn.tangent += tangent * triArea;
		usedVertList[idx0].tbn.binormal += binormal * triArea;

		//usedVertList[idx1].tbn.normal += unormal * triArea;
		usedVertList[idx1].tbn.tangent += tangent* triArea;
		usedVertList[idx1].tbn.binormal += binormal * triArea;

		//usedVertList[idx2].tbn.normal += unormal * triArea;
		usedVertList[idx2].tbn.tangent += tangent * triArea;
		usedVertList[idx2].tbn.binormal += binormal * triArea;
	}

	// normalize resulting tangent space
	for(int32 i = 0; i < usedVertList.numElem(); i++)
	{
		//usedVertList[i].tbn.normal = normalize(usedVertList[i].tbn.normal);
		usedVertList[i].tbn.tangent = normalize(usedVertList[i].tbn.tangent);
		usedVertList[i].tbn.binormal = normalize(usedVertList[i].tbn.binormal);
	}

	{
		Array<int> outIndices(PP_SL);

		// optimize vertex cache
		{
			MsgInfo("Optimizing group '%s'...\n", srcGroup->texture.ToCString());
			outIndices.setNum(indexList.numElem());
			meshopt_optimizeVertexCache(outIndices.ptr(), indexList.ptr(), indexList.numElem(), usedVertList.numElem());
			indexList.swap(outIndices);
		}

		if (simplifyThreshold > 0.0f)
		{
			MsgInfo("Simplifying group '%s' by %.2f threshold...\n", srcGroup->texture.ToCString(), simplifyThreshold);

			constexpr float simplifyTargetError = 0.1f;

			float lodError = 0.0f;
			outIndices.setNum(indexList.numElem());
			const int indexCount = meshopt_simplify(outIndices.ptr(), indexList.ptr(), indexList.numElem(),
				usedVertList[0].posUvs.point, usedVertList.numElem(), sizeof(usedVertList[0]),
				int(indexList.numElem() * simplifyThreshold), simplifyTargetError, 0u, &lodError);

			if(indexList.numElem() > indexCount)
				MsgInfo("   Simplified %d >>> %d, lod error %g\n", indexList.numElem(), indexCount, lodError);

			outIndices.setNum(indexCount, true);
			indexList.swap(outIndices);

			// TODO: also need to optimize vertex fetch
			//meshopt_optimizeVertexFetch();
		}

		// stripify group
		{
			const int stripBreakIdx = 0;	// TODO: fix bug in render

			MsgInfo("Generating strips for group '%s'...\n", srcGroup->texture.ToCString());
			outIndices.setNum(meshopt_stripifyBound(indexList.numElem()));
			const int indexCount = meshopt_stripify(outIndices.ptr(), indexList.ptr(), indexList.numElem(), usedVertList.numElem(), stripBreakIdx);

#ifdef _DEBUG
			// debug check with unstripify
			{
				Array<int> tmpIndices(PP_SL);
				tmpIndices.setNum(meshopt_unstripifyBound(outIndices.numElem()));
				const int unstripifyIndexCount = meshopt_unstripify(tmpIndices.ptr(), outIndices.ptr(), outIndices.numElem(), stripBreakIdx);

				ASSERT_MSG(unstripifyIndexCount >= indexList.numElem(), "debug check failed, stripify produces less triangles (unstrupify results %d indices, %d original)", unstripifyIndexCount, indexList.numElem());
			}
#endif

			outIndices.setNum(indexCount, true);
			indexList.swap(outIndices);
			dstGroup->primitiveType = STUDIO_PRIM_TRI_STRIP;
		}
	}

	// set index count and now that is triangle strip
	dstGroup->numIndices = indexList.numElem();
	dstGroup->numVertices = usedVertList.numElem();

	//WRT_TEXT("MODEL GROUP DATA");

	// set write offset for vertex buffer
	dstGroup->vertexOffset = WRITE_RELATIVE_OFS(dstGroup);

#if ENABLE_OLD_VERTEX_FORMAT == 0
	dstGroup->vertexType = vertexStreamsAvailableBits;

	ASSERT(vertexStreamsAvailableBits & (STUDIO_VERTFLAG_POS_UV | STUDIO_VERTFLAG_TBN));
	WRITE_RESERVE_NUM(studioVertexPosUv_t, dstGroup->numVertices);
	WRITE_RESERVE_NUM(studioVertexTBN_t, dstGroup->numVertices);

	if (vertexStreamsAvailableBits & STUDIO_VERTFLAG_BONEWEIGHT)
		WRITE_RESERVE_NUM(studioBoneWeight_t, dstGroup->numVertices);

	if (vertexStreamsAvailableBits & STUDIO_VERTFLAG_COLOR)
		WRITE_RESERVE_NUM(studioVertexColor_t, dstGroup->numVertices);

	for (int32 i = 0; i < dstGroup->numVertices; i++)
	{
		const StudioVertexData& vertData = usedVertList[i];

		studioVertexPosUv_t* vertPosUv = dstGroup->pPosUvs(i);
		studioVertexTBN_t* vertTbn = dstGroup->pTBNs(i);
		studioBoneWeight_t* vertBoneWeight = dstGroup->pBoneWeight(i);
		studioVertexColor_t* vertColor = dstGroup->pColor(i);

		*vertPosUv = vertData.posUvs;
		*vertTbn = vertData.tbn;

		if (vertexStreamsAvailableBits & STUDIO_VERTFLAG_BONEWEIGHT)
			*vertBoneWeight = vertData.boneWeights;

		if (vertexStreamsAvailableBits & STUDIO_VERTFLAG_COLOR)
			*vertColor = vertData.color;
	}
#else
	WRITE_RESERVE_NUM(studioVertexDesc_t, dstGroup->numVertices);

	// now fill studio verts
	for(int32 i = 0; i < dstGroup->numVertices; i++)
	{
		const StudioVertexData& vertData = usedVertList[i];
		studioVertexDesc_t& destVertDesc = *dstGroup->pVertex(i);

		destVertDesc.point = vertData.posUvs.point;
		destVertDesc.texCoord = vertData.posUvs.texCoord;

		destVertDesc.tangent = vertData.tbn.tangent;
		destVertDesc.binormal = vertData.tbn.binormal;
		destVertDesc.normal = vertData.tbn.normal;

		destVertDesc.boneweights = vertData.boneWeights;
	}
#endif

	// set write offset for index buffer
	dstGroup->indicesOffset = WRITE_RELATIVE_OFS(dstGroup);
	WRITE_RESERVE_NUM(uint32, dstGroup->numIndices);

	// now fill studio indexes
	for(uint32 i = 0; i < dstGroup->numIndices; i++)
		*dstGroup->pVertexIdx(i) = indexList[i];

	MsgWarning("   written %d %s\n", 
		dstGroup->primitiveType == STUDIO_PRIM_TRI_STRIP ? (dstGroup->numIndices - 2) : (dstGroup->numIndices / 3),
		dstGroup->primitiveType == STUDIO_PRIM_TRI_STRIP ? "strip primitives" : "triangles");
}

//************************************
// Writes models
//************************************
void CEGFGenerator::WriteModels(studioHdr_t* header, IVirtualStream* stream)
{
	/*
	Structure:

		studioMeshGroupDesc_t		models[numModels]
		studioMeshGroupDesc_t		groups[numMeshes]

		studioVertexDesc_t		vertices[sumVertsOfGroups]
		uint32					indices[sumIndicesOfGroups]
		
	*/

	Array<GenModel*> writeModels{ PP_SL };
	for (const GenLODList& lodList : m_modelLodLists)
	{
		if (!lodList.used)
			continue;

		for (const int modelIdx : lodList.lodmodels)
			writeModels.append(&m_modelrefs[modelIdx]);
	}

	// Write models
	header->numMeshGroups = writeModels.numElem();
	header->meshGroupsOffset = WRITE_OFS;
	WRITE_RESERVE_NUM( studioMeshGroupDesc_t, writeModels.numElem() );

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < writeModels.numElem(); i++)
	{
		GenModel& modelRef = *writeModels[i];
		studioMeshGroupDesc_t* pDesc = header->pMeshGroupDesc(i);

		modelRef.meshGroupIdx = i;

		pDesc->numMeshes = modelRef.model->meshes.numElem();
		pDesc->meshesOffset = WRITE_RELATIVE_OFS( pDesc );
		pDesc->transformIdx = EGF_INVALID_IDX;

		// TODO: transforms from each GenModel
		
		// pDesc->transformIdx = m_transforms.numElem();
		// studiotransform_t& modelTransform = m_transforms.append();
		// modelTransform.attachBoneIdx = 0;

		WRITE_RESERVE_NUM( studioMeshDesc_t, pDesc->numMeshes );
	}

	//WRT_TEXT("MODEL GROUPS OFFSET");

	// add models used by body groups
	// FIXME: Body groups will need a remapping once some models are unused
	for(int i = 0; i < writeModels.numElem(); i++)
	{
		const GenModel& modelRef = *writeModels[i];
		studioMeshGroupDesc_t* pDesc = header->pMeshGroupDesc(i);

		// write groups
		for(int j = 0; j < pDesc->numMeshes; j++)
		{
			studioMeshDesc_t* groupDesc = pDesc->pMesh(j);

			// shape key modifier (if available)
			DSShapeKey* key = (modelRef.shapeIndex != -1) ? modelRef.shapeData->shapes[modelRef.shapeIndex] : nullptr;
			WriteGroup(header, stream, modelRef.model->meshes[j], key, modelRef.simplifyThreshold, groupDesc);

			Msg("Wrote group %s:%d", modelRef.name.ToCString(), j);

			if (key)
				Msg(", shape key: %s", key->name.ToCString());

			if(groupDesc->materialIndex != -1)
				Msg(", material used: %s", m_usedMaterials[groupDesc->materialIndex]->materialname);

			Msg("\n");
		}
	}
}

//************************************
// Writes LODs
//************************************
void CEGFGenerator::WriteLods(studioHdr_t* header, IVirtualStream* stream)
{
	int writeModelsCount = 0;
	Array<GenLODList*> writeLodLists{ PP_SL };

	for (GenLODList& lodList : m_modelLodLists)
	{
		if (!lodList.used)
			continue;
		writeLodLists.append(&lodList);
		writeModelsCount += lodList.lodmodels.numElem();
	}

	header->lodsOffset = WRITE_OFS;
	header->numLods = writeLodLists.numElem();
	WRITE_RESERVE_NUM(studioLodModel_t, writeLodLists.numElem());

	ASSERT(writeModelsCount == header->numMeshGroups);

	for(int i = 0; i < writeLodLists.numElem(); i++)
	{
		GenLODList& lodList = *writeLodLists[i];
		studioLodModel_t* pLod = header->pLodModel(i);

		lodList.writeIdx = i;

		for(int j = 0; j < MAX_MODEL_LODS; j++)
		{
			const int modelIdx = (j < lodList.lodmodels.numElem()) ? m_modelrefs[lodList.lodmodels[j]].meshGroupIdx : -1;
			pLod->modelsIndexes[j] = (modelIdx == -1) ? EGF_INVALID_IDX : modelIdx;
		}
	}

	header->lodParamsOffset = WRITE_OFS;
	header->numLodParams = m_lodparams.numElem();
	WRITE_RESERVE_NUM(studioLodParams_t, m_lodparams.numElem());

	for(int i = 0; i < m_lodparams.numElem(); i++)
	{
		header->pLodParams(i)->distance = m_lodparams[i].distance;
		header->pLodParams(i)->flags = m_lodparams[i].flags;
	}
}

//************************************
// Writes body groups
//************************************
void CEGFGenerator::WriteBodyGroups(studioHdr_t* header, IVirtualStream* stream)
{
	header->bodyGroupsOffset = WRITE_OFS;
	header->numBodyGroups = m_bodygroups.numElem();
	WRITE_RESERVE_NUM(studioBodyGroup_t, m_bodygroups.numElem());

	for(int i = 0; i <  m_bodygroups.numElem(); i++)
	{
		studioBodyGroup_t& bodyGrp = *header->pBodyGroups(i);
		bodyGrp = m_bodygroups[i];
		bodyGrp.lodModelIndex = m_modelLodLists[bodyGrp.lodModelIndex].writeIdx;
	}
}

//************************************
// Writes attachments
//************************************
void CEGFGenerator::WriteAttachments(studioHdr_t* header, IVirtualStream* stream)
{
	header->transformsOffset = WRITE_OFS;
	header->numTransforms = m_transforms.numElem();
	WRITE_RESERVE_NUM(studioTransform_t, m_transforms.numElem());

	for(int i = 0; i < m_transforms.numElem(); i++)
		*header->pTransform(i) = m_transforms[i];
}

//************************************
// Writes IK chainns
//************************************
void CEGFGenerator::WriteIkChains(studioHdr_t* header, IVirtualStream* stream)
{
	header->ikChainsOffset = WRITE_OFS;
	header->numIKChains = m_ikchains.numElem();
	WRITE_RESERVE_NUM(studioIkChain_t, m_ikchains.numElem());

	for(int i = 0; i < m_ikchains.numElem(); i++)
	{
		const GenIKChain& srcChain = m_ikchains[i];
		studioIkChain_t* chain = header->pIkChain(i);

		chain->numLinks = srcChain.link_list.numElem();

		strcpy(chain->name, srcChain.name);

		chain->linksOffset = WRITE_RELATIVE_OFS(header->pIkChain(i));
		WRITE_RESERVE_NUM(studioIkLink_t, chain->numLinks);

		// write link list flipped
		for(int j = 0; j < chain->numLinks; j++)
		{
			const int link_id = (chain->numLinks - 1) - j;

			const GenIKLink& link = srcChain.link_list[link_id];

			Msg("IK chain bone id: %d\n", link.bone->refBone->boneIdx);

			chain->pLink(j)->bone = link.bone->refBone->boneIdx;
			chain->pLink(j)->mins = link.mins;
			chain->pLink(j)->maxs = link.maxs;
			chain->pLink(j)->damping = link.damping;
		}
	}
}

//************************************
// Writes material descs
//************************************
void CEGFGenerator::WriteMaterialDescs(studioHdr_t* header, IVirtualStream* stream)
{
	header->materialsOffset = WRITE_OFS;
	header->numMaterials = m_usedMaterials.numElem();
	WRITE_RESERVE_NUM(studioMaterialDesc_t, m_usedMaterials.numElem());

	// get used materials
	for(int i = 0; i < m_usedMaterials.numElem(); i++)
	{
		GenMaterialDesc* mat = m_usedMaterials[i];

		studioMaterialDesc_t* matDesc = header->pMaterial(i);
		strcpy(matDesc->materialname, fnmPathStripExt(mat->materialname));
	}
	

	// write material groups
	for (int i = 0; i < m_matGroups.numElem(); i++)
	{
		GenMaterialGroup* grp = m_matGroups[i];
		int materialGroupStart = header->numMaterials;

		for (int j = 0; j < grp->materials.numElem(); j++)
		{
			GenMaterialDesc& baseMat = m_materials[j];

			if (!baseMat.used)
				continue;

			int usedMaterialIdx = arrayFindIndex(m_usedMaterials, &baseMat);
			GenMaterialDesc& mat = grp->materials[usedMaterialIdx];

			header->numMaterials++;

			studioMaterialDesc_t* matDesc = header->pMaterial(materialGroupStart + j);
			strcpy(matDesc->materialname, fnmPathStripExt(mat.materialname));

			WRITE_RESERVE(studioMaterialDesc_t);
		}
	}
}

//************************************
// Writes material change-dirs
//************************************
void CEGFGenerator::WriteMaterialPaths(studioHdr_t* header, IVirtualStream* stream)
{
	header->materialSearchPathsOffset = WRITE_OFS;
	header->numMaterialSearchPaths = m_matpathes.numElem();
	WRITE_RESERVE_NUM(materialPathDesc_t, m_matpathes.numElem());

	for(int i = 0; i < m_matpathes.numElem(); i++)
		*header->pMaterialSearchPath(i) = m_matpathes[i];
}

//************************************
// Writes Motion package paths
//************************************
void CEGFGenerator::WriteMotionPackageList(studioHdr_t* header, IVirtualStream* stream)
{
	header->packagesOffset = WRITE_OFS;
	header->numMotionPackages = m_motionpacks.numElem();
	WRITE_RESERVE_NUM(motionPackageDesc_t, m_motionpacks.numElem());

	for(int i = 0; i < m_motionpacks.numElem(); i++)
		*header->pPackage(i) = m_motionpacks[i];
}

//************************************
// Writes bones
//************************************
void CEGFGenerator::WriteBones(studioHdr_t* header, IVirtualStream* stream)
{
	header->bonesOffset = WRITE_OFS;
	header->numBones = m_bones.numElem();
	WRITE_RESERVE_NUM(studioBoneDesc_t, m_bones.numElem());

	for(int i = 0; i < m_bones.numElem(); i++)
	{
		DSBone* srcBone = m_bones[i].refBone;
		studioBoneDesc_t* destBone = header->pBone(i);

		strcpy(destBone->name, srcBone->name);

		destBone->parent = srcBone->parentIdx;
		destBone->position = srcBone->position;
		destBone->rotation = srcBone->angles;
	}
}

void CEGFGenerator::Validate(studioHdr_t* header, const char* stage)
{
	Array<const GenModel*> writeModels{ PP_SL };
	for (const GenLODList& lodList : m_modelLodLists)
	{
		if (!lodList.used)
			continue;

		for (int modelIdx : lodList.lodmodels)
			writeModels.append(&m_modelrefs[modelIdx]);
	}

	// perform post-write basic validation
	ASSERT(header->numMeshGroups == writeModels.numElem());

	for (int i = 0; i < header->numMeshGroups; ++i)
	{
		const GenModel& modelRef = *writeModels[i];

		studioMeshGroupDesc_t* pDesc = header->pMeshGroupDesc(i);
		ASSERT_MSG(pDesc->numMeshes == modelRef.model->meshes.numElem(), "numMeshes invalid after %s", stage);
	}
}

//************************************
// Writes EGF model
//************************************
bool CEGFGenerator::GenerateEGF()
{
	CMemoryStream egfStream(nullptr, VS_OPEN_WRITE, FILEBUFFER_EQGF, PP_SL);

	// Make header
	studioHdr_t* header = (studioHdr_t*)egfStream.GetBasePointer();
	memset(header, 0, sizeof(studioHdr_t));

	// Basic header data
	header->ident = EQUILIBRIUM_MODEL_SIGNATURE;
	header->version = EQUILIBRIUM_MODEL_VERSION;

#if ENABLE_OLD_VERTEX_FORMAT == 0
	header->flags = STUDIO_FLAG_NEW_VERTEX_FMT;
#else
	header->flags = 0;
#endif

	header->length = sizeof(studioHdr_t);

	// set model name
	fnmPathFixSeparators(m_outputFilename);
	strncpy(header->modelName, m_outputFilename, elementsOf(header->modelName));

	egfStream.Write(header, 1, sizeof(studioHdr_t));

	// write models
	WriteModels(header, &egfStream);
	Validate(header, "Write models");
	
	// write lod info and data
	WriteLods(header, &egfStream);
	Validate(header, "Write lods");
	
	// write body groups
	WriteBodyGroups(header, &egfStream);
	Validate(header, "Write body groups");

	// write attachments
	WriteAttachments(header, &egfStream);
	Validate(header, "Write attachments");

	// write ik chains
	WriteIkChains(header, &egfStream);
	Validate(header, "Write Ik Chains");
	
	if(m_notextures == false)
	{
		// write material descs and paths
		WriteMaterialDescs(header, &egfStream);
		Validate(header, "Write material descs");

		WriteMaterialPaths(header, &egfStream);
		Validate(header, "Write material paths");
	}

	// write motion packages
	WriteMotionPackageList(header, &egfStream);
	Validate(header, "Write motion pack list");

	// write bones
	WriteBones(header, &egfStream);
	Validate(header, "Write bones");

	// set the size of file (size with header), for validation purposes
	header->length = egfStream.GetSize();

	egfStream.Seek(0, VS_SEEK_SET);
	egfStream.Write(header, 1, sizeof(studioHdr_t));

	int totalTris = 0;
	int totalVerts = 0;
	for (int i = 0; i < header->numMeshGroups; i++)
	{
		studioMeshGroupDesc_t* pMeshGroupDesc = header->pMeshGroupDesc(i);

		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			studioMeshDesc_t* pMesh = pMeshGroupDesc->pMesh(j);
			totalVerts += pMesh->numVertices;
			switch (pMesh->primitiveType)
			{
			case STUDIO_PRIM_TRIANGLES:
				totalTris += pMesh->numIndices / 3;
				break;
			case STUDIO_PRIM_TRI_STRIP:
				totalTris += pMesh->numIndices - 2;
				break;
			}
		}
	}

	Msg(" total vertices: %d\n", totalVerts);
	Msg(" total triangles: %d\n", totalTris);
	Msg(" models: %d\n", header->numMeshGroups);
	Msg(" body groups: %d\n", header->numBodyGroups);
	Msg(" bones: %d\n", header->numBones);
	Msg(" lods: %d\n", header->numLods);
	Msg(" materials: %d\n", header->numMaterials);
	Msg(" ik chains: %d\n", header->numIKChains);
	Msg(" search paths: %d\n", header->numMaterialSearchPaths);
	Msg("   Wrote %d bytes:\n", header->length);

	g_fileSystem->MakeDir(fnmPathExtractPath(m_outputFilename), SP_MOD);

	// open output model file
	IFilePtr file = g_fileSystem->Open(m_outputFilename.ToCString(), "wb", -1);
	if(file)
	{
		MsgWarning("\nWriting EGF '%s'\n", m_outputFilename.ToCString());

		// write model
		egfStream.WriteToStream(file);
	}
	else
	{
		MsgError("File creation denined, can't save compiled model\n");
	}

	return true;
}