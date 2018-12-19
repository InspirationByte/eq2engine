//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: Eq Model Script compilation
//
//********************************************************************************

#include "DebugInterface.h"
#include "runtime_compileegf.h"
#include "utils/geomtools.h"

#include "dsm_loader.h"
#include "model.h"

struct cbone_t
{
	dsmskelbone_t*		referencebone;

	DkList<cbone_t*>	childs;
	cbone_t*			parent;
};

struct ciklink_t
{
	Vector3D			mins;
	Vector3D			maxs;

	cbone_t*			bone;

	float				damping;
};

struct ikchain_t
{
	char name[44];

	DkList<ciklink_t> link_list;
};

struct clodmodel_t
{
	dsmmodel_t*			lodmodels[MAX_MODELLODS];
};


// model compilation-time data

char g_refspath[256];

DkList<dsmmodel_t*>				g_modelrefs;	// all loaded model references

DkList<clodmodel_t>				g_modellodrefs;	// all LOD reference models including main LOD
DkList<studiolodparams_t>		g_lodparams;	// lod parameters

DkList<materialpathdesc_t>		g_matpathes;	// material patches
DkList<ikchain_t>				g_ikchains;		// ik chain list
DkList<cbone_t>					g_bones;		// bone list
DkList<studioattachment_t>		g_attachments;	// attachment list
DkList<studiobodygroup_t>		g_bodygroups;	// body group list
DkList<studiomaterialdesc_t>	g_materials;	// materials that this model uses

bool g_notextures = false;

//************************************
// Finds bone
//************************************
cbone_t* FindBoneByName(char* pszName)
{
	for(int i = 0; i < g_bones.numElem(); i++)
	{
		if(!stricmp(g_bones[i].referencebone->name, pszName))
			return &g_bones[i];
	}
	return NULL;
}

//************************************
// Finds lod model
//************************************
clodmodel_t* FindModelLodGroupByName(char* pszName)
{
	for(int i = 0; i < g_modellodrefs.numElem(); i++)
	{
		if(!stricmp(g_modellodrefs[i].lodmodels[0]->name, pszName))
			return &g_modellodrefs[i];
	}
	return NULL;
}

//************************************
// Finds lod model, returns index
//************************************
int FindModelLodIdGroupByName(char* pszName)
{
	for(int i = 0; i < g_modellodrefs.numElem(); i++)
	{
		if(!stricmp(g_modellodrefs[i].lodmodels[0]->name, pszName))
			return i;
	}
	return -1;
}

//************************************
// Returns material index
//************************************
int GetMaterialIndex(char* pszName)
{
	for(int i = 0; i < g_materials.numElem(); i++)
	{
		if(!stricmp(g_materials[i].materialname, pszName))
			return i;
	}

	return -1;
}

//************************************
// Returns reference index
//************************************
int GetReferenceIndex(dsmmodel_t* pRef)
{
	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		if(g_modelrefs[i] == pRef)
			return i;
	}

	return -1;
}

//************************************
// Loads reference
//************************************
dsmmodel_t* ParseAndLoadModel(char* pszParamsString)
{
	dsmmodel_t *model = new dsmmodel_t;

	char ref_name[44];
	char part_name[44];

	int aCnt = sscanf(pszParamsString, "%s %s", part_name, ref_name);

	if(aCnt != 2)
	{
		ErrorMsg("Error! 'model' key must have name and path to reference in value\nusage: model <part name> <*.dsm path>");
		return false;
	}

	char ref_path[256];
	strcpy(ref_path, g_refspath);
	strcat(ref_path, "/");
	strcat(ref_path, ref_name);

	//DkStr finRefPath = DkStr(g_refspath) + DkStr("/") + ref_name;

	// load DSM model
	if(!LoadDSM(model, ref_path)) return false;

	int nVerts = GetTotalVertsOfDSM(model);

	if((float)nVerts/3.0f != nVerts/3)
	{
		ErrorMsg(varargs("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", ref_name));
		return false;
	}

	Msg("Adding reference '%s' '%s' with %d triangles, %d bones\n", ref_name, part_name, nVerts/3, model->bones.numElem());

	// check material list
	for(int i = 0; i < model->groups.numElem(); i++)
	{
		// if material is not found, add new one
		int found = GetMaterialIndex(model->groups[i]->texture);
		if(found == -1)
		{
			// create new material
			studiomaterialdesc_t desc;
			strcpy(desc.materialname, model->groups[i]->texture);
			g_materials.append(desc);
		}
	}

	// set model to as part name
	strcpy(model->name, part_name);

	// add model to list
	g_modelrefs.append(model);

	return model;
}

