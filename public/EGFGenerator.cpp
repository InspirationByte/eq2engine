//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "EGFGenerator.h"

#include "dsm_esm_loader.h"

//------------------------------------------------------------

CEGFGenerator::CEGFGenerator() 
	: m_modelScale(1.0f), m_modelOffset(0.0f), m_notextures(false), m_physModels()
{

}

CEGFGenerator::~CEGFGenerator()
{
	Cleanup();
}

//************************************
// Finds bone
//************************************
cbone_t* CEGFGenerator::FindBoneByName(const char* pszName)
{
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		if(!stricmp(m_bones[i].referencebone->name, pszName))
			return &m_bones[i];
	}
	return nullptr;
}

//************************************
// Finds lod model
//************************************
clodmodel_t* CEGFGenerator::FindModelLodGroupByName(const char* pszName)
{
	for(int i = 0; i < m_modellodrefs.numElem(); i++)
	{
		if(!stricmp(m_modellodrefs[i].lodmodels[0]->name, pszName))
			return &m_modellodrefs[i];
	}
	return nullptr;
}

//************************************
// Finds egfCa model
//************************************
egfcamodel_t* CEGFGenerator::FindModelByName(const char* pszName)
{
	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		if(!stricmp(m_modelrefs[i].model->name, pszName))
			return &m_modelrefs[i];
	}
	return nullptr;
}

//************************************
// Finds lod model, returns index
//************************************
int CEGFGenerator::FindModelLodIdGroupByName(const char* pszName)
{
	for(int i = 0; i < m_modellodrefs.numElem(); i++)
	{
		if(!stricmp(m_modellodrefs[i].lodmodels[0]->name, pszName))
			return i;
	}
	return -1;
}

//************************************
// Returns material index
//************************************
int CEGFGenerator::GetMaterialIndex(const char* pszName)
{
	for(int i = 0; i < m_materials.numElem(); i++)
	{
		if(!stricmp(m_materials[i].materialname, pszName))
			return i;
	}

	return -1;
}

//************************************
// Returns reference index
//************************************
int CEGFGenerator::GetReferenceIndex(dsmmodel_t* pRef)
{
	if(pRef == nullptr)
		return -1;

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		if(m_modelrefs[i].model == pRef)
			return i;
	}

	return -1;
}

//************************************
// Loads a model
//************************************
egfcamodel_t CEGFGenerator::LoadModel(const char* pszFileName)
{
	egfcamodel_t mod;

	mod.model = new dsmmodel_t;

	EqString modelPath( CombinePath(3, m_refsPath.c_str(), CORRECT_PATH_SEPARATOR_STR, pszFileName) );

	EqString ext(modelPath.Path_Extract_Ext());

	// load shape keys
	if( !ext.CompareCaseIns("esx") )
	{
		mod.shapeData = new esmshapedata_t;

		Msg("Loading shape file '%s'\n", modelPath.c_str());

		if( LoadESXShapes( mod.shapeData, modelPath.c_str() ))
		{
			MsgAccept("esx shapes loaded!\n");

			// use referenced filename by the shape file
			modelPath = CombinePath(3, m_refsPath.c_str(), CORRECT_PATH_SEPARATOR_STR, mod.shapeData->reference.c_str());
		}
		else
		{
			delete mod.shapeData;
			mod.shapeData = nullptr;
		}
	}

	// load DSM model
	if(!LoadSharedModel(mod.model, modelPath.c_str()))
	{
		FreeModel(mod);
		return mod;
	}

	int nVerts = GetTotalVertsOfDSM( mod.model );

	if((float)nVerts/3.0f != nVerts/3)
	{
		MsgError("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", modelPath.c_str());
		
		FreeModel(mod);
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
		dsmskelbone_t* bone = mod.model->bones[i];

		bone->position *= m_modelScale;

		if(bone->parent_id == -1)
			bone->position += m_modelOffset;
	}

	// check material list and move/scale verts
	for(int i = 0; i < mod.model->groups.numElem(); i++)
	{
		dsmgroup_t* group = mod.model->groups[i];

		// scale vertices
		for(int j = 0; j < group->verts.numElem(); j++)
		{
			group->verts[j].position *= m_modelScale;
			group->verts[j].position += m_modelOffset;
		}

		// if material is not found, add new one
		int found = GetMaterialIndex(mod.model->groups[i]->texture);
		if(found == -1)
		{
			if(m_materials.numElem()-1 == MAX_STUDIOMATERIALS)
			{
				MsgError("Exceeded used material count (MAX_STUDIOMATERIALS = %d)!", MAX_STUDIOMATERIALS);
				break;
			}

			// create new material
			studiomaterialdesc_t desc;
			strcpy(desc.materialname, mod.model->groups[i]->texture);
			m_materials.append(desc);
		}
	}

	return mod;
}

