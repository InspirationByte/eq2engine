//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

// TODO:	There is a lot of shit coding happening since 2011 with model refs etc
//			and it needs to be sanitized ASAP.

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "EGFGenerator.h"

#include "dsm_esm_loader.h"
#include "dsm_loader.h"
#include "egf/dsm_fbx_loader.h"

using namespace SharedModel;

//------------------------------------------------------------

CEGFGenerator::CEGFGenerator()
{

}

CEGFGenerator::~CEGFGenerator()
{
	Cleanup();
}

//************************************
// Finds bone
//************************************
CEGFGenerator::GenBone_t* CEGFGenerator::FindBoneByName(const char* pszName)
{
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		if(!stricmp(m_bones[i].refBone->name, pszName))
			return &m_bones[i];
	}
	return nullptr;
}

//************************************
// Finds lod model
//************************************
CEGFGenerator::GenLODList_t* CEGFGenerator::FindModelLodGroupByName(const char* pszName)
{
	for(int i = 0; i < m_modelLodLists.numElem(); i++)
	{
		if(!m_modelLodLists[i].name.CompareCaseIns(pszName))
			return &m_modelLodLists[i];
	}
	return nullptr;
}

//************************************
// Finds egfCa model
//************************************
int CEGFGenerator::FindModelIndexByName(const char* pszName)
{
	for (int i = 0; i < m_modelrefs.numElem(); i++)
	{
		if (!m_modelrefs[i].name.CompareCaseIns(pszName))
			return i;
	}
	return -1;
}


CEGFGenerator::GenModel_t* CEGFGenerator::FindModelByName(const char* pszName)
{
	const int foundIdx = FindModelIndexByName(pszName);
	if (foundIdx == -1)
		return nullptr;

	return &m_modelrefs[foundIdx];
}