//************************************
// Loads reference models
//************************************
bool LoadModels(kvsection_t* pSection)
{
	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "model"))
		{
			// parse and load model

			dsmmodel_t* model = ParseAndLoadModel(pSection->pKeys[i]->value);
			if(!model) return false;
			else
			{
				clodmodel_t lod_model;

				memset(lod_model.lodmodels, NULL, sizeof(lod_model.lodmodels));

				lod_model.lodmodels[0] = model;

				// add a LOD model replacement table
				g_modellodrefs.append(lod_model);
			}
		}
	}

	if(g_modelrefs.numElem() == 0)
	{
		ErrorMsg("Error! Model must have at least one reference!\nusage: model <part name> <*.dsm path>");
		return false;
	}

	Msg("Added %d model references\n", g_modelrefs.numElem());

	return true;
}

//************************************
// Parses LOD data
//************************************
void ParseLodData(kvsection_t* pSection, float lod_distance)
{
	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "replace"))
		{
			dsmmodel_t* pReplaceModel = ParseAndLoadModel(pSection->pKeys[i]->value);
			if(!pReplaceModel) return;
			else
			{
				clodmodel_t* lodgroup = FindModelLodGroupByName(pReplaceModel->name);

				if(lodgroup)
					lodgroup->lodmodels[g_lodparams.numElem()] = pReplaceModel;
				else 
					ErrorMsg(varargs("No such reference named %s\n", pReplaceModel->name));
			}
		}
	}
}

//************************************
// Parses LODs
//************************************
void ParseLods(kvsection_t* pSection)
{
	// always add first lod
	studiolodparams_t lod;
	lod.distance = 0;
	lod.flags = 0; // TODO: STUDIO_MAIN_LOD

	// add new lod parameters
	g_lodparams.append(lod);

	for(int i = 0; i < pSection->sections.numElem(); i++)
	{
		char	lod_str[44];
		float	lod_distance;

		int nArgs = sscanf(pSection->sections[i]->sectionname, "%s %f", lod_str, &lod_distance);

		if(nArgs > 1 && !stricmp(lod_str, "lod"))
		{
			ParseLodData(pSection->sections[i], lod_distance);

			studiolodparams_t lod;
			lod.distance = lod_distance;
			lod.flags = 0;

			// add new lod parameters
			g_lodparams.append(lod);

			Msg("Added lod %d, distance: %2f\n", g_lodparams.numElem(), lod_distance);
		}
		else
		{
			//MsgError("Invalid lod section name string! usage: lod (distance)");
		}
	}
}

//************************************
// Load body groups
//************************************
bool LoadBodyGroups(kvsection_t* pSection)
{
	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "bodygroup"))
		{
			studiobodygroup_t bodygroup;

			char ref_name[44];
			
			int nArgs = sscanf(pSection->pKeys[i]->value, "%s %s", bodygroup.name, ref_name);

			if(nArgs < 2)
			{
				ErrorMsg(varargs("Invalid body group string format\n"));
				MsgWarning("usage: bodygroup \"(name) (reference)\"\n");
			}

			int body_index = FindModelLodIdGroupByName(ref_name);
			bodygroup.lodmodel_index = body_index;

			Msg("Adding body group %s\n", bodygroup.name);

			g_bodygroups.append(bodygroup);
		}
	}

	if(g_bodygroups.numElem() == 0)
	{
		ErrorMsg("Model must have at least one body group\n");
	}

	return true;
}

