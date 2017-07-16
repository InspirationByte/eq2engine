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
#	include "../actc/tc.h"
}
#endif // USE_ACTC

#include "core_base_header.h"

#include "scriptcompile.h"
#include "utils/geomtools.h"

#include "dsm_esm_loader.h"

#include "mtriangle_framework.h"

// extern compilation-time data
extern DkList<egfcamodel_t>			g_modelrefs;	// all loaded model references
extern DkList<clodmodel_t>			g_modellodrefs;	// all LOD reference models including main LOD
extern DkList<studiolodparams_t>	g_lodparams;	// lod parameters
extern DkList<materialpathdesc_t>	g_matpathes;	// material paths
extern DkList<ikchain_t>			g_ikchains;		// ik chain list
extern DkList<cbone_t>				g_bones;		// bone list
extern DkList<studioattachment_t>	g_attachments;	// attachment list
extern DkList<studiobodygroup_t>	g_bodygroups;	// body group list
extern DkList<studiomaterialdesc_t>	g_materials;	// materials that this model uses
extern DkList<motionpackagedesc_t>	g_motionpacks;

extern bool g_notextures;

// EGF file buffer
#define FILEBUFFER_EQGF (16 * 1024 * 1024)

// data pointers
static ubyte *pData;
static ubyte *pStart;

// studio header for write
static studiohdr_t *g_hdr;

#define CHECK_OVERFLOW(arrsize, add_size)	ASSERT(WRITE_OFS+add_size < arrsize)

#define WTYPE_ADVANCE(type)				CHECK_OVERFLOW(FILEBUFFER_EQGF, sizeof(type)); pData += sizeof(type)
#define WTYPE_ADVANCE_NUM(type, num)	pData += (sizeof(type)*num)
#define WRT_TEXT(text)					strcpy((char*)pData, text); pData += strlen(text)+1

#define WRITE_OFS						(pData - pStart)		// write offset over header
#define OBJ_WRITE_OFS(obj)				((pData - pStart) - ((ubyte*)obj - pStart))	// write offset over object

void WriteAdvance(void* data, int size)
{
	memcpy(pData, data, size);

	pData += size;

	g_hdr->length = (pData - pStart);
}

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
   unsigned int i;
   int* original = new int[len];
   memcpy(original, indices, len*sizeof(int));

   for (i = 0; i != len; ++i)
      indices[i] = original[len-1-i];

   delete [] original;
}

