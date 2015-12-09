//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Model Script compilation
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "scriptcompile.h"
#include "physgen_process.h"

#include "dsm_esm_loader.h"

// model compilation-time data

char g_refspath[256];

DkList<egfcamodel_t>			g_modelrefs;	// all loaded model references

DkList<clodmodel_t>				g_modellodrefs;	// all LOD reference models including main LOD
DkList<studiolodparams_t>		g_lodparams;	// lod parameters


DkList<motionpackagedesc_t>		g_motionpacks;	// motion packages
DkList<materialpathdesc_t>		g_matpathes;	// material paths
DkList<ikchain_t>				g_ikchains;		// ik chain list
DkList<cbone_t>					g_bones;		// bone list
DkList<studioattachment_t>		g_attachments;	// attachment list
DkList<studiobodygroup_t>		g_bodygroups;	// body group list
DkList<studiomaterialdesc_t>	g_materials;	// materials that this model uses

Vector3D g_scalemodel = Vector3D(1);

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
	if(pRef == NULL)
		return -1;

	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		if(g_modelrefs[i].model == pRef)
			return i;
	}

	return -1;
}

egfcamodel_t LoadModel(const char* pszFileName)
{
	egfcamodel_t mod;

	mod.model = new dsmmodel_t;

	char ref_path[256];
	strcpy(ref_path, g_refspath);
	strcat(ref_path, "/");
	strcat(ref_path, pszFileName);

	EqString ext(_Es(ref_path).Path_Extract_Ext());

	// load shape keys
	if( !ext.CompareCaseIns("esx") )
	{
		mod.shapeData = new esmshapedata_t;

		Msg("Loading shape file '%s'\n", ref_path);

		if( LoadESXShapes( mod.shapeData, ref_path ))
		{
			MsgAccept("esx shapes loaded!\n");

			// copy reference file name and load it
			strcpy(ref_path, g_refspath);
			strcat(ref_path, "/");
			strcat(ref_path, mod.shapeData->reference.c_str());
		}
		else
		{
			delete mod.shapeData;
			mod.shapeData = NULL;
		}
	}

	// load DSM model
	if(!LoadSharedModel(mod.model, ref_path))
	{
		FreeEGFCaModel(mod);
		return mod;
	}

	int nVerts = GetTotalVertsOfDSM( mod.model );

	if((float)nVerts/3.0f != nVerts/3)
	{
		MsgError("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", ref_path);
		
		FreeEGFCaModel(mod);
		return mod;
	}

	// assign shape indexes
	if( mod.shapeData )
	{
		AssignShapeKeyVertexIndexes(mod.model, mod.shapeData);
	}

	// scale bones
	for(int i = 0; i < mod.model->bones.numElem(); i++)
	{
		mod.model->bones[i]->position *= g_scalemodel;
	}

	// check material list
	for(int i = 0; i < mod.model->groups.numElem(); i++)
	{
		// scale vertices
		for(int j = 0; j < mod.model->groups[i]->verts.numElem(); j++)
		{
			mod.model->groups[i]->verts[j].position *= g_scalemodel;
		}

		// if material is not found, add new one
		int found = GetMaterialIndex(mod.model->groups[i]->texture);
		if(found == -1)
		{
			// create new material
			studiomaterialdesc_t desc;
			strcpy(desc.materialname, mod.model->groups[i]->texture);
			g_materials.append(desc);
		}
	}

	// add model to list
	//g_modelrefs.append(model);

	return mod;
}

//************************************
// Frees model ref
//************************************
void FreeEGFCaModel( egfcamodel_t& mod )
{
	FreeDSM(mod.model);
	delete mod.model;

	if(mod.shapeData)
		FreeShapes(mod.shapeData);

	mod.model = NULL;
	mod.shapeData = NULL;
}