//************************************
// Checks bone for availablity in list
//************************************
bool BoneListCheckForBone(char* pszName, DkList<dsmskelbone_t*> &pBones)
{
	for(int i = 0; i < pBones.numElem(); i++)
	{
		if(!stricmp(pBones[i]->name, pszName))
			return true;
	}

	return false;
}

//************************************
// returns bone index for availablity in list
//************************************
int BoneListGetBoneIndex(char* pszName, DkList<dsmskelbone_t*> &pBones)
{
	for(int i = 0; i < pBones.numElem(); i++)
	{
		if(!stricmp(pBones[i]->name, pszName))
			return i;
	}

	return -1;
}


//************************************
// Remaps vertex bone indicesnew_bones
//************************************
void BoneRemapDSMGroup(dsmgroup_t* pGroup, DkList<dsmskelbone_t*> &old_bones, DkList<dsmskelbone_t*> &new_bones)
{
	for(int i = 0; i < pGroup->verts.numElem(); i++)
	{
		for(int j = 0; j < pGroup->verts[i].numWeights; j++)
		{
			int bone_id = pGroup->verts[i].weight_bones[j];

			// find bone in new list by bone name in old list
			int new_bone_id = BoneListGetBoneIndex(old_bones[bone_id]->name, new_bones);

			// point to bone in new list
			pGroup->verts[i].weight_bones[j] = new_bone_id;
		}
	}
}

//************************************
// Remaps vertex bone indices and merges skeleton
//************************************
void BoneMergeRemapDSM(dsmmodel_t* pDSM, DkList<dsmskelbone_t*> &new_bones)
{
	// remap groups
	for(int i = 0; i < pDSM->groups.numElem(); i++)
	{
		BoneRemapDSMGroup(pDSM->groups[i], pDSM->bones, new_bones);
	}

	// reset skeleton
	FreeDSMBones(pDSM);

	// and copy it
	for(int i = 0; i < new_bones.numElem(); i++)
	{
		// copy bone
		dsmskelbone_t* pBone = new dsmskelbone_t;
		memcpy(pBone, new_bones[i], sizeof(dsmskelbone_t));

		// add
		pDSM->bones.append(pBone);
	}
}

//************************************
// Merges all bones of all models
//************************************
void MergeBones()
{
	if(g_bones.numElem() == 0)
		return;

	// first, load all bones into the single list, as unique
	DkList<dsmskelbone_t*> allBones;

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		for(int j = 0; j < g_modelrefs[i]->bones.numElem(); j++)
		{
			// not found in new list? add
			if(!BoneListCheckForBone(g_modelrefs[i]->bones[j]->name, allBones))
			{
				// copy bone
				dsmskelbone_t* pBone = new dsmskelbone_t;
				memcpy(pBone, g_modelrefs[i]->bones[j], sizeof(dsmskelbone_t));

				// set new bone id
				pBone->bone_id = allBones.numElem();

				allBones.append(pBone);
			}
		}
	}

	// relink parent bones
	for(int i = 0; i < allBones.numElem(); i++)
	{
		int parentBoneIndex = BoneListGetBoneIndex(allBones[i]->parent_name, allBones);
		allBones[i]->parent_id = parentBoneIndex;
	}

	// All DSM vertices must be remapped now...
	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		BoneMergeRemapDSM(g_modelrefs[i], allBones);
	}

	for(int i = 0; i < allBones.numElem(); i++)
		delete allBones[i];

	allBones.clear();
}

//************************************
// Builds bone chains
//************************************
void BuildBoneChains()
{
	// make bone list first
	// NOTE: using a first reference because it's already remapped

	Msg("Bones:\n");

	for(int i = 0; i < g_modelrefs[0]->bones.numElem(); i++)
	{
		cbone_t cbone;
		cbone.referencebone = g_modelrefs[0]->bones[i];

		g_bones.append(cbone);

		Msg(" %s\n", cbone.referencebone->name);
	}

	Msg("  total bones: %d\n", g_bones.numElem());

	// set parents
	for(int i = 0; i < g_bones.numElem(); i++)
	{
		int parent_id = g_modelrefs[0]->bones[i]->parent_id;

		if(parent_id == -1)
			g_bones[i].parent = NULL;
		else
			g_bones[i].parent = &g_bones[parent_id];
	}

	// build child lists
	for(int i = 0; i < g_bones.numElem(); i++)
	{
		if(g_bones[i].parent)
			g_bones[i].parent->childs.append(&g_bones[i]);
	}
}