//************************************
// Frees model ref
//************************************
void CEGFGenerator::FreeModel( egfcamodel_t& mod )
{
	FreeDSM(mod.model);
	delete mod.model;

	if(mod.shapeData)
		FreeShapes(mod.shapeData);

	mod.model = nullptr;
	mod.shapeData = nullptr;
}

//************************************
// Loads reference
//************************************
dsmmodel_t* CEGFGenerator::ParseAndLoadModels(kvkeybase_t* pKeyBase)
{
	DkList<EqString> modelfilenames;
	DkList<EqString> shapeByModels;

	if(pKeyBase->values.numElem() > 1)
	{
		// DRVSYN: vertex order for damaged model
		if(pKeyBase->values.numElem() > 3 && !stricmp(KV_GetValueString(pKeyBase, 2), "shapeby"))
		{
			const char* shapekeyName = KV_GetValueString(pKeyBase, 3, "");

			if(shapekeyName)
				Msg("Model shape key is '%s'\n", shapekeyName );

			shapeByModels.append(shapekeyName);
		}
		else
			shapeByModels.append("");

		modelfilenames.append( KV_GetValueString(pKeyBase, 1) );
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

			FreeModel(model);

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
	}

	// merge the models
	if(models.numElem() > 1)
	{
		dsmmodel_t* merged = new dsmmodel_t;

		// set model to as part name
		strcpy(merged->name, KV_GetValueString(pKeyBase, 0, "invalid_model_name"));

		for(int i = 0; i < models.numElem(); i++)
		{
			dsmmodel_t* model = models[i].model;

			if(i == 0)
			{
				merged->bones.append(model->bones);
				model->bones.clear();
			}

			merged->groups.append(model->groups);
			model->groups.clear();

			FreeModel(models[i]);
		}

		egfcamodel_t mref;
		mref.model = merged;
		mref.shapeData = nullptr;

		// add model to list
		m_modelrefs.append( mref );

		return merged;
	}
	else if(models.numElem() > 0)
	{
		// add model to list
		m_modelrefs.append( models[0] );

		return models[0].model;
	}
	else
	{
		MsgError("got model definition, but nothing added\n");
	}

	return nullptr;
}

//************************************
// Loads reference models
//************************************
bool CEGFGenerator::LoadModels(kvkeybase_t* pSection)
{
	MsgWarning("\nLoading models\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* keyBase = pSection->keys[i];

		if(!stricmp(keyBase->name, "global_scale"))
		{
			// try apply global scale
			m_modelScale = KV_GetVector3D(keyBase, 0, Vector3D(1.0f));
		}
		if(!stricmp(keyBase->name, "global_offset"))
		{
			// try apply global offset
			m_modelOffset = KV_GetVector3D(keyBase, 0, Vector3D(0.0f));
		}
		else if(!stricmp(keyBase->name, "model"))
		{
			// parse and load model
			dsmmodel_t* model = ParseAndLoadModels( keyBase );

			if(!model)
				return false;
			else
			{
				clodmodel_t lod_model;

				memset(lod_model.lodmodels, 0, sizeof(lod_model.lodmodels));

				// set model to lod 0
				lod_model.lodmodels[0] = model;

				// add a LOD model replacement table
				m_modellodrefs.append(lod_model);
			}
		}
	}

	if(m_modelrefs.numElem() == 0)
	{
		MsgError("Error! Model must have at least one reference!\n");
		MsgWarning("usage: model <part name> <*.esm or *.obj path>\n");
		return false;
	}

	Msg("Added %d model references\n", m_modelrefs.numElem());

	return true;
}