//************************************
// Loads reference
//************************************
dsmmodel_t* ParseAndLoadModels(kvkeybase_t* pKeyBase)
{
	DkList<EqString> modelfilenames;
	DkList<EqString> shapeByModels;

	if(pKeyBase->values.numElem() > 1)
	{
		// DRVSYN: vertex order for damaged model
		if(pKeyBase->values.numElem() > 3 && !stricmp(pKeyBase->values[2], "shapeby"))
		{
			const char* shapekeyName = KV_GetValueString(pKeyBase, 3, "");

			if(shapekeyName)
				Msg("Model shape key is '%s'\n", shapekeyName );

			shapeByModels.append(shapekeyName);
		}
		else
			shapeByModels.append("");

		modelfilenames.append( pKeyBase->values[1] );
	}

	// get section data
	for(int i = 0; i < pKeyBase->keys.numElem(); i++)
	{
		modelfilenames.append( pKeyBase->keys[i]->name );

		shapeByModels.append("");
	}

	// load the models
	DkList<egfcamodel_t> models;

	for(int i = 0; i < modelfilenames.numElem(); i++)
	{
		egfcamodel_t model = LoadModel( modelfilenames[i].GetData() );

		if(!model.model)
			continue;

		// set shapeby
		model.shapeBy = FindShapeKeyIndex(model.shapeData, shapeByModels[i].c_str());

		// set model to as part name
		strcpy(model.model->name, KV_GetValueString(pKeyBase, 0, "invalid_model_name"));

		int nVerts = GetTotalVertsOfDSM(model.model);

		if((float)nVerts/3.0f != nVerts/3)
		{
			MsgError("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", model.model->name);

			FreeEGFCaModel(model);

			continue;
		}

		Msg("Adding reference %s '%s' with %d triangles (in %d groups), %d bones\n", 
				modelfilenames[i].GetData(),
				model.model->name, 
				nVerts/3, 
				model.model->groups.numElem(), 
				model.model->bones.numElem());

		// add finally
		models.append( model );
		//g_modelrefs.append( model );
	}

	// merge the models
	if(models.numElem() > 1)
	{
		dsmmodel_t* merged = new dsmmodel_t;

		// set model to as part name
		strcpy(merged->name, KV_GetValueString(pKeyBase, 0, "invalid_model_name"));

		for(int i = 0; i < models.numElem(); i++)
		{
			if(i == 0)
			{
				merged->bones.append(models[i].model->bones);
				models[i].model->bones.clear();
			}

			merged->groups.append(models[i].model->groups);
			models[i].model->groups.clear();

			FreeEGFCaModel(models[i]);
		}

		egfcamodel_t mref;
		mref.model = merged;
		mref.shapeData = NULL;

		// add model to list
		g_modelrefs.append( mref );

		return merged;
	}
	else if(models.numElem() > 0)
	{
		// add model to list
		g_modelrefs.append( models[0] );

		return models[0].model;
	}
	else
	{
		MsgError("got model definition, but nothing added\n");
	}

	return NULL;
}

//************************************
// Loads reference models
//************************************
bool LoadModels(kvkeybase_t* pSection)
{
	// try apply global scale
	kvkeybase_t* pair = pSection->FindKeyBase("global_scale");

	if(pair)
		sscanf(pair->values[0], "%f %f %f", &g_scalemodel.x,&g_scalemodel.y,&g_scalemodel.z);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "model"))
		{
			// parse and load model
			dsmmodel_t* model = ParseAndLoadModels( pSection->keys[i] );

			if(!model)
				return false;
			else
			{
				clodmodel_t lod_model;

				memset(lod_model.lodmodels, NULL, sizeof(lod_model.lodmodels));

				// set model to lod 0
				lod_model.lodmodels[0] = model;

				// add a LOD model replacement table
				g_modellodrefs.append(lod_model);
			}
		}
	}

	if(g_modelrefs.numElem() == 0)
	{
		MsgError("Error! Model must have at least one reference!\n");
		MsgWarning("usage: model <part name> <*.esm or *.obj path>\n");
		return false;
	}

	Msg("Added %d model references\n", g_modelrefs.numElem());

	return true;
}