//************************************
// Loads material pathes to use in engine
//************************************
bool LoadMaterialPaths(kvsection_t* pSection)
{
	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "materialpath"))
		{
			materialpathdesc_t desc;
			strcpy(desc.m_szSearchPathString, pSection->pKeys[i]->value);
			
			g_matpathes.append(desc);
		}

		if(!stricmp(pSection->pKeys[i]->keyname, "notextures"))
		{
			g_notextures = KeyValuePair_GetValueBool(pSection->pKeys[i]);
		}
	}

	if(g_matpathes.numElem() == 0 && g_notextures == false)
	{
		ErrorMsg("Error! Model must have at least one materialpath!\n");
		return false;
	}

	if(g_matpathes.numElem() > 0)
		Msg("Added %d material paths\n", g_matpathes.numElem());

	return true;
}

//************************************
// Parses ik chain info from section
//************************************
void ParseIKChain(kvsection_t* pSection)
{
	ikchain_t ikCh;

	char effector_name[44];

	kvkeyvaluepair_t *pair = KeyValues_FindKey(pSection, "chain");
	if(!pair)
	{
		ErrorMsg("ikchain error: 'chain' key not found\nusage: chain (name) (effector bone)");
		return;
	}
	
	int nArg = sscanf(pair->value, "%s %s", ikCh.name, effector_name);

	if(nArg < 2)
	{
		ErrorMsg("Invalid ikchain 'chain' write format\nusage: chain (name) (effector bone)");
	}

	cbone_t* effector_chain = FindBoneByName(effector_name);

	if(!effector_chain)
	{
		ErrorMsg(varargs("ikchain: effector bone %s not found\n", effector_name));
		return;
	}

	// fill link list
	cbone_t* cparent = effector_chain;
	do
	{
		ciklink_t link;

		link.damping = 1.0f;

		link.bone = cparent;

		link.mins = Vector3D(1);
		link.maxs = Vector3D(-1);

		// add new link
		ikCh.link_list.append(link);

		// advance parent
		cparent = cparent->parent;
	}
	while(cparent != NULL/* && cparent->parent != NULL*/);

	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "damping"))
		{
			char link_name[44];
			float fDamp = 1.0f;

			int args = sscanf(pSection->pKeys[i]->value, "%s %f\n",link_name, &fDamp);
			if(args < 2)
			{
				ErrorMsg("Too few arguments for ik parameter 'damping', usage: damping (bone name) (damping)\n");
			}

			// search for link and apply parameter if found
			for(int j = 0; j < ikCh.link_list.numElem(); j++)
			{
				if(!stricmp(ikCh.link_list[j].bone->referencebone->name, link_name))
				{
					ikCh.link_list[j].damping = fDamp;
					break;
				}
			}
		}
		else if(!stricmp(pSection->pKeys[i]->keyname, "link_limits"))
		{
			char link_name[44];
			Vector3D mins;
			Vector3D maxs;

			int args = sscanf(pSection->pKeys[i]->value, "%s %f %f %f %f %f %f\n",link_name, &mins.x,&mins.y,&mins.z, &maxs.x,&maxs.y,&maxs.z);
			if(args < 7)
			{
				ErrorMsg("Too few arguments for ik parameter 'link_limits'\nusage: link_limits (bone name) (min limits vector3) (max limits vector3)");
			}

			// search for link and apply parameter if found
			for(int j = 0; j < ikCh.link_list.numElem(); j++)
			{
				if(!stricmp(ikCh.link_list[j].bone->referencebone->name, link_name))
				{
					ikCh.link_list[j].mins = mins;
					ikCh.link_list[j].maxs = maxs;
					break;
				}
			}
		}
	}

	g_ikchains.append(ikCh);
};