//************************************
// Parses LOD data
//************************************
void CEGFGenerator::ParseLodData(kvkeybase_t* pSection, int lodIdx)
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
void CEGFGenerator::LoadLods(kvkeybase_t* pSection)
{
	MsgWarning("\nLoading LODs\n");

	// always add first lod
	studiolodparams_t lod;
	lod.distance = 0.0f;
	lod.flags = 0;

	// add new lod parameters
	m_lodparams.append(lod);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* lodKey = pSection->keys[i];

		if(!lodKey->IsSection())
			continue;

		if(!stricmp(lodKey->name, "lod"))
		{
			float lod_dist = KV_GetValueFloat(lodKey, 0, 1.0f);

			studiolodparams_t newlod;
			newlod.distance = lod_dist;
			newlod.flags = 0;

			if(m_lodparams.numElem()-1 == MAX_MODELLODS)
			{
				MsgError("Exceeded lod count (MAX_MODELLODS = %d)!", MAX_MODELLODS);
				return;
			}

			// add new lod parameters
			int lodIdx = m_lodparams.append(newlod);

			ParseLodData( lodKey, lodIdx );

			Msg("Added lod %d, distance: %2f\n", lodIdx, lod_dist);
		}
	}
}


//************************************
// Load body groups
//************************************
bool CEGFGenerator::LoadBodyGroups(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* keyBase = pSection->keys[i];

		if(!stricmp(keyBase->name, "bodygroup"))
		{
			studiobodygroup_t bodygroup;

			char ref_name[44];

			if(keyBase->values.numElem() < 2 && !keyBase->IsSection())
			{
				MsgError("Invalid body group string format\n");
				MsgWarning("usage: bodygroup \"(name)\" \"(reference)\"\n");
				MsgWarning("	or\n");
				MsgWarning("usage: bodygroup \"(name)\" { <references> }\n");
			}

			if(keyBase->values.numElem() > 1)
			{
				strcpy(bodygroup.name, KV_GetValueString(keyBase, 0));
				strcpy(ref_name, KV_GetValueString(keyBase, 1));

				int lodIndex = FindModelLodIdGroupByName(ref_name);

				if(lodIndex == -1)
				{
					MsgError("reference '%s' not found for bodygroup '%s'\n", ref_name, bodygroup.name);
					return false;
				}

				bodygroup.lodmodel_index = lodIndex;

				Msg("Adding body group '%s'\n", bodygroup.name);

				m_bodygroups.append(bodygroup);
			}
			else if(keyBase->IsSection())
			{
#pragma todo("multi-model body groups")
			}
		}
	}

	if(m_bodygroups.numElem() == 0)
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
void CEGFGenerator::MergeBones()
{
	if(m_bones.numElem() == 0)
		return;

	MsgWarning("\nMerging bones\n");

	// first, load all bones into the single list, as unique
	DkList<dsmskelbone_t*> allBones;

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		dsmmodel_t* model = m_modelrefs[i].model;

		for(int j = 0; j < model->bones.numElem(); j++)
		{
			// not found in new list? add
			if(!BoneListCheckForBone(model->bones[j]->name, allBones))
			{
				// copy bone
				dsmskelbone_t* pBone = new dsmskelbone_t;
				memcpy(pBone, model->bones[j], sizeof(dsmskelbone_t));

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
	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		BoneMergeRemapDSM( m_modelrefs[i].model, allBones );
	}
	
	for(int i = 0; i < allBones.numElem(); i++)
		delete allBones[i];

	allBones.clear();
}

//************************************
// Builds bone chains
//************************************
void CEGFGenerator::BuildBoneChains()
{
	Msg("Bones:\n");

	// using a first reference since it's already remapped
	dsmmodel_t* model = m_modelrefs[0].model;

	// make bone list first
	for(int i = 0; i < model->bones.numElem(); i++)
	{
		cbone_t cbone;
		cbone.referencebone = model->bones[i];

		m_bones.append(cbone);

		Msg(" %s\n", cbone.referencebone->name);
	}

	Msg("  total bones: %d\n", m_bones.numElem());

	// set parents
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		int parent_id = model->bones[i]->parent_id;

		if(parent_id == -1)
			m_bones[i].parent = nullptr;
		else
			m_bones[i].parent = &m_bones[parent_id];
	}

	// build child lists
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		if(m_bones[i].parent)
			m_bones[i].parent->childs.append(&m_bones[i]);
	}
}

