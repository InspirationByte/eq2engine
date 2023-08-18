//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Geometry Format Writer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "math/Utility.h"
#include "EGFGenerator.h"
#include "utils/AdjacentTriangles.h"

#include "dsm_loader.h"
#include "dsm_esm_loader.h"

#define USE_ACTC

#ifdef USE_ACTC
extern "C"{
#include "actc/tc.h"
}
#endif // USE_ACTC

using namespace SharedModel;

const char* GetACTCErrorString(int result)
{
	switch(result)
	{
		case ACTC_ALLOC_FAILED:
			return "ACTC_ALLOC_FAILED";

		case ACTC_DURING_INPUT:
			return "ACTC_DURING_INPUT";

		case ACTC_DURING_OUTPUT:
			return "ACTC_DURING_OUTPUT";

		case ACTC_IDLE:
			return "ACTC_IDLE";

		case ACTC_INVALID_VALUE:
			return "ACTC_INVALID_VALUE";

		case ACTC_DATABASE_EMPTY:
			return "ACTC_DATABASE_EMPTY";

		case ACTC_DATABASE_CORRUPT:
			return "ACTC_DATABASE_CORRUPT";
	}

	return "OTHER ERROR";
}

// EGF file buffer
#define FILEBUFFER_EQGF (16 * 1024 * 1024)

#define WRITE_RESERVE(type)				stream->Seek(sizeof(type), VS_SEEK_CUR)
#define WRITE_RESERVE_NUM(type, num)	stream->Seek(sizeof(type)*num, VS_SEEK_CUR)

#define WRITE_OFS						stream->Tell()		// write offset over header
#define WRITE_RELATIVE_OFS(obj)			(stream->Tell() - ((ubyte*)obj - (ubyte*)header))	// write offset over object

//---------------------------------------------------------------------------------------