//************************************
// Finds lod model, returns index
//************************************
int CEGFGenerator::FindModelLodIdGroupByName(const char* pszName)
{
	for(int i = 0; i < m_modelLodLists.numElem(); i++)
	{
		if(!m_modelLodLists[i].name.CompareCaseIns(pszName))
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

void CEGFGenerator::AddModelLodUsageReference(int lodModelIndex)
{
	GenLODList_t& lod = m_modelLodLists[lodModelIndex];

	for (int i = 0; i < lod.lodmodels.numElem(); i++)
	{
		const int modelIdx = lod.lodmodels[i];
		if(modelIdx >= 0) // skip empty
			++m_modelrefs[modelIdx].used;
	}
}

//************************************
// Loads a model
//************************************
int CEGFGenerator::LoadModel(const char* pszFileName)
{
	if (!stricmp(pszFileName, "_dummy"))
		return -1;

	GenModel_t mod;
	mod.model = CRefPtr_new(dsmmodel_t);

	EqString modelPath;
	CombinePath(modelPath, 2, m_refsPath.ToCString(), pszFileName);

	EqString ext(modelPath.Path_Extract_Ext());

	if (!ext.CompareCaseIns("fbx"))
	{
		mod.shapeData = CRefPtr_new(esmshapedata_t);

		Msg("Loading FBX from '%s'\n", modelPath.ToCString());

		if (!LoadFBXShapes(mod.model, mod.shapeData, modelPath.ToCString()))
		{
			delete mod.shapeData;
			mod.shapeData = nullptr;

			MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
			FreeModel(mod);
			return -1;
		}
	}
	else if( !ext.CompareCaseIns("esx") ) // Legacy now because I want to drop the Blender ESM/ESX plugin support
	{
		mod.shapeData = CRefPtr_new(esmshapedata_t);

		Msg("Loading shape file '%s'\n", modelPath.ToCString());

		if( LoadESXShapes( mod.shapeData, modelPath.ToCString() ))
		{
			// use referenced filename by the shape file
			CombinePath(modelPath, 2, m_refsPath.ToCString(), mod.shapeData->reference.ToCString());
		}
		else
		{
			delete mod.shapeData;
			mod.shapeData = nullptr;
		}

		if (!LoadSharedModel(mod.model, modelPath.ToCString()))
		{
			MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
			FreeModel(mod);
			return -1;
		}
	}
	else if(!LoadSharedModel(mod.model, modelPath.ToCString()))
	{
		MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
		FreeModel(mod);
		return -1;
	}

	if (!PostProcessDSM(mod))
	{
		FreeModel(mod);
		return -1;
	}

	return m_modelrefs.append(mod);
}

bool CEGFGenerator::PostProcessDSM(GenModel_t& mod)
{
	const int nVerts = GetTotalVertsOfDSM(mod.model);

	if ((float)nVerts / 3.0f != nVerts / 3)
	{
		MsgError("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", mod.model->name);
		return false;
	}

	// assign shape indexes
	if (mod.shapeData)
	{
		AssignShapeKeyVertexIndexes(mod.model, mod.shapeData);

		for (int i = 0; i < mod.shapeData->shapes.numElem(); ++i)
		{
			esmshapekey_t* shapeKey = mod.shapeData->shapes[i];
			for (int j = 0; j < shapeKey->verts.numElem(); ++j)
			{
				shapeKey->verts[j].position *= m_modelScale;
				shapeKey->verts[j].position += m_modelOffset;
			}
		}
	}

	// scale bones
	for (int i = 0; i < mod.model->bones.numElem(); i++)
	{
		dsmskelbone_t* bone = mod.model->bones[i];

		bone->position *= m_modelScale;

		if (bone->parent_id == -1)
			bone->position += m_modelOffset;
	}

	// check material list and move/scale verts
	for (int i = 0; i < mod.model->groups.numElem(); i++)
	{
		dsmgroup_t* group = mod.model->groups[i];

		// scale vertices
		for (int j = 0; j < group->verts.numElem(); j++)
		{
			group->verts[j].position *= m_modelScale;
			group->verts[j].position += m_modelOffset;
		}

		// if material is not found, add new one
		int found = GetMaterialIndex(mod.model->groups[i]->texture);
		if (found == -1)
		{
			if (m_materials.numElem() - 1 == MAX_STUDIOMATERIALS)
			{
				MsgError("Exceeded used material count (MAX_STUDIOMATERIALS = %d)!", MAX_STUDIOMATERIALS);
				break;
			}

			// create new material
			GenMaterialDesc_t desc;
			strcpy(desc.materialname, mod.model->groups[i]->texture);

			m_materials.append(desc);
		}
	}

	return true;
}

//************************************
// Frees model ref
//************************************
void CEGFGenerator::FreeModel(GenModel_t& mod )
{
	mod.model = nullptr;
	mod.shapeData = nullptr;
}

void CEGFGenerator::LoadModelsFromFBX(KVSection* pKeyBase)
{
	EqString modelPath;
	CombinePath(modelPath, 2, m_refsPath.ToCString(), KV_GetValueString(pKeyBase));

	Array<dsmmodel_t*> models(PP_SL);
	Array<esmshapedata_t*> shapeDatas(PP_SL);

	Msg("Using FBX Source '%s'\n", KV_GetValueString(pKeyBase));

	if (!LoadFBX(models, shapeDatas, modelPath))
		return;

	//pKeyBase->FindSection(models[i]->name)

	for (int i = 0; i < pKeyBase->keys.numElem(); ++i)
	{
		KVSection* modelSec = pKeyBase->keys[i];
		const char* modelName = modelSec->name;
		const int foundIdx = models.findIndex([modelName](dsmmodel_t* model) {
			return !stricmp(model->name, modelName);
		});

		if (foundIdx == -1)
			continue;

		const char* refName = KV_GetValueString(modelSec);

		GenModel_t mod;
		mod.model = CRefPtr(models[foundIdx]);
		mod.shapeData = CRefPtr(shapeDatas[foundIdx]);
		mod.name = refName;

		// DRVSYN: vertex order for damaged model
		if (modelSec->values.numElem() > 1 && !stricmp(KV_GetValueString(modelSec, 1), "shapeby"))
		{
			const char* shapekeyName = KV_GetValueString(modelSec, 2, nullptr);

			// set shapeby
			if (shapekeyName)
			{
				Msg("Model shape key is '%s'\n", shapekeyName);
				mod.shapeIndex = FindShapeKeyIndex(mod.shapeData, shapekeyName);
			}
		}

		if (!PostProcessDSM(mod))
			return;

		const int newModelIndex = m_modelrefs.append(mod);

		// Start a new LOD. LOD has a name of reference
		GenLODList_t& lodModel = m_modelLodLists.append();
		lodModel.lodmodels.append(newModelIndex);
		lodModel.name = refName;

		const int nVerts = GetTotalVertsOfDSM(mod.model);

		Msg("Adding reference %s '%s' with %d triangles (in %d groups), %d bones\n",
			mod.model->name,
			refName,
			nVerts / 3,
			mod.model->groups.numElem(),
			mod.model->bones.numElem());
	}
}

//************************************
// Loads reference
//************************************
int CEGFGenerator::ParseAndLoadModels(KVSection* pKeyBase)
{
	Array<EqString> modelfilenames(PP_SL);
	Array<EqString> shapeByModels(PP_SL);

	if(pKeyBase->values.numElem() > 1)
	{
		// DRVSYN: vertex order for damaged model
		if(pKeyBase->values.numElem() > 3 && !stricmp(KV_GetValueString(pKeyBase, 2), "shapeby"))
		{
			const char* shapekeyName = KV_GetValueString(pKeyBase, 3, nullptr);

			if (shapekeyName)
			{
				Msg("Model shape key is '%s'\n", shapekeyName);
				shapeByModels.append(shapekeyName);
			}
			else
				shapeByModels.append("");
		}
		else
			shapeByModels.append("");

		// add model
		modelfilenames.append( KV_GetValueString(pKeyBase, 1) );
	}

	// go through all keys inside model section
	for(int i = 0; i < pKeyBase->keys.numElem(); i++)
	{
		modelfilenames.append( pKeyBase->keys[i]->name );
		shapeByModels.append("");

		Msg("Adding model '%s'\n", pKeyBase->keys[i]->name);
	}

	// load the models
	Array<GenModel_t> models(PP_SL);

	for(int i = 0; i < modelfilenames.numElem(); i++)
	{
		Msg("Loading model '%s'\n", modelfilenames[i].ToCString());
		const int modelIdx = LoadModel( modelfilenames[i].ToCString() );

		if(modelIdx == -1)
			continue;

		GenModel_t& model = m_modelrefs[modelIdx];
		model.shapeIndex = FindShapeKeyIndex(model.shapeData, shapeByModels[i].ToCString());;

		if (model.shapeIndex != -1)
			Msg("Shape key used: %s\n", shapeByModels[i].ToCString());

		GenLODList_t& lod_model = m_modelLodLists.append();
		lod_model.lodmodels.append(modelIdx);
		lod_model.name = model.name;

		// set model to as part name
		model.name = KV_GetValueString(pKeyBase, 0, "invalid_model_name");

		const int nVerts = GetTotalVertsOfDSM(model.model);
		Msg("Adding reference %s '%s' with %d triangles (in %d groups), %d bones\n", 
				modelfilenames[i].GetData(),
				model.name.ToCString(),
				nVerts/3, 
				model.model->groups.numElem(), 
				model.model->bones.numElem());

		// add finally
		models.append( model );
	}

	// merge the models
	if(models.numElem() > 1)
	{
		dsmmodel_t* merged = PPNew dsmmodel_t;

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

		GenModel_t mref;
		mref.model = CRefPtr(merged);
		mref.shapeData = nullptr;
		mref.name = merged->name;

		return m_modelrefs.append(mref);
	}
	else if(models.numElem() > 0)
	{
		return m_modelrefs.append(models[0]);
	}

	MsgError("got model definition '%s', but nothing added\n", KV_GetValueString(pKeyBase));
	return -1;
}

//************************************
// Loads reference models
//************************************
bool CEGFGenerator::ParseModels(KVSection* pSection)
{
	MsgWarning("\nLoading models\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* keyBase = pSection->keys[i];

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
		else if (!stricmp(keyBase->name, "FBXSource"))
		{
			LoadModelsFromFBX(keyBase);
		}
		else if(!stricmp(keyBase->name, "model"))
		{
			// parse and load model
			ParseAndLoadModels( keyBase );
		}
	}

	if(m_modelrefs.numElem() == 0)
	{
		MsgError("Error! Model must have at least one reference!\n");
		MsgWarning("usage: model <part name> <*.esm|*.fbx|*.obj| path>\n");
		return false;
	}

	Msg("Added %d model references\n", m_modelrefs.numElem());

	// Add dummy (used for LODs)
	GenModel_t mod{ "_dummy", CRefPtr_new(dsmmodel_t), nullptr };
	strcpy(mod.model->name, "_dummy");
	m_modelrefs.append(mod);

	return true;
}

//************************************
// Parses LOD data
//************************************
void CEGFGenerator::ParseLodData(KVSection* pSection, int lodIdx)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		if (stricmp(pSection->keys[i]->name, "replace"))
			continue;

		const char* replaceModelName = KV_GetValueString(pSection->keys[i]);
		GenLODList_t* lodgroup = FindModelLodGroupByName(replaceModelName);
		if (!lodgroup)
		{
			MsgError("No such reference named %s\n", replaceModelName);
			continue;
		}

		int replaceByExisting = FindModelIndexByName(KV_GetValueString(pSection->keys[i], 1));
		const int replaceByModel = replaceByExisting != -1 ? replaceByExisting : ParseAndLoadModels(pSection->keys[i]);

		lodgroup->lodmodels.append(replaceByModel);
	}
}