//************************************
// Parses LOD data
//************************************
void ParseLodData(kvkeybase_t* pSection, int lodIdx)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "replace"))
		{
			dsmmodel_t* pReplaceModel = ParseAndLoadModels( pSection->keys[i] );

			if(!pReplaceModel)
			{
				return;
			}
			else
			{
				clodmodel_t* lodgroup = FindModelLodGroupByName( pReplaceModel->name );

				if(lodgroup)
				{
					// set lod
					lodgroup->lodmodels[lodIdx] = pReplaceModel;
				}
				else 
					MsgError("No such reference named %s\n", pReplaceModel->name);
			}
		}
	}
}

//************************************
// Parses LODs
//************************************
void ParseLods(kvkeybase_t* pSection)
{
	// always add first lod
	studiolodparams_t lod;
	lod.distance = 0.0f;
	lod.flags = 0;

	// add new lod parameters
	g_lodparams.append(lod);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!pSection->keys[i]->IsSection())
			continue;

		if(!stricmp(pSection->keys[i]->name, "lod"))
		{
			float lod_dist = KV_GetValueFloat(pSection->keys[i], 0, 1.0f);

			studiolodparams_t newlod;
			newlod.distance = lod_dist;
			newlod.flags = 0;

			// add new lod parameters
			int lodIdx = g_lodparams.append(newlod);

			ParseLodData( pSection->keys[i], lodIdx );

			Msg("Added lod %d, distance: %2f\n", lodIdx, lod_dist);
		}

		/*
		char	lod_str[44];
		float	lod_distance;

		int nArgs = sscanf(pSection->keys[i]->name, "%s %f", lod_str, &lod_distance);

		if(nArgs > 1 && !stricmp(lod_str, "lod"))
		{
			ParseLodData( pSection->keys[i], lod_distance );

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
		*/
	}
}