//************************************
// Loads ik chains if available
//************************************
void LoadIKChains(kvsection_t* pSection)
{
	for(int i = 0; i < pSection->sections.numElem(); i++)
	{
		if(!stricmp(pSection->sections[i]->sectionname, "ikchain"))
		{
			ParseIKChain(pSection->sections[i]);
		}
	}

	if(g_ikchains.numElem() > 0)
		Msg("Parsed %d IK chains\n", g_ikchains.numElem());
}

//************************************
// Loads attachments if available
//************************************
void LoadAttachments(kvsection_t* pSection)
{
	for(int i = 0; i < pSection->pKeys.numElem(); i++)
	{
		if(!stricmp(pSection->pKeys[i]->keyname, "attachment"))
		{
			studioattachment_t attach;

			char attach_to_bone[44];

			// parse attachment
			int sCnt = sscanf(pSection->pKeys[i]->value, "%s %s %f %f %f %f %f %f", attach.name, attach_to_bone,
				&attach.position.x,&attach.position.y,&attach.position.z,
				&attach.angles.x,&attach.angles.y,&attach.angles.z);

			if(sCnt < 8)
			{
				ErrorMsg("Invalid attachment definition\nusage: attachment \"(name) (bone name) (position x y z) (rotation x y z)");
				continue;
			}

			cbone_t* pBone = FindBoneByName(attach_to_bone);

			if(!pBone)
			{
				ErrorMsg(varargs("Can't find bone %s for attachment %s\n", attach_to_bone, attach.name));
				continue;
			}

			attach.bone_id = pBone->referencebone->bone_id;

			g_attachments.append(attach);

			MsgInfo("Adding attachment %s\n", attach.name);
		}
	}

	if(g_attachments.numElem())
		Msg("Total attachments: %d\n", g_attachments.numElem());
}

// EGF file buffer
#define FILEBUFFER_EQGF (4 * 1024 * 1024)

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

// from exporter - compares two verts
bool CompareVertex(studiovertexdesc_t &v0, studiovertexdesc_t &v1)
{
	if(v0.point == v1.point &&
		v0.normal == v1.normal &&
		v0.boneweights.bones[0] == v1.boneweights.bones[0] &&
		v0.boneweights.bones[1] == v1.boneweights.bones[1] &&
		v0.boneweights.bones[2] == v1.boneweights.bones[2] &&
		v0.boneweights.bones[3] == v1.boneweights.bones[3] &&
		v0.texCoord == v1.texCoord)
		return true;

	return false;
}

// finds vertex index
int FindVertexInList(DkList<studiovertexdesc_t>* verts, studiovertexdesc_t &vertex)
{
	for( int i = 0; i < verts->numElem(); i++ )
	{
		if ( CompareVertex(verts->ptr()[i],vertex) )
		{
			return i;
		}
	}

	return -1;
}