// writes group
void WriteGroup(dsmgroup_t* srcGroup, esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup)
{
	int material_index = -1;
	
	if(g_notextures == false)
		material_index = GetMaterialIndex( srcGroup->texture );

	// DSM groups to be generated indices and optimized here

	dstGroup->materialIndex = material_index;

	// triangle list by default
	dstGroup->primitiveType = EGFPRIM_TRIANGLES;

	dstGroup->numindices = 0;
	dstGroup->numvertices = srcGroup->verts.numElem();



	DkList<studiovertexdesc_t>	gVertexList;
	DkList<studiovertexdesc_t>	gVertexList2;

	DkList<int32>				gIndexList;

	for(int i = 0; i < dstGroup->numvertices; i++)
	{
		gVertexList.resize(gVertexList.numElem() + dstGroup->numvertices);
		gIndexList.resize(gVertexList.numElem() + dstGroup->numvertices);

		studiovertexdesc_t vertex = MakeStudioVertex( srcGroup->verts[i] );
		studiovertexdesc_t vertex2 = vertex;

		// add vertex or point to existing vertex if found duplicate
		int nIndex = 0;

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

	// set index count
	dstGroup->numindices = gIndexList.numElem();
	dstGroup->numvertices = usedVertList.numElem();

	if((float)dstGroup->numindices / (float)3.0f != (int)dstGroup->numindices / (int)3)
	{
		MsgError("Model group has invalid triangles!\n");
		return;
	}

	// calculate rest of tangent space
	for(uint32 i = 0; i < dstGroup->numindices; i+=3)
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

	// normalize them once
	for(uint32 i = 0; i < dstGroup->numindices; i+=3)
	{
		int32 idx0 = gIndexList[i];
		int32 idx1 = gIndexList[i+1];
		int32 idx2 = gIndexList[i+2];

		usedVertList[idx0].tangent = normalize(usedVertList[idx0].tangent);
		usedVertList[idx0].binormal = normalize(usedVertList[idx0].binormal);

		usedVertList[idx1].tangent = normalize(usedVertList[idx1].tangent);
		usedVertList[idx1].binormal = normalize(usedVertList[idx1].binormal);

		usedVertList[idx2].tangent = normalize(usedVertList[idx2].tangent);
		usedVertList[idx2].binormal = normalize(usedVertList[idx2].binormal);
	}

#ifdef USE_ACTC
	// optimize model using ACTC
	DkList<int32>	gOptIndexList;

	ACTCData* tc;

	MsgInfo("Optimizing group...\n");

    tc = actcNew();
    if(tc == NULL)
	{
		Msg("Model optimization disabled\n");
		goto skipOptimize;
    }

	// optimization code

	gOptIndexList.resize( gIndexList.numElem() );

	// we want only triangle fans (say THANKS to DirectX10)
	actcParami(tc, ACTC_OUT_MIN_FAN_VERTS, INT_MAX);
	actcParami(tc, ACTC_OUT_HONOR_WINDING, ACTC_FALSE);

	actcBeginInput(tc);

	MsgInfo("phase 1: adding triangles to optimizer\n");

	// input all indices
	for(uint32 i = 0; i < gIndexList.numElem(); i+=3)
		actcAddTriangle(tc, gIndexList[i], gIndexList[i+1], gIndexList[i+2]);

	actcEndInput(tc);

	int prim;
	uint32 v1, v2, v3;

	MsgInfo("phase 2: generate triangles\n");

	int nTriangleResults = 0;

	int stripLength = 0;
	int primCount = 0;

	int evenPrimitives = 0;

	actcBeginOutput(tc);
	while(prim = actcStartNextPrim(tc, &v1, &v2) != ACTC_DATABASE_EMPTY)
	{
		if(prim == ACTC_PRIM_FAN)
		{
			MsgError("This should not generate triangle fans! Sorry!\n");
			Msg("optimization disabled\n");
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

		// start a primitive of type "prim" with v1 and v2
		while(actcGetNextVert(tc, &v3) != ACTC_PRIM_COMPLETE)
		{
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

	// destroy context
	actcDelete( tc );

	MsgWarning("group optimization complete\n");

	// replace
	gIndexList.clear();
	gIndexList.append( gOptIndexList );

	gOptIndexList.clear();

	dstGroup->primitiveType = EGFPRIM_TRI_STRIP;
#endif // USE_ACTC

	// set index count and now that is triangle strip
	dstGroup->numindices = gIndexList.numElem();

skipOptimize:

	//WRT_TEXT("MODEL GROUP DATA");

	// set write offset for vertex buffer
	dstGroup->vertexoffset = OBJ_WRITE_OFS(dstGroup);

	// now fill studio verts
	for(int32 i = 0; i < dstGroup->numvertices; i++)
	{
		*dstGroup->pVertex(i) = usedVertList[i];
		
		WTYPE_ADVANCE(studiovertexdesc_t);
	}

	// set write offset for index buffer
	dstGroup->indicesoffset = OBJ_WRITE_OFS(dstGroup);

	// now fill studio indexes
	for(uint32 i = 0; i < dstGroup->numindices; i++)
	{
		*dstGroup->pVertexIdx(i) = gIndexList[i];
		
		WTYPE_ADVANCE(uint32);
	}
}

//************************************
// Writes models
//************************************
void WriteModels()
{
	// Write models
	g_hdr->modelsoffset = WRITE_OFS;
	g_hdr->nummodels = g_modelrefs.numElem();

	// move offset forward, to not overwrite the studiomodeldescs
	WTYPE_ADVANCE_NUM( studiomodeldesc_t, g_modelrefs.numElem() );

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		studiomodeldesc_t* pDesc = g_hdr->pModelDesc(i);

		pDesc->numgroups = g_modelrefs[i].model->groups.numElem();
		pDesc->groupsoffset = OBJ_WRITE_OFS( pDesc );

		//Msg("Write offset: %d, structure offset: %d\n", WRITE_OFS, ((ubyte*)pDesc - pStart));

		// skip headers
		WTYPE_ADVANCE_NUM( modelgroupdesc_t, g_hdr->pModelDesc(i)->numgroups );
	}

	//WRT_TEXT("MODEL GROUPS OFFSET");

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		studiomodeldesc_t* pDesc = g_hdr->pModelDesc(i);

		// write groups
		for(int j = 0; j < pDesc->numgroups; j++)
		{
			modelgroupdesc_t* pDesc = g_hdr->pModelDesc(i)->pGroup(j);

			// shape key modifier (if available)
			esmshapekey_t* key = (g_modelrefs[i].shapeBy != -1) ? g_modelrefs[i].shapeData->shapes[g_modelrefs[i].shapeBy] : NULL;

			WriteGroup(g_modelrefs[i].model->groups[j], key, pDesc);
		}
	}
}

//************************************
// Writes LODs
//************************************
void WriteLods()
{
	g_hdr->lodsoffset = WRITE_OFS;
	g_hdr->numlods = g_modellodrefs.numElem();

	for(int i = 0; i < g_modellodrefs.numElem(); i++)
	{
		for(int j = 0; j < MAX_MODELLODS; j++)
		{
			int refId = GetReferenceIndex( g_modellodrefs[i].lodmodels[j] );

			g_hdr->pLodModel(i)->lodmodels[j] = refId;

			if(refId != -1)
				g_hdr->pModelDesc(i)->lod_index = j;

			WTYPE_ADVANCE(studiolodmodel_t);
		}
	}

	g_hdr->lodparamsoffset = WRITE_OFS;
	g_hdr->numlodparams = g_lodparams.numElem();

	for(int i = 0; i < g_lodparams.numElem(); i++)
	{
		g_hdr->pLodParams(i)->distance = g_lodparams[i].distance;
		g_hdr->pLodParams(i)->flags = g_lodparams[i].flags;

		WTYPE_ADVANCE(studiolodparams_t);
	}
}

//************************************
// Writes body groups
//************************************
void WriteBodyGroups()
{
	g_hdr->bodygroupsoffset = WRITE_OFS;
	g_hdr->numbodygroups = g_bodygroups.numElem();

	for(int i = 0; i <  g_bodygroups.numElem(); i++)
	{
		*g_hdr->pBodyGroups(i) = g_bodygroups[i];
		WTYPE_ADVANCE(studiobodygroup_t);
	}
}

//************************************
// Writes attachments
//************************************
void WriteAttachments()
{
	g_hdr->attachmentsoffset = WRITE_OFS;
	g_hdr->numattachments = g_attachments.numElem();

	for(int i = 0; i < g_attachments.numElem(); i++)
	{
		*g_hdr->pAttachment(i) = g_attachments[i];
		WTYPE_ADVANCE(studioattachment_t);
	}
}

//************************************
// Writes IK chainns
//************************************
void WriteIkChains()
{
	g_hdr->ikchainsoffset = WRITE_OFS;
	g_hdr->numikchains = g_ikchains.numElem();

	// advance n times to give more space
	WTYPE_ADVANCE_NUM(studioikchain_t, g_ikchains.numElem());

	for(int i = 0; i < g_ikchains.numElem(); i++)
	{
		g_hdr->pIkChain(i)->numlinks = g_ikchains[i].link_list.numElem();

		strcpy(g_hdr->pIkChain(i)->name, g_ikchains[i].name);

		g_hdr->pIkChain(i)->linksoffset = OBJ_WRITE_OFS(g_hdr->pIkChain(i));

		// write link list flipped
		//for(int j = g_ikchains[i].link_list.numElem()-1; j > -1; j--)
		for(int j = 0; j < g_ikchains[i].link_list.numElem(); j++)
		{
			int link_id = (g_ikchains[i].link_list.numElem() - 1) - j;

			Msg("IK chain bone id: %d\n", g_ikchains[i].link_list[link_id].bone->referencebone->bone_id);

			g_hdr->pIkChain(i)->pLink(j)->bone = g_ikchains[i].link_list[link_id].bone->referencebone->bone_id;
			g_hdr->pIkChain(i)->pLink(j)->mins = g_ikchains[i].link_list[link_id].mins;
			g_hdr->pIkChain(i)->pLink(j)->maxs = g_ikchains[i].link_list[link_id].maxs;
			g_hdr->pIkChain(i)->pLink(j)->damping = g_ikchains[i].link_list[link_id].damping;

			WTYPE_ADVANCE(studioiklink_t);
		}
	
	}
}

//************************************
// Writes material descs
//************************************
void WriteMaterialDescs()
{
	g_hdr->materialsoffset = WRITE_OFS;
	g_hdr->nummaterials = g_materials.numElem();

	for(int i = 0; i < g_materials.numElem(); i++)
	{
		EqString mat_no_ext = g_materials[i].materialname;
		mat_no_ext = mat_no_ext.Path_Strip_Ext();

		strcpy(g_materials[i].materialname, mat_no_ext.c_str());

		Msg("Material used: %s\n", g_materials[i].materialname);

		*g_hdr->pMaterial(i) = g_materials[i];
		WTYPE_ADVANCE(studiomaterialdesc_t);
	}
}

//************************************
// Writes material change-dirs
//************************************
void WriteMaterialCDs()
{
	g_hdr->searchpathdescsoffset = WRITE_OFS;
	g_hdr->numsearchpathdescs = g_matpathes.numElem();

	for(int i = 0; i < g_matpathes.numElem(); i++)
	{
		*g_hdr->pMaterialSearchPath(i) = g_matpathes[i];
		WTYPE_ADVANCE(materialpathdesc_t);
	}
}

//************************************
// Writes Motion package paths
//************************************
void WriteMotionPackages()
{
	g_hdr->packagesoffset = WRITE_OFS;
	g_hdr->nummotionpackages = g_motionpacks.numElem();

	for(int i = 0; i < g_motionpacks.numElem(); i++)
	{
		*g_hdr->pPackage(i) = g_motionpacks[i];
		WTYPE_ADVANCE(motionpackagedesc_t);
	}
}

//************************************
// Writes bones
//************************************
void WriteBones()
{
	g_hdr->bonesoffset = WRITE_OFS;
	g_hdr->numbones = g_bones.numElem();

	for(int i = 0; i < g_bones.numElem(); i++)
	{
		strcpy(g_hdr->pBone(i)->name, g_bones[i].referencebone->name);

		g_hdr->pBone(i)->parent = g_bones[i].referencebone->parent_id;
		g_hdr->pBone(i)->position = g_bones[i].referencebone->position;
		g_hdr->pBone(i)->rotation = g_bones[i].referencebone->angles;

		WTYPE_ADVANCE(bonedesc_t);
	}
}

//************************************
// Writes EGF model
//************************************
bool WriteEGF(const char* filename)
{
	MsgWarning("\nWriting EGF '%s'\n", filename);

	// Allocate data
	pStart	= (ubyte*)calloc( 1, FILEBUFFER_EQGF );

	memset(pStart, 0xFF, FILEBUFFER_EQGF);

	pData = pStart;

	// Make header
	g_hdr = (studiohdr_t*)pStart;

	// initialize header
	memset(g_hdr, 0, sizeof(studiohdr_t));

	// Basic header data
	g_hdr->ident = EQUILIBRIUM_MODEL_SIGNATURE;
	g_hdr->version = EQUILIBRIUM_MODEL_VERSION;
	g_hdr->flags = 0;
	g_hdr->length = sizeof(studiohdr_t);

	// set model name
	strcpy( g_hdr->modelname, filename );

	FixSlashes( g_hdr->modelname );

	// Advance header
	WTYPE_ADVANCE( studiohdr_t );

	// write models
	WriteModels();
	
	// write lod info and data
	WriteLods();
	
	// write body groups
	WriteBodyGroups();

	// write attachments
	WriteAttachments();

	// write ik chains
	WriteIkChains();
	
	if(g_notextures == false)
	{
		// write material descs
		WriteMaterialDescs();

		// write search pathes
		WriteMaterialCDs();
	}

	// write motion packages
	WriteMotionPackages();

	// write bones
	WriteBones();

	g_hdr->length = WRITE_OFS;

	// open model file
	DKFILE *file = g_fileSystem->Open(filename, "wb");
	if(file)
	{
		// write model
		file->Write(pStart,g_hdr->length,1);

		g_fileSystem->Close(file);

		file = NULL;

		Msg(" models: %d\n", g_hdr->nummodels);
		Msg(" body groups: %d\n", g_hdr->numbodygroups);
		Msg(" bones: %d\n", g_hdr->numbones);
		Msg(" lods: %d\n", g_hdr->numlods);
		Msg(" materials: %d\n", g_hdr->nummaterials);
		Msg(" ik chains: %d\n", g_hdr->numikchains);
		Msg(" search paths: %d\n", g_hdr->numsearchpathdescs);
		Msg("   Wrote %d bytes:\n", g_hdr->length);
	}
	else
	{
		MsgError("File creation denined, can't save compiled model\n");
	}

	// free writeable data
	free(pStart);

	return true;
}