//************************************
// Load body groups
//************************************
bool LoadBodyGroups(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "bodygroup"))
		{
			studiobodygroup_t bodygroup;

			char ref_name[44];

			if(pSection->keys[i]->values.numElem() < 2 && !pSection->keys[i]->IsSection())
			{
				MsgError("Invalid body group string format\n");
				MsgWarning("usage: bodygroup \"(name)\" \"(reference)\"\n");
				MsgWarning("	or\n");
				MsgWarning("usage: bodygroup \"(name)\" { <references> }\n");
			}

			if(pSection->keys[i]->values.numElem() > 1)
			{
				strcpy(bodygroup.name, pSection->keys[i]->values[0]);
				strcpy(ref_name, pSection->keys[i]->values[1]);

				int lodIndex = FindModelLodIdGroupByName(ref_name);

				if(lodIndex == -1)
				{
					MsgError("reference '%s' not found for bodygroup '%s'\n", ref_name, bodygroup.name);
					return false;
				}

				bodygroup.lodmodel_index = lodIndex;

				Msg("Adding body group '%s'\n", bodygroup.name);

				g_bodygroups.append(bodygroup);
			}
			else if(pSection->keys[i]->IsSection())
			{
#pragma todo("multi-model body groups")
			}
		}
	}

	if(g_bodygroups.numElem() == 0)
	{
		MsgError("Model must have at least one body group\n");
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
// Remaps vertex bone indices new_bones
//************************************
void BoneRemapDSMGroup(dsmgroup_t* pGroup, DkList<dsmskelbone_t*> &old_bones, DkList<dsmskelbone_t*> &new_bones)
{
	for(int i = 0; i < pGroup->verts.numElem(); i++)
	{
		for(int j = 0; j < pGroup->verts[i].weights.numElem(); j++)
		{
			int bone_id = pGroup->verts[i].weights[j].bone;

			// find bone in new list by bone name in old list
			int new_bone_id = BoneListGetBoneIndex(old_bones[bone_id]->name, new_bones);

			// point to bone in new list
			pGroup->verts[i].weights[j].bone = new_bone_id;
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
		for(int j = 0; j < g_modelrefs[i].model->bones.numElem(); j++)
		{
			// not found in new list? add
			if(!BoneListCheckForBone(g_modelrefs[i].model->bones[j]->name, allBones))
			{
				// copy bone
				dsmskelbone_t* pBone = new dsmskelbone_t;
				memcpy(pBone, g_modelrefs[i].model->bones[j], sizeof(dsmskelbone_t));

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
		BoneMergeRemapDSM( g_modelrefs[i].model, allBones );
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

	for(int i = 0; i < g_modelrefs[0].model->bones.numElem(); i++)
	{
		cbone_t cbone;
		cbone.referencebone = g_modelrefs[0].model->bones[i];

		g_bones.append(cbone);

		Msg(" %s\n", cbone.referencebone->name);
	}

	Msg("  total bones: %d\n", g_bones.numElem());

	// set parents
	for(int i = 0; i < g_bones.numElem(); i++)
	{
		int parent_id = g_modelrefs[0].model->bones[i]->parent_id;

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
bool LoadMaterialPaths(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "materialpath"))
		{
			materialpathdesc_t desc;

			int sp_len = strlen(pSection->keys[i]->values[0])-1;

			if(pSection->keys[i]->values[0][sp_len] != '/' || pSection->keys[i]->values[0][sp_len] != '\\')
			{
				strcpy(desc.m_szSearchPathString, varargs("%s/", pSection->keys[i]->values[0]));
			}
			else
			{
				strcpy(desc.m_szSearchPathString, pSection->keys[i]->values[0]);
			}
			
			g_matpathes.append(desc);
		}

		if(!stricmp(pSection->keys[i]->name, "notextures"))
		{
			g_notextures = KV_GetValueBool(pSection->keys[i]);
		}
	}

	if(g_matpathes.numElem() == 0 && g_notextures == false)
	{
		MsgError("Error! Model must have at least one materialpath!\n");
		return false;
	}

	if(g_matpathes.numElem() > 0)
		Msg("Added %d material paths\n", g_matpathes.numElem());

	return true;
}

//************************************
// Loads material pathes to use in engine
//************************************
bool LoadMotionPackagePatchs(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "addmotionpackage"))
		{
			motionpackagedesc_t desc;

			strcpy(desc.packagename, pSection->keys[i]->values[0]);
			
			g_motionpacks.append(desc);
		}
	}

	if(g_motionpacks.numElem() > 0)
		Msg("Added %d external motion packages\n", g_motionpacks.numElem());

	return true;
}

//************************************
// Parses ik chain info from section
//************************************
void ParseIKChain(kvkeybase_t* pSection)
{
	ikchain_t ikCh;

	char effector_name[44];

	kvkeybase_t *pair = pSection->FindKeyBase("chain");
	if(!pair)
	{
		MsgError("ikchain error: 'chain' key not found\n");
		MsgWarning("usage: chain (name) (effector bone)\n");
		return;
	}
	
	int nArg = sscanf(pair->values[0], "%s %s", ikCh.name, effector_name);

	if(nArg < 2)
	{
		MsgError("Invalid ikchain 'chain' write format\n");
		MsgWarning("usage: chain (name) (effector bone)\n");
	}

	cbone_t* effector_chain = FindBoneByName(effector_name);

	if(!effector_chain)
	{
		MsgError("ikchain: effector bone %s not found\n", effector_name);
		return;
	}

	// fill link list
	cbone_t* cparent = effector_chain;
	do
	{
		ciklink_t link;

		link.damping = 1.0f;

		link.bone = cparent;

		link.mins = Vector3D(-360);
		link.maxs = Vector3D(360);

		// add new link
		ikCh.link_list.append(link);

		// advance parent
		cparent = cparent->parent;
	}
	while(cparent != NULL/* && cparent->parent != NULL*/);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "damping"))
		{
			char link_name[44];
			float fDamp = 1.0f;

			int args = sscanf(pSection->keys[i]->values[0], "%s %f\n",link_name, &fDamp);
			if(args < 2)
			{
				MsgError("Too few arguments for ik parameter 'damping'\n");
				MsgWarning("usage: damping (bone name) (damping)\n");
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
		else if(!stricmp(pSection->keys[i]->name, "link_limits"))
		{
			char link_name[44];
			Vector3D mins;
			Vector3D maxs;

			int args = sscanf(pSection->keys[i]->values[0], "%s %f %f %f %f %f %f\n",link_name, &mins.x,&mins.y,&mins.z, &maxs.x,&maxs.y,&maxs.z);
			if(args < 7)
			{
				MsgError("Too few arguments for ik parameter 'link_limits'\n");
				MsgWarning("usage: link_limits (bone name) (min limits vector3) (max limits vector3)\n");
			}

			bool bFound = false;

			// search for link and apply parameter if found
			for(int j = 0; j < ikCh.link_list.numElem(); j++)
			{
				if(!stricmp(ikCh.link_list[j].bone->referencebone->name, link_name))
				{
					ikCh.link_list[j].mins = mins;
					ikCh.link_list[j].maxs = maxs;

					bFound = true;

					break;
				}
			}

			if(!bFound)
				MsgError("link_limits: bone '%s' not found\n", link_name);
		}
	}

	g_ikchains.append(ikCh);
};

