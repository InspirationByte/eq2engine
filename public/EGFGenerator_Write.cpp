//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Geometry Format Writer
//////////////////////////////////////////////////////////////////////////////////

#define USE_ACTC

#ifdef USE_ACTC
extern "C"
{
#	include "tc.h"
}
#endif // USE_ACTC

#include "core_base_header.h"

#include "EGFGenerator.h"

#include "utils/geomtools.h"
#include "VirtualStream.h"

#include "dsm_esm_loader.h"

#include "mtriangle_framework.h"

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
#define OBJ_WRITE_OFS(obj)				(stream->Tell() - ((ubyte*)obj - stream->GetBasePointer()))	// write offset over object

//---------------------------------------------------------------------------------------

// from exporter - compares two verts
bool CompareVertex(const studiovertexdesc_t &v0, const studiovertexdesc_t &v1)
{
	if(	compare_epsilon(v0.point, v1.point, 0.0015f) &&
		compare_epsilon(v0.normal, v1.normal, 0.0015f) &&
		compare_epsilon(v0.texCoord, v1.texCoord, 0.0025f) &&
		v0.boneweights.bones[0] == v1.boneweights.bones[0] &&
		v0.boneweights.bones[1] == v1.boneweights.bones[1] &&
		v0.boneweights.bones[2] == v1.boneweights.bones[2] &&
		v0.boneweights.bones[3] == v1.boneweights.bones[3] &&
		fsimilar(v0.boneweights.weight[0],v1.boneweights.weight[0], 0.0025f) &&
		fsimilar(v0.boneweights.weight[1],v1.boneweights.weight[1], 0.0025f) &&
		fsimilar(v0.boneweights.weight[2],v1.boneweights.weight[2], 0.0025f) &&
		fsimilar(v0.boneweights.weight[3],v1.boneweights.weight[3], 0.0025f)
		)
		return true;

	return false;
}

// finds vertex index
int FindVertexInList(const DkList<studiovertexdesc_t>& verts, const studiovertexdesc_t &vertex)
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

//---------------------------------------------------------------------------------------

// from exporter - compares two verts
bool CompareVertexNoPosition(const studiovertexdesc_t &v0, const studiovertexdesc_t &v1)
{
	if(	compare_epsilon(v0.texCoord, v1.texCoord, 0.0001f) &&
		v0.boneweights.bones[0] == v1.boneweights.bones[0] &&
		v0.boneweights.bones[1] == v1.boneweights.bones[1] &&
		v0.boneweights.bones[2] == v1.boneweights.bones[2] &&
		v0.boneweights.bones[3] == v1.boneweights.bones[3] &&
		fsimilar(v0.boneweights.weight[0],v1.boneweights.weight[0], 0.0001f) &&
		fsimilar(v0.boneweights.weight[1],v1.boneweights.weight[1], 0.0001f) &&
		fsimilar(v0.boneweights.weight[2],v1.boneweights.weight[2], 0.0001f) &&
		fsimilar(v0.boneweights.weight[3],v1.boneweights.weight[3], 0.0001f)
		)
		return true;

	return false;
}