//************************************
// Parses LODs
//************************************
void CEGFGenerator::ParseLods(KVSection* pSection)
{
	MsgWarning("\nLoading LODs\n");

	// always add first lod
	studiolodparams_t lod;
	lod.distance = 0.0f;
	lod.flags = 0;
	m_lodparams.append(lod);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* lodKey = pSection->keys[i];

		if(!lodKey->IsSection())
			continue;

		if (stricmp(lodKey->name, "lod"))
			continue;

		if (m_lodparams.numElem() + 1 >= MAX_MODEL_LODS)
		{
			MsgError("Reached max lod count (MAX_MODEL_LODS = %d)!", MAX_MODEL_LODS);
			return;
		}

		const float lodDist = KV_GetValueFloat(lodKey, 0, 1.0f);
		const int lodIdx = m_lodparams.numElem();

		studiolodparams_t& newlod = m_lodparams.append();
		newlod.distance = lodDist;
		newlod.flags = 0;

		ParseLodData(lodKey, lodIdx);

		Msg("Added lod %d, distance: %2f\n", lodIdx, lodDist);
	}
}


//************************************
// Load body groups
//************************************
bool CEGFGenerator::ParseBodyGroups(KVSection* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* keyBase = pSection->keys[i];

		if(stricmp(keyBase->name, "bodygroup"))
			continue;
		
		if(keyBase->values.numElem() < 2 && !keyBase->IsSection())
		{
			MsgError("Invalid body group string format\n");
			MsgWarning("usage: bodygroup \"(name)\" \"(reference)\"\n");
			MsgWarning("	or\n");
			MsgWarning("usage: bodygroup \"(name)\" { <references> }\n");
		}

		const char* bodyGroupName = KV_GetValueString(keyBase, 0);
		if(keyBase->values.numElem() > 1)
		{
			const char* refName = KV_GetValueString(keyBase, 1);
			const int lodIndex = FindModelLodIdGroupByName(refName);

			if (lodIndex == -1)
			{
				MsgError("reference '%s' not found for bodygroup '%s'\n", refName, bodyGroupName);
				return false;
			}

			studiobodygroup_t& bodygroup = m_bodygroups.append();
			strcpy(bodygroup.name, bodyGroupName);
			bodygroup.lodModelIndex = lodIndex;

			Msg("Adding body group '%s'\n", bodygroup.name);

			// mark the models
			AddModelLodUsageReference(lodIndex);
		}
		else if(keyBase->IsSection())
		{
			MsgError("%s error - Multi-model body groups NOT YET SUPPORTED - tell a programmer!\n", bodyGroupName);
			// TODO: multi-model body groups support
		}
	}

	if(m_bodygroups.numElem() == 0)
	{
		MsgError("Model must have at least one body group\n");
	}

	return true;
}