//************************************
// Loads material pathes to use in engine
//************************************
bool CEGFGenerator::LoadMaterialPaths(kvkeybase_t* pSection)
{
	MsgWarning("\nLoading material paths\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* keyBase = pSection->keys[i];

		if(!stricmp(keyBase->name, "materialpath"))
		{
			materialpathdesc_t desc;

			EqString path = KV_GetValueString(keyBase);

			int sp_len = path.Length()-1;

			if(path.c_str()[sp_len] != '/' || path.c_str()[sp_len] != '\\')
			{
				strcpy(desc.m_szSearchPathString, varargs("%s/", path.c_str()));
			}
			else
			{
				strcpy(desc.m_szSearchPathString, path.c_str());
			}
			
			m_matpathes.append(desc);
		}

		if(	!stricmp(keyBase->name, "notextures") || 
			!stricmp(keyBase->name, "nomaterials"))
		{
			m_notextures = KV_GetValueBool(keyBase);
		}
	}

	if(m_matpathes.numElem() == 0 && m_notextures == false)
	{
		MsgError("Error! Model must have at least one materialpath!\n");
		return false;
	}

	if(m_matpathes.numElem() > 0)
		Msg("Added %d material paths\n", m_matpathes.numElem());

	return true;
}

//************************************
// Loads material pathes to use in engine
//************************************
bool CEGFGenerator::LoadMotionPackagePatchs(kvkeybase_t* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* keyBase = pSection->keys[i];

		if(!stricmp(keyBase->name, "addmotionpackage"))
		{
			if(m_lodparams.numElem()-1 == MAX_MOTIONPACKAGES)
			{
				MsgError("Exceeded motion packages count (MAX_MOTIONPACKAGES = %d)!", MAX_MOTIONPACKAGES);
				return false;
			}

			motionpackagedesc_t desc;

			strcpy(desc.packagename, KV_GetValueString(keyBase));

			m_motionpacks.append(desc);
		}
	}

	if(m_motionpacks.numElem() > 0)
		Msg("Added %d external motion packages\n", m_motionpacks.numElem());

	return true;
}