// finds vertex index
int FindVertexInListNoPosition(const DkList<studiovertexdesc_t>& verts, const studiovertexdesc_t &vertex)
{
	for( int i = 0; i < verts.numElem(); i++ )
	{
		if ( CompareVertexNoPosition(verts[i],vertex) )
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

	// assign bones and it's weights
	for(int j = 0; j < 4; j++)
	{
		vertex.boneweights.bones[j] = -1;
		vertex.boneweights.weight[j] = 0.0f;
	}

	vertex.boneweights.numweights = vert.weights.numElem();

	// debug
	int nActualWeights = 0;

	for(int j = 0; j < vertex.boneweights.numweights; j++)
	{
		vertex.boneweights.bones[j] = vert.weights[j].bone;

		if(vertex.boneweights.bones[j] != -1)
		{
			vertex.boneweights.weight[j] = vert.weights[j].weight;
			nActualWeights++;
		}
	}

	return vertex;
}

void ApplyShapeKeyOnVertex( esmshapekey_t* modShapeKey, const dsmvertex_t& vert, studiovertexdesc_t& studioVert )
{
	for(int i = 0; i < modShapeKey->verts.numElem(); i++)
	{
		if( modShapeKey->verts[i].vertexId == vert.vertexId )
		{
			studioVert.point = modShapeKey->verts[i].position;
			studioVert.normal = modShapeKey->verts[i].normal;
			return;
		}
	}
}

void ReverseStrip(int* indices, int len)
{
	int* original = new int[len];
	memcpy(original, indices, len*sizeof(int));

	for (int i = 0; i != len; ++i)
		indices[i] = original[len-1-i];

	delete [] original;
}

// writes group
void CEGFGenerator::WriteGroup(CMemoryStream* stream, dsmgroup_t* srcGroup, esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup)
{
	int material_index = -1;
	
	if(m_notextures == false)
		material_index = GetMaterialIndex( srcGroup->texture );

	// DSM groups to be generated indices and optimized here
	dstGroup->materialIndex = material_index;

	// triangle list by default
	dstGroup->primitiveType = EGFPRIM_TRIANGLES;

	dstGroup->numIndices = 0;
	dstGroup->numVertices = srcGroup->verts.numElem();

	DkList<studiovertexdesc_t>	gVertexList;
	DkList<studiovertexdesc_t>	gVertexList2;

	DkList<int32>				gIndexList;

	gVertexList.resize(gVertexList.numElem() + dstGroup->numVertices);
	gIndexList.resize(gVertexList.numElem() + dstGroup->numVertices);

	for(int i = 0; i < dstGroup->numVertices; i++)
	{
		studiovertexdesc_t vertex = MakeStudioVertex( srcGroup->verts[i] );
		studiovertexdesc_t vertex2 = vertex;

		// add vertex or point to existing vertex if found duplicate
		int nIndex;

		int equalVertex = FindVertexInList( gVertexList, vertex );

		if( equalVertex == -1 )
		{
			nIndex = gVertexList.numElem();

			gVertexList.append(vertex);

			// modify vertex by shape key
			if( modShapeKey )
			{
				ApplyShapeKeyOnVertex(modShapeKey, srcGroup->verts[i], vertex2);
				gVertexList2.append(vertex2);
			}
		}
		else
			nIndex = equalVertex;

		gIndexList.append(nIndex);
	}

	DkList<studiovertexdesc_t>& usedVertList = gVertexList;

	if( modShapeKey )
		usedVertList = gVertexList2;

	if((float)gIndexList.numElem() / (float)3.0f != (int)gIndexList.numElem() / (int)3)
	{
		MsgError("Model group has invalid triangles!\n");
		return;
	}

	// calculate rest of tangent space
	for(int32 i = 0; i < gIndexList.numElem(); i+=3)
	{
		Vector3D tangent;
		Vector3D binormal;
		Vector3D unormal;

		int32 idx0 = gIndexList[i];
		int32 idx1 = gIndexList[i+1];
		int32 idx2 = gIndexList[i+2];

		ComputeTriangleTBN(	usedVertList[idx0].point,usedVertList[idx1].point, usedVertList[idx2].point, 
							usedVertList[idx0].texCoord,usedVertList[idx1].texCoord, usedVertList[idx2].texCoord,
							unormal,
							tangent,
							binormal);

		float fTriangleArea = 1.0f; // TriangleArea(usedVertList[idx0].point,usedVertList[idx1].point, usedVertList[idx2].point);

		usedVertList[idx0].tangent += tangent*fTriangleArea;
		usedVertList[idx0].binormal += binormal*fTriangleArea;

		usedVertList[idx1].tangent += tangent*fTriangleArea;
		usedVertList[idx1].binormal += binormal*fTriangleArea;

		usedVertList[idx2].tangent += tangent*fTriangleArea;
		usedVertList[idx2].binormal += binormal*fTriangleArea;
	}

	// normalize resulting tangent space
	for(int32 i = 0; i < usedVertList.numElem(); i++)
	{
		usedVertList[i].tangent = normalize(usedVertList[i].tangent);
		usedVertList[i].binormal = normalize(usedVertList[i].binormal);
	}

#ifdef USE_ACTC
	{
		// optimize model using ACTC
		DkList<int32>	gOptIndexList;

		ACTCData* tc;

		tc = actcNew();
		if(tc == NULL)
		{
			Msg("Model optimization disabled\n");
			goto skipOptimize;
		}
		else
			MsgInfo("Optimizing group '%s'...\n", srcGroup->texture);

		// optimization code
		gOptIndexList.resize( gIndexList.numElem() );

		// configure it to make strips
		actcParami(tc, ACTC_OUT_MIN_FAN_VERTS, INT_MAX);
		actcParami(tc, ACTC_OUT_HONOR_WINDING, ACTC_FALSE);

		MsgInfo("   phase 1: adding triangles to optimizer (%d indices)\n", gIndexList.numElem());
		actcBeginInput(tc);

		// input all indices
		for(int i = 0; i < gIndexList.numElem(); i+=3)
		{
			int result = actcAddTriangle(tc, gIndexList[i], gIndexList[i+1], gIndexList[i+2]);

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

		int prim;
		uint32 v1, v2, v3;

		int nTriangleResults = 0;

		int stripLength = 0;
		int primCount = 0;

		actcBeginOutput(tc);

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

			if(primCount && v1 != v3)
			{
				// wait, WHAT? 
				gOptIndexList.append( v3 );
				gOptIndexList.append( v1 );
			}

			// reset
			stripLength = 2;

			gOptIndexList.append( v1 );
			gOptIndexList.append( v2 );

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

				gOptIndexList.append( v3 );
				stripLength++;
				nTriangleResults++;
			}

			if(stripLength & 1)
			{
				// add degenerate vertex
				gOptIndexList.append( v3 );
			}
			
			primCount++;
		}

		actcEndOutput( tc );

		// destroy
		actcDelete( tc );

		MsgWarning("   group optimization complete\n");

		// swap with new index list
		gIndexList.swap( gOptIndexList );

		dstGroup->primitiveType = EGFPRIM_TRI_STRIP;
	}
#endif // USE_ACTC

skipOptimize:

	// set index count and now that is triangle strip
	dstGroup->numIndices = gIndexList.numElem();
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
		*dstGroup->pVertexIdx(i) = gIndexList[i];
		
		WTYPE_ADVANCE(uint32);
	}
}

