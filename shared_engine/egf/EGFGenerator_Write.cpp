//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Geometry Format Writer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "math/Utility.h"
#include "EGFGenerator.h"

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

#define WTYPE_ADVANCE(type)				stream->Seek(sizeof(type), VS_SEEK_CUR)
#define WTYPE_ADVANCE_NUM(type, num)	stream->Seek(sizeof(type)*num, VS_SEEK_CUR)

#define WRITE_OFS						stream->Tell()		// write offset over header
#define OBJ_WRITE_OFS(obj)				(stream->Tell() - ((ubyte*)obj - (ubyte*)header))	// write offset over object

//---------------------------------------------------------------------------------------

// from exporter - compares two verts
static bool CompareVertex(const studiovertexdesc_t &v0, const studiovertexdesc_t &v1)
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
static int FindVertexInList(const Array<studiovertexdesc_t>& verts, const studiovertexdesc_t &vertex)
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

studiovertexdesc_t MakeStudioVertex(const dsmvertex_t& vert)
{
	studiovertexdesc_t vertex;

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

void ApplyShapeKeyOnVertex( esmshapekey_t* modShapeKey, const dsmvertex_t& vert, studiovertexdesc_t& studioVert, float weight )
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
void CEGFGenerator::WriteGroup(studiohdr_t* header, IVirtualStream* stream, dsmgroup_t* srcGroup, esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup)
{
	// DSM groups to be generated indices and optimized here
	dstGroup->materialIndex = m_notextures ? -1 : UsedMaterialIndex(srcGroup->texture);

	// triangle list by default
	dstGroup->primitiveType = EGFPRIM_TRIANGLES;

	dstGroup->numIndices = 0;
	dstGroup->numVertices = srcGroup->verts.numElem();

	Array<studiovertexdesc_t> vertexList(PP_SL);
	Array<studiovertexdesc_t> shapeVertsList(PP_SL);	// shape key verts
	Array<int32> indexList(PP_SL);

	vertexList.reserve(dstGroup->numVertices);
	shapeVertsList.reserve(dstGroup->numVertices);
	indexList.reserve(dstGroup->numVertices);

	for(int i = 0; i < dstGroup->numVertices; i++)
	{
		const studiovertexdesc_t vertex = MakeStudioVertex( srcGroup->verts[i] );

		// add vertex or point to existing vertex if found duplicate
		const int foundVertex = FindVertexInList(vertexList, vertex );

		if(foundVertex == -1)
		{
			const int newIndex = vertexList.append(vertex);
			indexList.append(newIndex);

			// modify vertex by shape key
			if( modShapeKey )
			{
				studiovertexdesc_t shapeVert = vertex;
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

	Array<studiovertexdesc_t>& usedVertList = modShapeKey ? shapeVertsList : vertexList;

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
			MsgInfo("Optimizing group '%s'...\n", srcGroup->texture);

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
				actcDelete( tc );
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
				MsgError("   ACTC error: %s (%d)!\n", GetACTCErrorString(prim), prim);
				Msg("   optimization disabled\n");
				actcDelete( tc );
				goto skipOptimize;
			}

			if(prim == ACTC_PRIM_FAN)
			{
				MsgError("   This should not generate triangle fans! Sorry!\n");
				Msg("   optimization disabled\n");
				actcDelete( tc );
				goto skipOptimize;
			}

			if (optimizedIndices.numElem() + (optimizedIndices.numElem() > 2 ? 5 : 3) > indexList.numElem())
			{
				Msg("   optimization disabled due to no profit\n");
				actcDelete(tc);
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
					MsgError("   ACTC error: %s (%d)!\n", GetACTCErrorString(result), result);
					Msg("   optimization disabled\n");
					actcDelete( tc );
					goto skipOptimize;
				}

				if (optimizedIndices.numElem() + 1 > indexList.numElem())
				{
					Msg("   optimization disabled due to no profit\n");
					actcDelete(tc);
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

		// destroy
		actcDelete( tc );

		MsgWarning("   group optimization complete, generated %d indices\n", optimizedIndices.numElem() - 2);

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
	dstGroup->vertexOffset = OBJ_WRITE_OFS(dstGroup);

	// now fill studio verts
	for(int32 i = 0; i < dstGroup->numVertices; i++)
	{
		*dstGroup->pVertex(i) = usedVertList[i];
		
		WTYPE_ADVANCE(studiovertexdesc_t);
	}

	// set write offset for index buffer
	dstGroup->indicesOffset = OBJ_WRITE_OFS(dstGroup);

	// now fill studio indexes
	for(uint32 i = 0; i < dstGroup->numIndices; i++)
	{
		*dstGroup->pVertexIdx(i) = indexList[i];
		
		WTYPE_ADVANCE(uint32);
	}
}

//************************************
// Writes models
//************************************
void CEGFGenerator::WriteModels(studiohdr_t* header, IVirtualStream* stream)
{
	/*
	Structure:

		studiomodeldesc_t		models[numModels]
		studiomodeldesc_t		groups[numGroups]

		studiovertexdesc_t		vertices[sumVertsOfGroups]
		uint32					indices[sumIndicesOfGroups]
		
	*/

	// Write models
	header->modelsOffset = WRITE_OFS;
	header->numModels = m_modelrefs.numElem();

	// move offset forward, to not overwrite the studiomodeldescs
	WTYPE_ADVANCE_NUM( studiomodeldesc_t, m_modelrefs.numElem() );

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		GenModel_t& modelRef = m_modelrefs[i];
		if(!modelRef.used)
			continue;

		studiomodeldesc_t* pDesc = header->pModelDesc(i);

		pDesc->numGroups = modelRef.model->groups.numElem();
		pDesc->groupsOffset = OBJ_WRITE_OFS( pDesc );

		// skip headers
		WTYPE_ADVANCE_NUM( modelgroupdesc_t, header->pModelDesc(i)->numGroups );
	}

	//WRT_TEXT("MODEL GROUPS OFFSET");

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		GenModel_t& modelRef = m_modelrefs[i];
		if(!modelRef.used)
			continue;

		studiomodeldesc_t* pDesc = header->pModelDesc(i);

		// write groups
		for(int j = 0; j < pDesc->numGroups; j++)
		{
			modelgroupdesc_t* groupDesc = header->pModelDesc(i)->pGroup(j);

			// shape key modifier (if available)
			esmshapekey_t* key = (modelRef.shapeBy != -1) ? modelRef.shapeData->shapes[modelRef.shapeBy] : nullptr;

			WriteGroup(header, stream, modelRef.model->groups[j], key, groupDesc);

			if(groupDesc->materialIndex != -1)
				Msg("Group %s:%d material used: %s\n", modelRef.model->name, j, m_usedMaterials[groupDesc->materialIndex]->materialname);
		}
	}
}

//************************************
// Writes LODs
//************************************
void CEGFGenerator::WriteLods(studiohdr_t* header, IVirtualStream* stream)
{
	header->lodsOffset = WRITE_OFS;
	header->numLods = m_modelLodLists.numElem();

	for(int i = 0; i < m_modelLodLists.numElem(); i++)
	{
		for(int j = 0; j < MAX_MODEL_LODS; j++)
		{
			int refId = GetReferenceIndex( m_modelLodLists[i].lodmodels[j] );

			header->pLodModel(i)->modelsIndexes[j] = refId;

			if(refId != -1)
				header->pModelDesc(i)->lodIndex = j;

			WTYPE_ADVANCE(studiolodmodel_t);
		}
	}

	header->lodParamsOffset = WRITE_OFS;
	header->numLodParams = m_lodparams.numElem();

	for(int i = 0; i < m_lodparams.numElem(); i++)
	{
		header->pLodParams(i)->distance = m_lodparams[i].distance;
		header->pLodParams(i)->flags = m_lodparams[i].flags;

		WTYPE_ADVANCE(studiolodparams_t);
	}
}

//************************************
// Writes body groups
//************************************
void CEGFGenerator::WriteBodyGroups(studiohdr_t* header, IVirtualStream* stream)
{
	header->bodyGroupsOffset = WRITE_OFS;
	header->numBodyGroups = m_bodygroups.numElem();

	for(int i = 0; i <  m_bodygroups.numElem(); i++)
	{
		*header->pBodyGroups(i) = m_bodygroups[i];
		WTYPE_ADVANCE(studiobodygroup_t);
	}
}

//************************************
// Writes attachments
//************************************
void CEGFGenerator::WriteAttachments(studiohdr_t* header, IVirtualStream* stream)
{
	header->attachmentsOffset = WRITE_OFS;
	header->numAttachments = m_attachments.numElem();

	for(int i = 0; i < m_attachments.numElem(); i++)
	{
		*header->pAttachment(i) = m_attachments[i];
		WTYPE_ADVANCE(studioattachment_t);
	}
}

//************************************
// Writes IK chainns
//************************************
void CEGFGenerator::WriteIkChains(studiohdr_t* header, IVirtualStream* stream)
{
	header->ikChainsOffset = WRITE_OFS;
	header->numIKChains = m_ikchains.numElem();

	// advance n times to give more space
	WTYPE_ADVANCE_NUM(studioikchain_t, m_ikchains.numElem());

	for(int i = 0; i < m_ikchains.numElem(); i++)
	{
		GenIKChain_t& srcChain = m_ikchains[i];
		studioikchain_t* chain = header->pIkChain(i);

		chain->numLinks = srcChain.link_list.numElem();

		strcpy(chain->name, srcChain.name);

		chain->linksOffset = OBJ_WRITE_OFS(header->pIkChain(i));

		// write link list flipped
		for(int j = 0; j < srcChain.link_list.numElem(); j++)
		{
			int link_id = (srcChain.link_list.numElem() - 1) - j;

			GenIKLink_t& link = srcChain.link_list[link_id];

			Msg("IK chain bone id: %d\n", link.bone->refBone->bone_id);

			chain->pLink(j)->bone = link.bone->refBone->bone_id;
			chain->pLink(j)->mins = link.mins;
			chain->pLink(j)->maxs = link.maxs;
			chain->pLink(j)->damping = link.damping;

			WTYPE_ADVANCE(studioiklink_t);
		}
	}
}

//************************************
// Writes material descs
//************************************
void CEGFGenerator::WriteMaterialDescs(studiohdr_t* header, IVirtualStream* stream)
{
	header->materialsOffset = WRITE_OFS;
	header->numMaterials = 0;
	//header->numMaterialGroups = 0;

	// get used materials
	for(int i = 0; i < m_usedMaterials.numElem(); i++)
	{
		GenMaterialDesc_t* mat = m_usedMaterials[i];

		header->numMaterials++;

		EqString mat_no_ext(mat->materialname);

		studiomaterialdesc_t* matDesc = header->pMaterial(i);
		strcpy(matDesc->materialname, mat_no_ext.Path_Strip_Ext().ToCString());

		WTYPE_ADVANCE(studiomaterialdesc_t);
	}

	// write material groups
	for (int i = 0; i < m_matGroups.numElem(); i++)
	{
		GenMaterialGroup_t* grp = m_matGroups[i];
		int materialGroupStart = header->numMaterials;

		//header->numMaterialGroups++;

		for (int j = 0; j < grp->materials.numElem(); j++)
		{
			GenMaterialDesc_t& baseMat = m_materials[j];

			if (!baseMat.used)
				continue;

			int usedMaterialIdx = m_usedMaterials.findIndex(&baseMat);
			GenMaterialDesc_t& mat = grp->materials[usedMaterialIdx];

			header->numMaterials++;

			EqString mat_no_ext(mat.materialname);

			studiomaterialdesc_t* matDesc = header->pMaterial(materialGroupStart + j);
			strcpy(matDesc->materialname, mat_no_ext.Path_Strip_Ext().ToCString());

			WTYPE_ADVANCE(studiomaterialdesc_t);
		}
	}
}

//************************************
// Writes material change-dirs
//************************************
void CEGFGenerator::WriteMaterialPaths(studiohdr_t* header, IVirtualStream* stream)
{
	header->materialSearchPathsOffset = WRITE_OFS;
	header->numMaterialSearchPaths = m_matpathes.numElem();

	for(int i = 0; i < m_matpathes.numElem(); i++)
	{
		*header->pMaterialSearchPath(i) = m_matpathes[i];
		WTYPE_ADVANCE(materialpathdesc_t);
	}
}

//************************************
// Writes Motion package paths
//************************************
void CEGFGenerator::WriteMotionPackageList(studiohdr_t* header, IVirtualStream* stream)
{
	header->packagesOffset = WRITE_OFS;
	header->numMotionPackages = m_motionpacks.numElem();

	for(int i = 0; i < m_motionpacks.numElem(); i++)
	{
		*header->pPackage(i) = m_motionpacks[i];
		WTYPE_ADVANCE(motionpackagedesc_t);
	}
}

//************************************
// Writes bones
//************************************
void CEGFGenerator::WriteBones(studiohdr_t* header, IVirtualStream* stream)
{
	header->bonesOffset = WRITE_OFS;
	header->numBones = m_bones.numElem();

	for(int i = 0; i < m_bones.numElem(); i++)
	{
		dsmskelbone_t* srcBone = m_bones[i].refBone;
		bonedesc_t* destBone = header->pBone(i);

		strcpy(destBone->name, srcBone->name);

		destBone->parent = srcBone->parent_id;
		destBone->position = srcBone->position;
		destBone->rotation = srcBone->angles;

		WTYPE_ADVANCE(bonedesc_t);
	}
}

//************************************
// Writes EGF model
//************************************
bool CEGFGenerator::GenerateEGF()
{
	CMemoryStream memStream(nullptr, VS_OPEN_WRITE, FILEBUFFER_EQGF);

	// Make header
	studiohdr_t* header = (studiohdr_t*)memStream.GetBasePointer();
	memset(header, 0, sizeof(studiohdr_t));

	// Basic header data
	header->ident = EQUILIBRIUM_MODEL_SIGNATURE;
	header->version = EQUILIBRIUM_MODEL_VERSION;
	header->flags = 0;
	header->length = sizeof(studiohdr_t);

	// set model name
	strcpy( header->modelName, m_outputFilename.ToCString() );

	FixSlashes( header->modelName );

	memStream.Write(header, 1, sizeof(studiohdr_t));

	// write models
	WriteModels(header, &memStream);
	
	// write lod info and data
	WriteLods(header, &memStream);
	
	// write body groups
	WriteBodyGroups(header, &memStream);

	// write attachments
	WriteAttachments(header, &memStream);

	// write ik chains
	WriteIkChains(header, &memStream);
	
	if(m_notextures == false)
	{
		// write material descs and paths
		WriteMaterialDescs(header, &memStream);
		WriteMaterialPaths(header, &memStream);
	}

	// write motion packages
	WriteMotionPackageList(header, &memStream);

	// write bones
	WriteBones(header, &memStream);

	// set the size of file (size with header), for validation purposes
	header->length = memStream.Tell();

	memStream.Seek(0, VS_SEEK_SET);
	memStream.Write(header, 1, sizeof(studiohdr_t));

	memStream.Seek(header->length, VS_SEEK_SET);

	int totalTris = 0;
	int totalVerts = 0;
	for (int i = 0; i < header->numModels; i++)
	{
		studiomodeldesc_t* pModelDesc = header->pModelDesc(i);

		for (int j = 0; j < pModelDesc->numGroups; j++)
		{
			modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);
			totalVerts += pGroup->numVertices;
			switch (pGroup->primitiveType)
			{
			case EGFPRIM_TRIANGLES:
				totalTris += pGroup->numIndices / 3;
				break;
			case EGFPRIM_TRIANGLE_FAN:
			case EGFPRIM_TRI_STRIP:
				totalTris += pGroup->numIndices - 2;
				break;
			}
		}
	}

	Msg(" total vertices: %d\n", totalVerts);
	Msg(" total triangles: %d\n", totalTris);
	Msg(" models: %d\n", header->numModels);
	Msg(" body groups: %d\n", header->numBodyGroups);
	Msg(" bones: %d\n", header->numBones);
	Msg(" lods: %d\n", header->numLods);
	Msg(" materials: %d\n", header->numMaterials);
	Msg(" ik chains: %d\n", header->numIKChains);
	Msg(" search paths: %d\n", header->numMaterialSearchPaths);
	Msg("   Wrote %d bytes:\n", header->length);

	g_fileSystem->MakeDir(m_outputFilename.Path_Extract_Path().ToCString(), SP_MOD);

	// open output model file
	IFile *file = g_fileSystem->Open(m_outputFilename.ToCString(), "wb", -1);
	if(file)
	{
		MsgWarning("\nWriting EGF '%s'\n", m_outputFilename.ToCString());

		// write model
		memStream.WriteToFileStream(file);

		g_fileSystem->Close(file);

		file = nullptr;
	}
	else
	{
		MsgError("File creation denined, can't save compiled model\n");
	}

	return true;
}