//************************************
// Loads ik chains if available
//************************************
void LoadIKChains(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(pSection->keys[i]->IsSection() && !stricmp(pSection->keys[i]->name, "ikchain"))
		{
			ParseIKChain(pSection->keys[i]);
		}
	}

	if(g_ikchains.numElem() > 0)
		Msg("Parsed %d IK chains\n", g_ikchains.numElem());
}

//************************************
// Loads attachments if available
//************************************
void LoadAttachments(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if(!stricmp(pSection->keys[i]->name, "attachment"))
		{
			studioattachment_t attach;

			char attach_to_bone[44];

			// parse attachment
			int sCnt = sscanf(pSection->keys[i]->values[0], "%s %s %f %f %f %f %f %f", attach.name, attach_to_bone,
				&attach.position.x,&attach.position.y,&attach.position.z,
				&attach.angles.x,&attach.angles.y,&attach.angles.z);

			if(sCnt < 8)
			{
				MsgError("Invalid attachment definition\n");
				MsgWarning("usage: attachment \"(name) (bone name) (position x y z) (rotation x y z)\"");
				continue;
			}

			cbone_t* pBone = FindBoneByName(attach_to_bone);

			if(!pBone)
			{
				MsgError("Can't find bone %s for attachment %s\n", attach_to_bone, attach.name);
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

//************************************
// main function of script compilation
//************************************
void MakePhysModel(kvkeybase_t* pSection, char* egf_out_name)
{
	EqString podFileName(egf_out_name);
	podFileName = podFileName.Path_Strip_Ext() + _Es(".pod");

	kvkeybase_t* pPhysDataSection = pSection->FindKeyBase("physics", KV_FLAG_SECTION);

	if(!pPhysDataSection)
		return;

	MsgWarning("\nGenerating POD physics model\n");

	kvkeybase_t* pPair = pPhysDataSection->FindKeyBase("model");

	dsmmodel_t *physicsmodel = NULL;
	bool isLoadedModel = false;

	if(pPair)
	{
		clodmodel_t* lodMod = FindModelLodGroupByName( pPair->values[0] );

		if(!lodMod)
		{
			char ref_path[256];
			strcpy(ref_path, g_refspath);
			strcat(ref_path, "/");
			strcat(ref_path, pPair->values[0]);

			physicsmodel = new dsmmodel_t;

			if(!LoadSharedModel(physicsmodel, ref_path))
			{
				MsgError("*ERROR* Cannot find model reference '%s'\n", ref_path);
				return;
			}

			isLoadedModel = true;
		}
		else
		{
			physicsmodel = lodMod->lodmodels[0];

			if(!physicsmodel)
				MsgError("*ERROR* empty lod reference '%s'\n", pPair->values[0]);
		}
	}
	else
	{
		physicsmodel = g_modelrefs[0].model;
	}

	// scale bones
	for(int i = 0; i < physicsmodel->bones.numElem(); i++)
	{
		physicsmodel->bones[i]->position *= g_scalemodel;
	}

	// check material list
	for(int i = 0; i < physicsmodel->groups.numElem(); i++)
	{
		// scale vertices
		for(int j = 0; j < physicsmodel->groups[i]->verts.numElem(); j++)
		{
			physicsmodel->groups[i]->verts[j].position *= g_scalemodel;
		}
	}

	physgenmakeparams_t params;
	params.pModel = physicsmodel;
	params.phys_section = pPhysDataSection;
	params.forcegroupdivision = false;

	params.outputname[0] = 0;
	strcpy(params.outputname, podFileName.GetData());

	// process loaded model
	GeneratePhysicsModel(&params);

	if(isLoadedModel)
		delete physicsmodel;
}

extern bool WriteEGF(const char* filename);

void Cleanup()
{
#ifdef EDITOR
	for(int i = 0; i < g_modelrefs.numElem(); i++)
	{
		if(g_modelrefs[i])
		{
			FreeDSM(g_modelrefs[i]);
			delete g_modelrefs[i];
		}
	}
#endif

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
bool CompileESCScript(const char* filename)
{
	// strip filename to set reference models path
	strcpy(g_refspath, filename);
	stripFileName(g_refspath);

	KeyValues g_script;

	MsgWarning("\nCompiling script \"%s\"\n", filename);

	if(g_script.LoadFromFile(filename))
	{
		// load all script data
		kvkeybase_t* mainsection = g_script.GetRootSection();
		if(mainsection)
		{
			kvkeybase_t* pSourcePath = mainsection->FindKeyBase("source_path");

			// set source path if defined by script
			if(pSourcePath)
			{
				char pathSep[2] = {CORRECT_PATH_SEPARATOR, '\0'};
				strcat(g_refspath, pathSep);
				strcat(g_refspath, KV_GetValueString(pSourcePath, 0, ""));
			}

			// get new model filename
			kvkeybase_t* pPair = mainsection->FindKeyBase("modelfilename");
			if(!pPair)
			{
				MsgError("No output model file name specified in the script (modelfilename)\n");
				return false;
			}

			MsgWarning("\nLoading models\n");

			// try load models
			if( !LoadModels( mainsection ) )
				return false;

			MsgWarning("\nLoading LODs\n");

			// load lod/replacement data
			ParseLods( mainsection );

			// parse body groups
			if( !LoadBodyGroups( mainsection ) )
				return false;

			MsgWarning("\nMerging bones\n");

			// merge bones first
			MergeBones();

			// build bone chain after loading models
			BuildBoneChains();

			MsgWarning("\nLoading material pathes\n");

			// load material paths - needed for material system.
			if(!LoadMaterialPaths( mainsection ))
				return false;

			// external motion packages
			LoadMotionPackagePatchs( mainsection );

			MsgWarning("\nLoading attachments\n");

			// load attachments if available
			LoadAttachments( mainsection );

			MsgWarning("\nLoading IK chains\n");

			// load ik chains if available
			LoadIKChains( mainsection );

			// TODO: Optimize model here

			// build egf model
			// and save it to file
			if(!WriteEGF( pPair->values[0] ))
				return false;

			//Sleep(5000);

			//make physics model using DSM
			MakePhysModel( mainsection,  pPair->values[0]);

			// clean all after compilation
			Cleanup();
		}
		else
		{
			MsgError("No 'CompileParams' section found in script.\n");
			return false;
		}
	}
	else
	{
		MsgError("Can't open %s\n", filename);
		return false;
	}

	return true;
}