//************************************
// Parses ik chain info from section
//************************************
void CEGFGenerator::ParseIKChain(kvkeybase_t* pSection)
{
	if(pSection->values.numElem() < 2)
	{
		MsgError("Too few arguments for 'ikchain'\n");
		MsgWarning("usage: ikchain (bone name) (effector bone name)\n");
		return;
	}

	ikchain_t ikCh;

	char effector_name[44];
	
	strcpy(ikCh.name, KV_GetValueString(pSection, 0));
	strcpy(effector_name, KV_GetValueString(pSection, 1));

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
	while(cparent != nullptr/* && cparent->parent != nullptr*/);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* sec = pSection->keys[i];

		if(!stricmp(sec->name, "damping"))
		{
			if(sec->values.numElem() < 2)
			{
				MsgError("Too few arguments for ik parameter 'damping'\n");
				MsgWarning("usage: damping (bone name) (damping)\n");
				continue;
			}

			char link_name[44];
			float fDamp = 1.0f;

			strcpy(link_name, KV_GetValueString(sec, 0));
			fDamp = KV_GetValueFloat(sec, 1);

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
		else if(!stricmp(sec->name, "link_limits"))
		{
			if(sec->values.numElem() < 7)
			{
				MsgError("Too few arguments for ik parameter 'link_limits'\n");
				MsgWarning("usage: link_limits (bone name) (MinX MinY MinZ) (MaxX MaxY MaxZ)\n");
			}

			char link_name[44];
			Vector3D mins;
			Vector3D maxs;

			strcpy(link_name, KV_GetValueString(sec, 0));
			mins = KV_GetVector3D(sec, 1);
			maxs = KV_GetVector3D(sec, 4);

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

	m_ikchains.append(ikCh);
};

//************************************
// Loads ik chains if available
//************************************
void CEGFGenerator::LoadIKChains(kvkeybase_t* pSection)
{
	MsgWarning("\nLoading IK chains\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* chainSec = pSection->keys[i];

		if(chainSec->IsSection() && !stricmp(chainSec->name, "ikchain"))
		{
			ParseIKChain(chainSec);
		}
	}

	if(m_ikchains.numElem() > 0)
		Msg("Parsed %d IK chains\n", m_ikchains.numElem());
}

//************************************
// Loads attachments if available
//************************************
void CEGFGenerator::LoadAttachments(kvkeybase_t* pSection)
{
	MsgWarning("\nLoading attachments\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		kvkeybase_t* attachSec = pSection->keys[i];

		if(!stricmp(attachSec->name, "attachment"))
		{
			if(attachSec->values.numElem() < 8)
			{
				MsgError("Invalid attachment definition\n");
				MsgWarning("usage: attachment (name) (bone name) (position x y z) (rotation x y z)\n");
				continue;
			}

			studioattachment_t attach;

			char attach_to_bone[44];

			strcpy(attach.name, KV_GetValueString(attachSec, 0));
			strcpy(attach_to_bone, KV_GetValueString(attachSec, 1));

			attach.position = KV_GetVector3D(attachSec, 2);
			attach.angles = KV_GetVector3D(attachSec, 5);

			cbone_t* pBone = FindBoneByName(attach_to_bone);

			if(!pBone)
			{
				MsgError("Can't find bone %s for attachment %s\n", attach_to_bone, attach.name);
				continue;
			}

			attach.bone_id = pBone->referencebone->bone_id;

			m_attachments.append(attach);

			MsgInfo("Adding attachment %s\n", attach.name);
		}
	}

	if(m_attachments.numElem())
		Msg("Total attachments: %d\n", m_attachments.numElem());
}

//************************************
// main function of script compilation
//************************************
bool CEGFGenerator::GeneratePOD()
{
	if(!m_physModels.HasObjects())
		return true;

	MsgWarning("\nWriting physics objects...\n");

	EqString outputPOD(m_outputFilename.Path_Strip_Ext() + _Es(".pod"));
	m_physModels.SaveToFile( outputPOD.c_str() );

	return true;
}

void CEGFGenerator::Cleanup()
{
	m_attachments.clear();
	m_bodygroups.clear();
	m_ikchains.clear();
	m_lodparams.clear();

	m_materials.clear();
	m_matpathes.clear();
	m_motionpacks.clear();

	m_bones.clear();
	
	for(int i = 0; i < m_modelrefs.numElem(); i++)
		FreeModel(m_modelrefs[i]);

	m_modelrefs.clear();
	m_modellodrefs.clear();
}

void CEGFGenerator::SetRefsPath(const char* path)
{
	m_refsPath = path;
}

void CEGFGenerator::SetOutputFilename(const char* filename)
{
	m_outputFilename = filename;
}

bool CEGFGenerator::InitFromKeyValues(const char* filename)
{
	KeyValues scriptFile;

	if(scriptFile.LoadFromFile(filename))
	{
		SetRefsPath( _Es(filename).Path_Strip_Name().c_str() );

		// strip filename to set reference models path
		MsgWarning("\nCompiling script \"%s\"\n", filename);

		// load all script data
		return InitFromKeyValues(scriptFile.GetRootSection());
	}
	else
	{
		MsgError("Can't open %s\n", filename);
		return false;
	}
}

void CEGFGenerator::LoadPhysModels(kvkeybase_t* mainsection)
{
	for(int i = 0; i < mainsection->keys.numElem(); i++)
	{
		kvkeybase_t* physObjectSec = mainsection->keys[i];

		if(stricmp(physObjectSec->name, "physics"))
			continue;

		if(!physObjectSec->IsSection())
		{
			MsgError("*ERROR* key 'physics' must be a section\n");
			continue;
		}

		dsmmodel_t* physModel = nullptr;

		kvkeybase_t* modelNamePair = physObjectSec->FindKeyBase("model");
		if(modelNamePair)
		{
			egfcamodel_t* foundRef = FindModelByName( KV_GetValueString(modelNamePair) );

			if(!foundRef)
			{
				// scaling already performed here
				egfcamodel_t newRef = LoadModel( KV_GetValueString(modelNamePair) );

				if(newRef.model == nullptr)
				{
					MsgError("*ERROR* Cannot find model reference '%s'\n", KV_GetValueString(modelNamePair));
					continue;
				}

				m_modelrefs.append(newRef);
				physModel = newRef.model;
			}
			else
			{
				//physicsmodel = lodMod->lodmodels[0];

				physModel = foundRef->model;

				if(!physModel)
				{
					MsgError("*ERROR* no such model '%s' for physics\n", KV_GetValueString(modelNamePair));
					continue;
				}
			}
		}
		else
		{
			MsgError("*ERROR* no model for physics object '%s' defined\n", KV_GetValueString(physObjectSec, 0, "unnamed"));
			continue;
		}

		MsgWarning("\nAdding physics object %s...\n", KV_GetValueString(physObjectSec, 0, ""));

		// append object
		m_physModels.GenerateGeometry(physModel, physObjectSec, false);
	}
}

//************************************
// main function of script compilation
//************************************
bool CEGFGenerator::InitFromKeyValues(kvkeybase_t* mainsection)
{
	kvkeybase_t* pSourcePath = mainsection->FindKeyBase("source_path");

	// set source path if defined by script
	if(pSourcePath)
	{
		m_refsPath = CombinePath(3, m_refsPath.c_str(), CORRECT_PATH_SEPARATOR_STR, KV_GetValueString(pSourcePath, 0, ""));
	}

	// get new model filename
	kvkeybase_t* outputModelFilenameKey = mainsection->FindKeyBase("modelfilename");
	if(outputModelFilenameKey)
		SetOutputFilename( KV_GetValueString(outputModelFilenameKey) );

	if(m_outputFilename.Length() == 0)
	{
		MsgError("No output model file name specified in the script ('modelfilename')\n");
		return false;
	}

	// try load models
	if( !LoadModels( mainsection ) )
		return false;

	// load lod/replacement data
	LoadLods( mainsection );

	// parse body groups
	if( !LoadBodyGroups( mainsection ) )
		return false;

	// merge bones first
	MergeBones();

	// build bone chain after loading models
	BuildBoneChains();

	// load material paths - needed for material system.
	if(!LoadMaterialPaths( mainsection ))
		return false;

	// external motion packages
	LoadMotionPackagePatchs( mainsection );

	// load attachments if available
	LoadAttachments( mainsection );

	// load ik chains if available
	LoadIKChains( mainsection );

	// load and pre-generate physics models as final operation
	LoadPhysModels( mainsection );

	return true;
}