//************************************
// Writes models
//************************************
void CEGFGenerator::WriteModels(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

	// Write models
	header->modelsOffset = WRITE_OFS;
	header->numModels = m_modelrefs.numElem();

	// move offset forward, to not overwrite the studiomodeldescs
	WTYPE_ADVANCE_NUM( studiomodeldesc_t, m_modelrefs.numElem() );

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		egfcamodel_t& modelRef = m_modelrefs[i];
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
		egfcamodel_t& modelRef = m_modelrefs[i];
		if(!modelRef.used)
			continue;

		studiomodeldesc_t* pDesc = header->pModelDesc(i);

		// write groups
		for(int j = 0; j < pDesc->numGroups; j++)
		{
			modelgroupdesc_t* groupDesc = header->pModelDesc(i)->pGroup(j);

			// shape key modifier (if available)
			esmshapekey_t* key = (modelRef.shapeBy != -1) ? modelRef.shapeData->shapes[modelRef.shapeBy] : NULL;

			WriteGroup(stream, modelRef.model->groups[j], key, groupDesc);

			if(groupDesc->materialIndex != -1)
			{
				Msg("Group %s:%d material used: %s\n", modelRef.model->name, j, m_materials[groupDesc->materialIndex].materialname);
				m_materials[groupDesc->materialIndex].used++;
			}
		}
	}
}