//************************************
// Load material groups
//************************************
bool CEGFGenerator::ParseMaterialGroups(KVSection* pSection)
{
	MsgInfo("* Default materialGroup:\n\t");
	for (int i = 0; i < m_materials.numElem(); i++)
		MsgInfo("%s ", m_materials[i].materialname);
	MsgInfo("\n");

	for (int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* keyBase = pSection->keys[i];

		if (stricmp(keyBase->name, "materialGroup"))
			continue;
		
		if (!keyBase->values.numElem())
		{
			MsgError("materialGroup: must have material names as values!\n");
			MsgError("	usage: materialGroup \"<material1>\" \"<material2>\" ... \"<materialN>\"\n");
			return false;
		}

		if (keyBase->values.numElem() != m_materials.numElem())
		{
			MsgError("materialGroup: must have same material count specified (%d)!\n", m_materials.numElem());
			MsgError("	usage: materialGroup \"<material1>\" \"<material2>\" ... \"<materialN>\"\n");
			return false;
		}

		GenMaterialGroup_t* group = PPNew GenMaterialGroup_t();
		m_matGroups.append(group);

		MsgInfo("Added materialGroup: ");

		for (int j = 0; j < keyBase->values.numElem(); j++)
		{
			// create new material
			GenMaterialDesc_t desc;
			strcpy(desc.materialname, KV_GetValueString(keyBase, j));

			MsgInfo("%s ", desc.materialname);

			group->materials.append(desc);
		}

		MsgInfo("\n");
	}

	return true;
}