// from exporter - compares two verts
static bool CompareVertex(const studioVertexDesc_t &v0, const studioVertexDesc_t &v1)
{
	static constexpr const float VERT_MERGE_EPS = 0.001f;
	static constexpr const float WEIGHT_MERGE_EPS = 0.001f;

	if (compare_epsilon(v0.point, v1.point, VERT_MERGE_EPS) &&
		compare_epsilon(v0.normal, v1.normal, VERT_MERGE_EPS) &&
		compare_epsilon(v0.texCoord, v1.texCoord, VERT_MERGE_EPS))
	{
		// check each bone weight
		int numEqual = 0;
		for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; ++i)
		{
			bool match = false;
			for (int j = 0; j < MAX_MODEL_VERTEX_WEIGHTS; ++j)
			{
				if (v0.boneweights.bones[i] == v1.boneweights.bones[j] && fsimilar(v0.boneweights.weight[i], v1.boneweights.weight[j], WEIGHT_MERGE_EPS))
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

// finds vertex index
static int FindVertexInList(const Array<studioVertexDesc_t>& verts, const studioVertexDesc_t &vertex)
{
	for( int i = 0; i < verts.numElem(); i++ )
	{
		if ( CompareVertex(verts[i],vertex) )
		{
			return i;
		}
	}

	return -1;
}

static studioVertexDesc_t MakeStudioVertex(const DSVertex& vert)
{
	studioVertexDesc_t vertex;

	vertex.point = vert.position;
	vertex.texCoord = vert.texcoord;
	vertex.normal = vert.normal;

	// zero the binormal and tangents because we will use a sum of it
	vertex.binormal = vec3_zero;
	vertex.tangent = vec3_zero;

	// reset
	for(int j = 0; j < MAX_MODEL_VERTEX_WEIGHTS; j++)
	{
		vertex.boneweights.bones[j] = -1;
		vertex.boneweights.weight[j] = 0.0f;
	}

	// assign bones and it's weights
	int weightCnt = 0;
	for(int j = 0; j < vert.weights.numElem(); j++)
	{
		if (vert.weights[j].bone == -1)
			continue;

		vertex.boneweights.bones[weightCnt] = vert.weights[j].bone;
		vertex.boneweights.weight[weightCnt] = vert.weights[j].weight;
		weightCnt++;
	}
	vertex.boneweights.numweights = weightCnt;
	ASSERT_MSG(weightCnt <= MAX_MODEL_VERTEX_WEIGHTS, "Too many weights on vertex (%d, max is %d)", vertex.boneweights.numweights, MAX_MODEL_VERTEX_WEIGHTS);

	return vertex;
}

void ApplyShapeKeyOnVertex( DSShapeKey* modShapeKey, const DSVertex& vert, studioVertexDesc_t& studioVert, float weight )
{
	for(int i = 0; i < modShapeKey->verts.numElem(); i++)
	{
		if( modShapeKey->verts[i].vertexId == vert.vertexId )
		{
			studioVert.point = lerp(studioVert.point, modShapeKey->verts[i].position, weight);
			studioVert.normal = lerp(studioVert.normal, modShapeKey->verts[i].normal, weight);
			return;
		}
	}
}

void ReverseStrip(int* indices, int len)
{
	int* original = PPNew int[len];
	memcpy(original, indices, len*sizeof(int));

	for (int i = 0; i != len; ++i)
		indices[i] = original[len-1-i];

	delete [] original;
}

int CEGFGenerator::UsedMaterialIndex(const char* pszName)
{
	int matIdx = GetMaterialIndex(pszName);

	GenMaterialDesc_t* material = &m_materials[matIdx];
	material->used++;

	return m_usedMaterials.addUnique(material);
}

// writes group
void CEGFGenerator::WriteGroup(studioHdr_t* header, IVirtualStream* stream, DSMesh* srcGroup, DSShapeKey* modShapeKey, studioMeshDesc_t* dstGroup)
{
	// DSM groups to be generated indices and optimized here
	dstGroup->materialIndex = m_notextures ? -1 : UsedMaterialIndex(srcGroup->texture);

	// triangle list by default
	dstGroup->primitiveType = EGFPRIM_TRIANGLES;

	dstGroup->numIndices = 0;
	dstGroup->numVertices = srcGroup->verts.numElem();

	Array<studioVertexDesc_t> vertexList(PP_SL);
	Array<studioVertexDesc_t> shapeVertsList(PP_SL);	// shape key verts
	Array<int32> indexList(PP_SL);

	vertexList.reserve(dstGroup->numVertices);
	shapeVertsList.reserve(dstGroup->numVertices);
	indexList.reserve(dstGroup->numVertices);

	for(int i = 0; i < dstGroup->numVertices; i++)
	{
		const studioVertexDesc_t vertex = MakeStudioVertex( srcGroup->verts[i] );

		// add vertex or point to existing vertex if found duplicate
		const int foundVertex = FindVertexInList(vertexList, vertex );

		if(foundVertex == -1)
		{
			const int newIndex = vertexList.append(vertex);
			indexList.append(newIndex);

			// modify vertex by shape key
			if( modShapeKey )
			{
				studioVertexDesc_t shapeVert = vertex;
				ApplyShapeKeyOnVertex(modShapeKey, srcGroup->verts[i], shapeVert, 1.0f);
				shapeVertsList.append(shapeVert);
			}
		}
		else
			indexList.append(foundVertex);
	}

	if((float)indexList.numElem() / (float)3.0f != (int)indexList.numElem() / (int)3)
	{
		MsgError("Model group has invalid triangles!\n");
		return;
	}

	auto usedVertList = ArrayRef<studioVertexDesc_t>(modShapeKey ? shapeVertsList : vertexList);

	// calculate rest of tangent space
	for(int32 i = 0; i < indexList.numElem(); i+=3)
	{
		Vector3D tangent;
		Vector3D binormal;
		Vector3D unormal;

		const int32 idx0 = indexList[i];
		const int32 idx1 = indexList[i+1];
		const int32 idx2 = indexList[i+2];

		ComputeTriangleTBN(	usedVertList[idx0].point,usedVertList[idx1].point, usedVertList[idx2].point, 
							usedVertList[idx0].texCoord,usedVertList[idx1].texCoord, usedVertList[idx2].texCoord,
							unormal,
							tangent,
							binormal);

		float fTriangleArea = 1.0f;// TriangleArea(usedVertList[idx0].point, usedVertList[idx1].point, usedVertList[idx2].point);

		//usedVertList[idx0].normal += unormal*fTriangleArea;
		usedVertList[idx0].tangent += tangent*fTriangleArea;
		usedVertList[idx0].binormal += binormal*fTriangleArea;

		//usedVertList[idx1].normal += unormal*fTriangleArea;
		usedVertList[idx1].tangent += tangent*fTriangleArea;
		usedVertList[idx1].binormal += binormal*fTriangleArea;

		//usedVertList[idx2].normal += unormal*fTriangleArea;
		usedVertList[idx2].tangent += tangent*fTriangleArea;
		usedVertList[idx2].binormal += binormal*fTriangleArea;
	}

	// normalize resulting tangent space
	for(int32 i = 0; i < usedVertList.numElem(); i++)
	{
		//usedVertList[i].normal = normalize(usedVertList[i].normal);
		usedVertList[i].tangent = normalize(usedVertList[i].tangent);
		usedVertList[i].binormal = normalize(usedVertList[i].binormal);
	}

#ifdef USE_ACTC
	{
		// optimize model using ACTC
		ACTCData* tc = actcNew();
		actcMakeEmpty(tc);

		if(tc == nullptr)
		{
			Msg("Model optimization disabled\n");
			goto skipOptimize;
		}
		else
			MsgInfo("Optimizing group '%s'...\n", srcGroup->texture.ToCString());

		// configure it to make strips
		actcParami(tc, ACTC_OUT_MIN_FAN_VERTS, INT_MAX);
		actcParami(tc, ACTC_OUT_HONOR_WINDING, ACTC_FALSE);

		MsgInfo("   phase 1: adding triangles to optimizer (%d tris)\n", indexList.numElem() / 3);
		actcBeginInput(tc);

		// input all indices
		for(int i = 0; i < indexList.numElem(); i+=3)
		{
			int result = actcAddTriangle(tc, indexList[i], indexList[i+1], indexList[i+2]);

			if(result < 0)
			{
				MsgError("   ACTC error: %s (%d)!\n", GetACTCErrorString(result), result);
				Msg("   optimization disabled\n");
				goto skipOptimize;
			}
		}

		actcEndInput(tc);

		MsgInfo("   phase 2: generate strips\n");

		actcBeginOutput(tc);

		Array<int32> optimizedIndices(PP_SL);
		optimizedIndices.reserve(indexList.numElem());

		int prim;
		uint32 v1;
		uint32 v2;
		uint32 v3 = -1;
		while((prim = actcStartNextPrim(tc, &v1, &v2)) != ACTC_DATABASE_EMPTY)
		{
			if(prim < 0)
			{
				MsgError("   actcStartNextPrim error: %s (%d)!\n", GetACTCErrorString(prim), prim);
				Msg("   optimization disabled\n");
				goto skipOptimize;
			}

			if(prim == ACTC_PRIM_FAN)
			{
				MsgError("   This should not generate triangle fans! Sorry!\n");
				Msg("   optimization disabled\n");
				goto skipOptimize;
			}

			if (optimizedIndices.numElem() + (optimizedIndices.numElem() > 2 ? 5 : 3) > indexList.numElem())
			{
				Msg("   optimization disabled due to no profit\n");
				goto skipOptimize;
			}

			if(optimizedIndices.numElem() > 2 && v1 != v3)
			{
				optimizedIndices.append( v3 );
				optimizedIndices.append( v1 );
			}

			optimizedIndices.append(v1);
			optimizedIndices.append(v2);

			int stripLength = 2;
			int result = ACTC_NO_ERROR;
			// start a primitive of type "prim" with v1 and v2
			while((result = actcGetNextVert(tc, &v3)) != ACTC_PRIM_COMPLETE)
			{
				if(result < 0)
				{
					MsgError("   actcGetNextVert error: %s (%d)!\n", GetACTCErrorString(result), result);
					Msg("   optimization disabled\n");
					goto skipOptimize;
				}

				if (optimizedIndices.numElem() + 1 > indexList.numElem())
				{
					Msg("   optimization disabled due to no profit\n");
					goto skipOptimize;
				}

				optimizedIndices.append( v3 );
				stripLength++;
			}

			if(stripLength & 1)
			{
				// add degenerate vertex to flip
				optimizedIndices.append( v3 );
			}
		}

		actcEndOutput( tc );
		actcDelete(tc);

		MsgWarning("   group optimization complete\n", optimizedIndices.numElem() - 2);

		// swap with new index list
		indexList.swap(optimizedIndices);

		dstGroup->primitiveType = EGFPRIM_TRI_STRIP;
	}
#endif // USE_ACTC

skipOptimize:

	// set index count and now that is triangle strip
	dstGroup->numIndices = indexList.numElem();
	dstGroup->numVertices = usedVertList.numElem();

	//WRT_TEXT("MODEL GROUP DATA");

	// set write offset for vertex buffer
	dstGroup->vertexOffset = WRITE_RELATIVE_OFS(dstGroup);
	WRITE_RESERVE_NUM(studioVertexDesc_t, dstGroup->numVertices);

	// now fill studio verts
	for(int32 i = 0; i < dstGroup->numVertices; i++)
		*dstGroup->pVertex(i) = usedVertList[i];

	// set write offset for index buffer
	dstGroup->indicesOffset = WRITE_RELATIVE_OFS(dstGroup);
	WRITE_RESERVE_NUM(uint32, dstGroup->numIndices);

	// now fill studio indexes
	for(uint32 i = 0; i < dstGroup->numIndices; i++)
		*dstGroup->pVertexIdx(i) = indexList[i];

	MsgWarning("   written %d triangles (strip including degenerates)\n", dstGroup->primitiveType == EGFPRIM_TRI_STRIP ? (dstGroup->numIndices - 2) : (dstGroup->numIndices / 3));
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

	for (int i = 0; i < m_modelrefs.numElem(); i++)
	{
		GenModel& modelRef = m_modelrefs[i];
		if (!modelRef.used)
			continue;
		writeModels.append(&modelRef);
	}

	// Write models
	header->numMeshGroups = writeModels.numElem();
	header->meshGroupsOffset = WRITE_OFS;
	WRITE_RESERVE_NUM( studioMeshGroupDesc_t, writeModels.numElem() );

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < writeModels.numElem(); i++)
	{
		const GenModel& modelRef = *writeModels[i];

		studioMeshGroupDesc_t* pDesc = header->pMeshGroupDesc(i);

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
			WriteGroup(header, stream, modelRef.model->meshes[j], key, groupDesc);

			if(groupDesc->materialIndex != -1)
				Msg("Wrote group %s:%d material used: %s\n", modelRef.name.ToCString(), j, m_usedMaterials[groupDesc->materialIndex]->materialname);
		}
	}
}

//************************************
// Writes LODs
//************************************
void CEGFGenerator::WriteLods(studioHdr_t* header, IVirtualStream* stream)
{
	Array<GenModel*> writeModels{ PP_SL };
	Array<GenLODList_t*> writeLods{ PP_SL };

	for (int i = 0; i < m_modelrefs.numElem(); i++)
	{
		GenModel& modelRef = m_modelrefs[i];
		if (!modelRef.used)
			continue;
		writeLods.append(&m_modelLodLists[i]);
		writeModels.append(&modelRef);
	}

	header->lodsOffset = WRITE_OFS;
	header->numLods = writeLods.numElem();
	WRITE_RESERVE_NUM(studioLodModel_t, writeLods.numElem());

	ASSERT(header->numLods == header->numMeshGroups);

	for(int i = 0; i < writeLods.numElem(); i++)
	{
		const GenLODList_t& lodList = *writeLods[i];
		studioLodModel_t* pLod = header->pLodModel(i);
		for(int j = 0; j < MAX_MODEL_LODS; j++)
		{
			const int modelIdx = (j < lodList.lodmodels.numElem()) ? max(-1, lodList.lodmodels[j]) : -1;
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
		*header->pBodyGroups(i) = m_bodygroups[i];
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
		GenMaterialDesc_t* mat = m_usedMaterials[i];
		EqString mat_no_ext(mat->materialname);

		studioMaterialDesc_t* matDesc = header->pMaterial(i);
		strcpy(matDesc->materialname, mat_no_ext.Path_Strip_Ext());
	}
	

	// write material groups
	for (int i = 0; i < m_matGroups.numElem(); i++)
	{
		GenMaterialGroup_t* grp = m_matGroups[i];
		int materialGroupStart = header->numMaterials;

		for (int j = 0; j < grp->materials.numElem(); j++)
		{
			GenMaterialDesc_t& baseMat = m_materials[j];

			if (!baseMat.used)
				continue;

			int usedMaterialIdx = arrayFindIndex(m_usedMaterials, &baseMat);
			GenMaterialDesc_t& mat = grp->materials[usedMaterialIdx];

			header->numMaterials++;

			EqString mat_no_ext(mat.materialname);

			studioMaterialDesc_t* matDesc = header->pMaterial(materialGroupStart + j);
			strcpy(matDesc->materialname, mat_no_ext.Path_Strip_Ext());

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
	Array<GenModel*> writeModels{ PP_SL };

	for (int i = 0; i < m_modelrefs.numElem(); i++)
	{
		GenModel& modelRef = m_modelrefs[i];
		if (!modelRef.used)
			continue;
		writeModels.append(&modelRef);
	}

	// perform post-wriote basic validation
	ASSERT(header->numMeshGroups == writeModels.numElem());

	for (int i = 0; i < header->numMeshGroups; ++i)
	{
		const GenModel& modelRef = *writeModels[i];

		studioMeshGroupDesc_t* pDesc = header->pMeshGroupDesc(i);
		ASSERT_MSG(pDesc->numMeshes == modelRef.model->meshes.numElem(), "numMeshes invalid after %s", stage);

		for (int j = 0; j < pDesc->numMeshes; j++)
		{
			studioMeshDesc_t* groupDesc = header->pMeshGroupDesc(i)->pMesh(j);

			for (int32 k = 0; k < groupDesc->numVertices; k++)
			{
				ASSERT_MSG(groupDesc->pVertex(k)->boneweights.numweights <= MAX_MODEL_VERTEX_WEIGHTS, "bone weights invalid after %s", stage);
			}
		}
	}
}

//************************************
// Writes EGF model
//************************************
bool CEGFGenerator::GenerateEGF()
{
	CMemoryStream memStream(nullptr, VS_OPEN_WRITE, FILEBUFFER_EQGF);

	// Make header
	studioHdr_t* header = (studioHdr_t*)memStream.GetBasePointer();
	memset(header, 0, sizeof(studioHdr_t));

	// Basic header data
	header->ident = EQUILIBRIUM_MODEL_SIGNATURE;
	header->version = EQUILIBRIUM_MODEL_VERSION;
	header->flags = 0;
	header->length = sizeof(studioHdr_t);

	// set model name
	strcpy( header->modelName, m_outputFilename.ToCString() );

	FixSlashes( header->modelName );

	memStream.Write(header, 1, sizeof(studioHdr_t));

	// write models
	WriteModels(header, &memStream);
	Validate(header, "Write models");
	
	// write lod info and data
	WriteLods(header, &memStream);
	Validate(header, "Write lods");
	
	// write body groups
	WriteBodyGroups(header, &memStream);
	Validate(header, "Write body groups");

	// write attachments
	WriteAttachments(header, &memStream);
	Validate(header, "Write attachments");

	// write ik chains
	WriteIkChains(header, &memStream);
	Validate(header, "Write Ik Chains");
	
	if(m_notextures == false)
	{
		// write material descs and paths
		WriteMaterialDescs(header, &memStream);
		Validate(header, "Write material descs");

		WriteMaterialPaths(header, &memStream);
		Validate(header, "Write material paths");
	}

	// write motion packages
	WriteMotionPackageList(header, &memStream);
	Validate(header, "Write motion pack list");

	// write bones
	WriteBones(header, &memStream);
	Validate(header, "Write bones");

	// set the size of file (size with header), for validation purposes
	header->length = memStream.Tell();

	memStream.Seek(0, VS_SEEK_SET);
	memStream.Write(header, 1, sizeof(studioHdr_t));

	memStream.Seek(header->length, VS_SEEK_SET);

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
			case EGFPRIM_TRIANGLES:
				totalTris += pMesh->numIndices / 3;
				break;
			case EGFPRIM_TRIANGLE_FAN:
			case EGFPRIM_TRI_STRIP:
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

	g_fileSystem->MakeDir(m_outputFilename.Path_Extract_Path().ToCString(), SP_MOD);

	// open output model file
	IFilePtr file = g_fileSystem->Open(m_outputFilename.ToCString(), "wb", -1);
	if(file)
	{
		MsgWarning("\nWriting EGF '%s'\n", m_outputFilename.ToCString());

		// write model
		memStream.WriteToStream(file);
	}
	else
	{
		MsgError("File creation denined, can't save compiled model\n");
	}

	return true;
}