//************************************
// Writes LODs
//************************************
void CEGFGenerator::WriteLods(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

	header->lodsOffset = WRITE_OFS;
	header->numLods = m_modellodrefs.numElem();

	for(int i = 0; i < m_modellodrefs.numElem(); i++)
	{
		for(int j = 0; j < MAX_MODELLODS; j++)
		{
			int refId = GetReferenceIndex( m_modellodrefs[i].lodmodels[j] );

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
void CEGFGenerator::WriteBodyGroups(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

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
void CEGFGenerator::WriteAttachments(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

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
void CEGFGenerator::WriteIkChains(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

	header->ikChainsOffset = WRITE_OFS;
	header->numIKChains = m_ikchains.numElem();

	// advance n times to give more space
	WTYPE_ADVANCE_NUM(studioikchain_t, m_ikchains.numElem());

	for(int i = 0; i < m_ikchains.numElem(); i++)
	{
		studioikchain_t* chain = header->pIkChain(i);

		chain->numLinks = m_ikchains[i].link_list.numElem();

		strcpy(chain->name, m_ikchains[i].name);

		chain->linksOffset = OBJ_WRITE_OFS(header->pIkChain(i));

		// write link list flipped
		//for(int j = g_ikchains[i].link_list.numElem()-1; j > -1; j--)
		for(int j = 0; j < m_ikchains[i].link_list.numElem(); j++)
		{
			int link_id = (m_ikchains[i].link_list.numElem() - 1) - j;

			ciklink_t& link = m_ikchains[i].link_list[link_id];

			Msg("IK chain bone id: %d\n", link.bone->referencebone->bone_id);

			chain->pLink(j)->bone = link.bone->referencebone->bone_id;
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
void CEGFGenerator::WriteMaterialDescs(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

	header->materialsOffset = WRITE_OFS;
	header->numMaterials = 0;

	for(int i = 0; i < m_materials.numElem(); i++)
	{
		egfcamaterialdesc_t& mat = m_materials[i];

		if(!mat.used)
			continue;

		header->numMaterials++;

		EqString mat_no_ext(mat.materialname);

		studiomaterialdesc_t* matDesc = header->pMaterial(i);
		strcpy(matDesc->materialname, mat_no_ext.Path_Strip_Ext().c_str());

		WTYPE_ADVANCE(studiomaterialdesc_t);
	}
}

//************************************
// Writes material change-dirs
//************************************
void CEGFGenerator::WriteMaterialPaths(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

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
void CEGFGenerator::WriteMotionPackageList(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

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
void CEGFGenerator::WriteBones(CMemoryStream* stream)
{
	studiohdr_t* header = (studiohdr_t*)stream->GetBasePointer();

	header->bonesOffset = WRITE_OFS;
	header->numBones = m_bones.numElem();

	for(int i = 0; i < m_bones.numElem(); i++)
	{
		dsmskelbone_t* srcBone = m_bones[i].referencebone;
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
	CMemoryStream memStream;
	if(!memStream.Open(NULL, VS_OPEN_WRITE, FILEBUFFER_EQGF))
	{
		MsgError("Failed to allocate memory stream!\n");
		return false;
	}

	// Make header
	studiohdr_t header;
	memset(&header, 0, sizeof(studiohdr_t));

	// Basic header data
	header.ident = EQUILIBRIUM_MODEL_SIGNATURE;
	header.version = EQUILIBRIUM_MODEL_VERSION;
	header.flags = 0;
	header.length = sizeof(studiohdr_t);

	// set model name
	strcpy( header.modelName, m_outputFilename.c_str() );

	FixSlashes( header.modelName );

	memStream.Write(&header, 1, sizeof(header));

	studiohdr_t* pHdr = (studiohdr_t*)memStream.GetBasePointer();

	// write models
	WriteModels(&memStream);
	
	// write lod info and data
	WriteLods(&memStream);
	
	// write body groups
	WriteBodyGroups(&memStream);

	// write attachments
	WriteAttachments(&memStream);

	// write ik chains
	WriteIkChains(&memStream);
	
	if(m_notextures == false)
	{
		// write material descs and paths
		WriteMaterialDescs(&memStream);
		WriteMaterialPaths(&memStream);
	}

	// write motion packages
	WriteMotionPackageList(&memStream);

	// write bones
	WriteBones(&memStream);

	// set the size of file (size with header), for validation purposes
	pHdr->length = memStream.Tell();

	Msg(" models: %d\n", pHdr->numModels);
	Msg(" body groups: %d\n", pHdr->numBodyGroups);
	Msg(" bones: %d\n", pHdr->numBones);
	Msg(" lods: %d\n", pHdr->numLods);
	Msg(" materials: %d\n", pHdr->numMaterials);
	Msg(" ik chains: %d\n", pHdr->numIKChains);
	Msg(" search paths: %d\n", pHdr->numMaterialSearchPaths);
	Msg("   Wrote %d bytes:\n", pHdr->length);

	// open model file
	IFile *file = g_fileSystem->Open(m_outputFilename.c_str(), "wb");
	if(file)
	{
		MsgWarning("\nWriting EGF '%s'\n", m_outputFilename.c_str());

		// write model
		memStream.WriteToFileStream(file);

		g_fileSystem->Close(file);

		file = NULL;
	}
	else
	{
		MsgError("File creation denined, can't save compiled model\n");
	}

	return true;
}