// writes group
void WriteGroup(dsmgroup_t* srcGroup, modelgroupdesc_t* dstGroup)
{
	int material_index = -1;
	
	if(g_notextures == false)
		material_index = GetMaterialIndex(srcGroup->texture);

	dstGroup->materialIndex = material_index;
	dstGroup->primitiveType = 0;
	dstGroup->numindices = 0;
	dstGroup->numvertices = srcGroup->verts.numElem();

	DkList<studiovertexdesc_t>	gVertexList;
	DkList<uint32>				gIndexList;

	for(int i = 0; i < dstGroup->numvertices; i++)
	{
		// fill vertex data

		studiovertexdesc_t vertex;

		vertex.point = srcGroup->verts[i].position;
		vertex.texCoord = srcGroup->verts[i].texcoord;
		vertex.normal = srcGroup->verts[i].normal;

		// zero the binormal and tangents because we will use a sum of it
		vertex.binormal = vec3_zero;
		vertex.tangent = vec3_zero;

		// assign bones and it's weights
		for(int j = 0; j < 4; j++)
		{
			vertex.boneweights.bones[j] = -1;
			vertex.boneweights.weight[j] = 0.0f;
		}

		vertex.boneweights.numweights = srcGroup->verts[i].numWeights;

		for(int j = 0; j < srcGroup->verts[i].numWeights; j++)
		{
			vertex.boneweights.bones[j] = srcGroup->verts[i].weight_bones[j];
			vertex.boneweights.weight[j] = srcGroup->verts[i].weights[j];
		}

		// add vertex or point to existing vertex if found duplicate
		int nIndex = 0;

		int equalVertex = FindVertexInList(&gVertexList, vertex);
		if(equalVertex == -1)
		{
			nIndex = gVertexList.numElem();

			gVertexList.append(vertex);
		}
		else
			nIndex = equalVertex;

		gIndexList.append(nIndex);
	}

	// set index count
	dstGroup->numindices = gIndexList.numElem();
	dstGroup->numvertices = gVertexList.numElem();

	if((float)dstGroup->numindices / (float)3.0f != (int)dstGroup->numindices / (int)3)
	{
		ErrorMsg("Model group has invalid triangles!\n");
		return;
	}

	// calculate rest of tangent space
	for(uint32 i = 0; i < dstGroup->numindices; i+=3)
	{
		Vector3D tangent;
		Vector3D binormal;
		Vector3D unormal;

		uint32 idx0 = gIndexList[i];
		uint32 idx1 = gIndexList[i+1];
		uint32 idx2 = gIndexList[i+2];

		ComputeTriangleTBN(	gVertexList[idx0].point,gVertexList[idx1].point, gVertexList[idx2].point, 
							gVertexList[idx0].texCoord,gVertexList[idx1].texCoord, gVertexList[idx2].texCoord,
							unormal,
							tangent,
							binormal);

		gVertexList[idx0].tangent += tangent;
		gVertexList[idx0].binormal += binormal;
		gVertexList[idx0].tangent = normalize(gVertexList[idx0].tangent);
		gVertexList[idx0].binormal = normalize(gVertexList[idx0].binormal);

		gVertexList[idx1].tangent += tangent;
		gVertexList[idx1].binormal += binormal;
		gVertexList[idx1].tangent = normalize(gVertexList[idx1].tangent);
		gVertexList[idx1].binormal = normalize(gVertexList[idx1].binormal);

		gVertexList[idx2].tangent += tangent;
		gVertexList[idx2].binormal += binormal;
		gVertexList[idx2].tangent = normalize(gVertexList[idx2].tangent);
		gVertexList[idx2].binormal = normalize(gVertexList[idx2].binormal);
	}

	//WRT_TEXT("MODEL GROUP DATA");

	// set write offset for vertex buffer
	dstGroup->vertexoffset = OBJ_WRITE_OFS(dstGroup);

	// now fill studio verts
	for(int32 i = 0; i < dstGroup->numvertices; i++)
	{
		*dstGroup->pVertex(i) = gVertexList[i];
		
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
	WTYPE_ADVANCE_NUM(studiomodeldesc_t, g_modelrefs.numElem());

	//WRT_TEXT("MODELS OFFSET");

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		studiomodeldesc_t* pDesc = g_hdr->pModelDesc(i);

		pDesc->numgroups = g_modelrefs[i]->groups.numElem();
		pDesc->groupsoffset = OBJ_WRITE_OFS(pDesc);

		//Msg("Write offset: %d, structure offset: %d\n", WRITE_OFS, ((ubyte*)pDesc - pStart));

		// skip headers
		WTYPE_ADVANCE_NUM(modelgroupdesc_t, g_hdr->pModelDesc(i)->numgroups);
	}

	//WRT_TEXT("MODEL GROUPS OFFSET");

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		studiomodeldesc_t* pDesc = g_hdr->pModelDesc(i);

		// write groups
		for(int j = 0; j < pDesc->numgroups; j++)
		{
			modelgroupdesc_t* pDesc = g_hdr->pModelDesc(i)->pGroup(j);

			WriteGroup(g_modelrefs[i]->groups[j], pDesc);
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
			int refId = GetReferenceIndex(g_modellodrefs[i].lodmodels[j]);

			g_hdr->pLodModel(i)->lodmodels[j] = refId;
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
	MsgWarning("\nWriting EGF '%s'\n", "egfview_temp.egf");

	// Allocate data
	pStart	= (ubyte*)calloc(1,FILEBUFFER_EQGF);

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
	g_hdr->length = sizeof(modelheader_t);

	// set model name
	strcpy(g_hdr->modelname, filename);

	// write advance
	WTYPE_ADVANCE(studiohdr_t);

	//WRT_TEXT("HEADER END");

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

	// write bones
	WriteBones();

	g_hdr->length = WRITE_OFS;

	// open model file
	FILE *file = GetFileSystem()->Open("egfview_temp.egf", "wb", SP_ROOT);
	if(file)
	{
		// write model
		fwrite(pStart,g_hdr->length,1,file);

		Msg(" models: %d\n", g_hdr->nummodels);
		Msg(" body groups: %d\n", g_hdr->numbodygroups);
		Msg(" bones: %d\n", g_hdr->numbones);
		Msg(" lods: %d\n", g_hdr->numlods);
		Msg(" materials: %d\n", g_hdr->nummaterials);
		Msg(" ik chains: %d\n", g_hdr->numikchains);
		Msg(" search paths: %d\n", g_hdr->numsearchpathdescs);
		Msg("   Wrote %d bytes:\n", g_hdr->length);

		GetFileSystem()->Close(file);
	}
	else
	{
		ErrorMsg("File creation denined, can't save compiled model\n");
	}

	free(pStart);

	return true;
}

void Cleanup()
{
	/*
	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		if(g_modelrefs[i])
		{
			FreeDSM(g_modelrefs[i]);
			delete g_modelrefs[i];
		}
	}
	*/

	g_modelrefs.clear();
	g_attachments.clear();
	g_bodygroups.clear();
	g_bones.clear();
	g_ikchains.clear();
	g_lodparams.clear();
	g_materials.clear();
	g_matpathes.clear();
	g_modellodrefs.clear();
}

//************************************
// main function of script compilation
//************************************
bool CompileESCScriptRT(const char* filename)
{
	// strip filename to set reference models path
	strcpy(g_refspath, filename);
	stripFileName(g_refspath);

	KeyValues g_script;

	MsgWarning("\nCompiling script \"%s\"\n", filename);

	if(g_script.LoadFromFile(filename))
	{
		// load all script data
		kvsection_t* mainsection = g_script.FindRootSection("CompileParams");
		if(mainsection)
		{
			// get new model filename
			kvkeyvaluepair_t* pPair = KeyValues_FindKey(mainsection, "modelfilename");
			if(!pPair)
			{
				ErrorMsg("No output model file name specified in the script (modelfilename)\n");
				return false;
			}

			MsgWarning("\nLoading models\n");

			// try load models
			if(!LoadModels(mainsection)) return false;

			MsgWarning("\nLoading LODs\n");
			// load lod/replacement data
			ParseLods(mainsection);

			// parse body groups - not implemented for now....
			if(!LoadBodyGroups(mainsection)) return false;

			MsgWarning("\nMerging bones\n");
			// merge bones first
			MergeBones();

			// build bone chain after loading models
			BuildBoneChains();

			MsgWarning("\nLoading material pathes\n");
			// load material paths - needed for material system.
			if(!LoadMaterialPaths(mainsection)) return false;

			MsgWarning("\nLoading attachments\n");
			// load attachments if available
			LoadAttachments(mainsection);

			MsgWarning("\nLoading IK chains\n");

			// load ik chains if available
			LoadIKChains(mainsection);

			// build egf model
			// and save it to file
			if(!WriteEGF(pPair->value)) return false;

			// clean all after compilation
			Cleanup();
		}
		else
		{
			ErrorMsg("No 'CompileParams' section found in script.\n");
			return false;
		}
	}
	else
	{
		ErrorMsg(varargs("Can't open %s\n", filename));
		return false;
	}

	return true;
}