//************************************
// Checks bone for availablity in list
//************************************
bool BoneListCheckForBone(const char* pszName, const Array<dsmskelbone_t*> &pBones)
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
int BoneListGetBoneIndex(const char* pszName, const Array<dsmskelbone_t*> &pBones)
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
void BoneRemapDSMGroup(dsmgroup_t* pGroup, const Array<dsmskelbone_t*> &old_bones, Array<dsmskelbone_t*> &new_bones)
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
void BoneMergeRemapDSM(dsmmodel_t* pDSM, Array<dsmskelbone_t*> &new_bones)
{
	// remap groups
	for(int i = 0; i < pDSM->groups.numElem(); i++)
	{
		BoneRemapDSMGroup(pDSM->groups[i], pDSM->bones, new_bones);
	}

	// reset skeleton
	for (int i = 0; i < pDSM->bones.numElem(); ++i)
		delete pDSM->bones[i];
	pDSM->bones.clear(true);

	// and copy it
	for(int i = 0; i < new_bones.numElem(); i++)
	{
		// copy bone
		dsmskelbone_t* pBone = PPNew dsmskelbone_t;
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

	// dissolve bones that has 0 vertex refs

	// first, load all bones into the single list, as unique
	Array<dsmskelbone_t*> allBones(PP_SL);

	for(int i = 0; i < m_modelrefs.numElem(); i++)
	{
		dsmmodel_t* model = m_modelrefs[i].model;

		for(int j = 0; j < model->bones.numElem(); j++)
		{
			// not found in new list? add
			if(!BoneListCheckForBone(model->bones[j]->name, allBones))
			{
				// copy bone
				dsmskelbone_t* pBone = PPNew dsmskelbone_t;
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
		allBones[i]->parent_id = BoneListGetBoneIndex(allBones[i]->parent_name, allBones);
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
		GenBone_t& cbone = m_bones.append();
		cbone.refBone = model->bones[i];

		Msg(" %s\n", cbone.refBone->name);
	}

	Msg("  total bones: %d\n", m_bones.numElem());

	// set parents
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		const int parent_id = model->bones[i]->parent_id;

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
bool CEGFGenerator::ParseMaterialPaths(KVSection* pSection)
{
	MsgWarning("\nAdding material paths\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* keyBase = pSection->keys[i];

		if(!stricmp(keyBase->name, "materialpath"))
		{
			materialpathdesc_t& desc = m_matpathes.append();

			const EqString path = KV_GetValueString(keyBase);
			const int sp_len = path.Length()-1;

			if(sp_len >= 0 && (path[sp_len] != '/' || path[sp_len] != '\\'))
				strcpy(desc.searchPath, EqString::Format("%s/", path.ToCString()).ToCString());
			else
				strcpy(desc.searchPath, path.ToCString());

			Msg("   '%s'\n", desc.searchPath);			
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
		Msg(" %d material paths total\n", m_matpathes.numElem());

	return true;
}

//************************************
// Loads material pathes to use in engine
//************************************
bool CEGFGenerator::ParseMotionPackagePaths(KVSection* pSection)
{
	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* keyBase = pSection->keys[i];

		if (stricmp(keyBase->name, "addmotionpackage"))
			continue;

		if(m_lodparams.numElem() + 1 >= MAX_MOTIONPACKAGES)
		{
			MsgError("Exceeded motion packages count (MAX_MOTIONPACKAGES = %d)!", MAX_MOTIONPACKAGES);
			return false;
		}

		motionpackagedesc_t& desc = m_motionpacks.append();
		strcpy(desc.packageName, KV_GetValueString(keyBase));
	}

	if(m_motionpacks.numElem() > 0)
		Msg("Added %d external motion packages\n", m_motionpacks.numElem());

	return true;
}

//************************************
// Parses ik chain info from section
//************************************
void CEGFGenerator::ParseIKChain(KVSection* pSection)
{
	if(pSection->values.numElem() < 2)
	{
		MsgError("Too few arguments for 'ikchain'\n");
		MsgWarning("usage: ikchain (bone name) (effector bone name)\n");
		return;
	}

	GenIKChain_t ikCh;

	char effector_name[44];
	
	strcpy(ikCh.name, KV_GetValueString(pSection, 0));
	strcpy(effector_name, KV_GetValueString(pSection, 1));

	GenBone_t* effector_chain = FindBoneByName(effector_name);

	if(!effector_chain)
	{
		MsgError("ikchain: effector bone %s not found\n", effector_name);
		return;
	}

	// fill link list
	GenBone_t* cparent = effector_chain;
	do
	{
		GenIKLink_t& link = ikCh.link_list.append();

		link.damping = 1.0f;
		link.bone = cparent;
		link.mins = Vector3D(-360);
		link.maxs = Vector3D(360);

		// advance parent
		cparent = cparent->parent;
	} while(cparent != nullptr/* && cparent->parent != nullptr*/);

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* sec = pSection->keys[i];

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
				if(!stricmp(ikCh.link_list[j].bone->refBone->name, link_name))
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
				if(!stricmp(ikCh.link_list[j].bone->refBone->name, link_name))
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
void CEGFGenerator::ParseIKChains(KVSection* pSection)
{
	MsgWarning("\nLoading IK chains\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* chainSec = pSection->keys[i];

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
void CEGFGenerator::ParseAttachments(KVSection* pSection)
{
	MsgWarning("\nLoading attachments\n");

	for(int i = 0; i < pSection->keys.numElem(); i++)
	{
		KVSection* attachSec = pSection->keys[i];

		if (stricmp(attachSec->name, "attachment"))
			continue;

		if(attachSec->values.numElem() < 8)
		{
			MsgError("Invalid attachment definition\n");
			MsgWarning("usage: attachment (name) (bone name) (position x y z) (rotation x y z)\n");
			continue;
		}

		char attach_to_bone[44];
		const char* attachmentName = KV_GetValueString(attachSec, 1);
		strcpy(attach_to_bone, KV_GetValueString(attachSec, 1));
		GenBone_t* pBone = FindBoneByName(attach_to_bone);

		if (!pBone)
		{
			MsgError("Can't find bone %s for attachment %s\n", attach_to_bone, attachmentName);
			continue;
		}

		studioattachment_t& attach = m_attachments.append();
		strcpy(attach.name, attachmentName);
		attach.position = KV_GetVector3D(attachSec, 2);
		attach.angles = KV_GetVector3D(attachSec, 5);
		attach.bone_id = pBone->refBone->bone_id;

		MsgInfo("Adding attachment %s\n", attach.name);
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
	m_physModels.SaveToFile( outputPOD.ToCString() );

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
	m_modelLodLists.clear();
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
		SetRefsPath( _Es(filename).Path_Strip_Name().ToCString() );

		// strip filename to set reference models path
		MsgWarning("\nCompiling script \"%s\"\n", filename);

		// load all script data
		return InitFromKeyValues(scriptFile.GetRootSection());
	}

	MsgError("Can't open %s\n", filename);
	return false;
}

void CEGFGenerator::ParsePhysModels(KVSection* mainsection)
{
	for(int i = 0; i < mainsection->keys.numElem(); i++)
	{
		KVSection* physObjectSec = mainsection->keys[i];

		if(stricmp(physObjectSec->name, "physics"))
			continue;

		if(!physObjectSec->IsSection())
		{
			MsgError("*ERROR* key 'physics' must be a section\n");
			continue;
		}

		dsmmodel_t* physModel = nullptr;

		KVSection* modelNamePair = physObjectSec->FindSection("model");
		if(modelNamePair)
		{
			GenModel_t* foundRef = FindModelByName( KV_GetValueString(modelNamePair) );

			if(!foundRef)
			{
				// scaling already performed here
				const int newModelIdx = LoadModel( KV_GetValueString(modelNamePair) );

				if(newModelIdx == -1)
				{
					MsgError("*ERROR* Cannot find model reference '%s'\n", KV_GetValueString(modelNamePair));
					continue;
				}

				physModel = m_modelrefs[newModelIdx].model;
			}
			else
			{
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
bool CEGFGenerator::InitFromKeyValues(KVSection* mainsection)
{
	KVSection* pSourcePath = mainsection->FindSection("source_path");

	// set source path if defined by script
	if(pSourcePath)
		CombinePath(m_refsPath, 2, m_refsPath.ToCString(), KV_GetValueString(pSourcePath, 0, ""));

	// get new model filename
	SetOutputFilename(KV_GetValueString(mainsection->FindSection("modelfilename")));

	if(m_outputFilename.Length() == 0)
	{
		MsgError("No output model file name specified in the script ('modelfilename')\n");
		return false;
	}

	// try load models
	if( !ParseModels( mainsection ) )
		return false;

	// load lod/replacement data
	ParseLods( mainsection );

	// parse body groups
	if( !ParseBodyGroups( mainsection ) )
		return false;

	if (!ParseMaterialGroups(mainsection))
		return false;

	// merge bones first
	MergeBones();

	// build bone chain after loading models
	BuildBoneChains();

	// load material paths - needed for material system.
	if(!ParseMaterialPaths( mainsection ))
		return false;

	// external motion packages
	ParseMotionPackagePaths( mainsection );

	// load attachments if available
	ParseAttachments( mainsection );

	// load ik chains if available
	ParseIKChains( mainsection );

	// load and pre-generate physics models as final operation
	ParsePhysModels( mainsection );

	return true;
}