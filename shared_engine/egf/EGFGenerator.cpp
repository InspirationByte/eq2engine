//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
CEGFGenerator::GenBone* CEGFGenerator::FindBoneByName(const char* pszName) const
{
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		if(!m_bones[i].refBone->name.CompareCaseIns(pszName))
			return (GenBone*)&m_bones[i];
	}
	return nullptr;
}

//************************************
// Finds lod model
//************************************
CEGFGenerator::GenLODList* CEGFGenerator::FindModelLodGroupByName(const char* pszName) const
{
	for(int i = 0; i < m_modelLodLists.numElem(); i++)
	{
		if(!m_modelLodLists[i].name.CompareCaseIns(pszName))
			return (GenLODList*)&m_modelLodLists[i];
	}
	return nullptr;
}

//************************************
// Finds egfCa model
//************************************
int CEGFGenerator::FindModelIndexByName(const char* pszName) const
{
	for (int i = 0; i < m_modelrefs.numElem(); i++)
	{
		if (!m_modelrefs[i].name.CompareCaseIns(pszName))
			return i;
	}
	return -1;
}


CEGFGenerator::GenModel* CEGFGenerator::FindModelByName(const char* pszName) const
{
	const int foundIdx = FindModelIndexByName(pszName);
	if (foundIdx == -1)
		return nullptr;

	return (GenModel*)&m_modelrefs[foundIdx];
}

//************************************
// Finds lod model, returns index
//************************************
int CEGFGenerator::FindModelLodIdGroupByName(const char* pszName) const
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
int CEGFGenerator::GetMaterialIndex(const char* pszName) const
{
	for(int i = 0; i < m_materials.numElem(); i++)
	{
		if(!CString::CompareCaseIns(m_materials[i].materialname, pszName))
			return i;
	}

	return -1;
}

//************************************
// Loads a model
//************************************
bool CEGFGenerator::LoadModel(const char* pszFileName, GenModel& mod)
{
	if (!CString::CompareCaseIns(pszFileName, "_dummy"))
		return false;

	mod.model = CRefPtr_new(DSModel);

	EqString modelPath;
	fnmPathCombine(modelPath, m_refsPath.ToCString(), pszFileName);

	EqString ext(fnmPathExtractExt(modelPath));

	if (!ext.CompareCaseIns("fbx"))
	{
		mod.shapeData = CRefPtr_new(DSShapeData);

		Msg("Loading FBX from '%s'\n", modelPath.ToCString());

		DSModelContainer sharedModel;
		if (!LoadFBXShapes(sharedModel, modelPath))
		{
			SAFE_DELETE(mod.shapeData);

			MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
			FreeModel(mod);
			return -1;
		}

		mod.model = sharedModel.model;
		mod.shapeData = sharedModel.shapeData;
		mod.transform = sharedModel.transform;
	}
	else if( !ext.CompareCaseIns("esx") ) // Legacy now because I want to drop the Blender ESM/ESX plugin support
	{
		mod.shapeData = CRefPtr_new(DSShapeData);

		Msg("Loading shape file '%s'\n", modelPath.ToCString());

		if( LoadESXShapes( mod.shapeData, modelPath.ToCString() ))
		{
			// use referenced filename by the shape file
			fnmPathCombine(modelPath, m_refsPath.ToCString(), mod.shapeData->reference.ToCString());
		}
		else
		{
			mod.shapeData = nullptr;
		}

		if (!LoadSharedModel(mod.model, modelPath.ToCString()))
		{
			MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
			FreeModel(mod);
			return false;
		}
	}
	else if(!LoadSharedModel(mod.model, modelPath.ToCString()))
	{
		MsgError("Reference model '%s' cannot be loaded!\n", modelPath.ToCString());
		FreeModel(mod);
		return false;
	}

	if (!PostProcessDSM(mod))
	{
		FreeModel(mod);
		return false;
	}

	return true;
}

bool CEGFGenerator::PostProcessDSM(GenModel& mod)
{
	const int nVerts = GetTotalVertsOfDSM(mod.model);

	if ((float)nVerts / 3.0f != nVerts / 3)
	{
		MsgError("Reference model '%s' has invalid triangles (tip: vertex count must be divisible by 3 without remainder)\n", mod.model->name.ToCString());
		return false;
	}

	// assign shape indexes
	if (mod.shapeData)
	{
		AssignShapeKeyVertexIndexes(mod.model, mod.shapeData);

		for (int i = 0; i < mod.shapeData->shapes.numElem(); ++i)
		{
			DSShapeKey* shapeKey = mod.shapeData->shapes[i];
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
		DSBone* bone = mod.model->bones[i];

		bone->position *= m_modelScale;

		if (bone->parentIdx == -1)
			bone->position += m_modelOffset;
	}

	// check material list and move/scale verts
	for (int i = 0; i < mod.model->meshes.numElem(); i++)
	{
		DSMesh* group = mod.model->meshes[i];

		// scale vertices
		for (int j = 0; j < group->verts.numElem(); j++)
		{
			group->verts[j].position *= m_modelScale;
			group->verts[j].position += m_modelOffset;
		}

		// if material is not found, add new one
		int found = GetMaterialIndex(mod.model->meshes[i]->texture);
		if (found == -1)
		{
			if (m_materials.numElem() - 1 == MAX_STUDIOMATERIALS)
			{
				MsgError("Exceeded used material count (MAX_STUDIOMATERIALS = %d)!", MAX_STUDIOMATERIALS);
				break;
			}

			// create new material
			GenMaterialDesc desc;
			strcpy(desc.materialname, mod.model->meshes[i]->texture);

			m_materials.append(desc);
		}
	}

	return true;
}

//************************************
// Frees model ref
//************************************
void CEGFGenerator::FreeModel(GenModel& mod )
{
	mod.model = nullptr;
	mod.shapeData = nullptr;
}

void CEGFGenerator::LoadModelsFromFBX(const KVSection* pKeyBase)
{
	EqString modelPath;
	fnmPathCombine(modelPath, m_refsPath.ToCString(), KV_GetValueString(pKeyBase));

	Msg("Using FBX Source '%s'\n", KV_GetValueString(pKeyBase));

	Array<DSModelContainer> fbxModels(PP_SL);
	if (!LoadFBX(fbxModels, modelPath))
		return;

	for (const KVSection* modelSec : pKeyBase->Keys())
	{
		const char* modelName = modelSec->name;
		const char* refName = KV_GetValueString(modelSec);

		const int foundIdx = arrayFindIndexF(fbxModels, [refName](const DSModelContainer& cont) {
			return !cont.model->name.CompareCaseIns(refName);
		});

		if (foundIdx == -1)
			continue;

		const DSModelContainer& cont = fbxModels[foundIdx];

		GenModel mod;
		mod.name = modelName;
		mod.model = cont.model;
		mod.shapeData = cont.shapeData;
		mod.transform = cont.transform;

		// DRVSYN: vertex order for damaged model
		if (modelSec->values.numElem() > 1 && !CString::CompareCaseIns(KV_GetValueString(modelSec, 1), "shapeBy"))
		{
			const char* shapeKeyName = KV_GetValueString(modelSec, 2, nullptr);

			if (shapeKeyName)
			{
				mod.shapeIndex = FindShapeKeyIndex(mod.shapeData, shapeKeyName);
				if (mod.shapeIndex == -1)
					MsgError("Error: shapeBy - Can't find shape key %s in model %s (ref %s)\n", shapeKeyName, mod.model->name.ToCString(), mod.name.ToCString());
			}
			else
				MsgError("Error: shapeBy - no shape key specified for ref %s\n", mod.model->name.ToCString());
		}

		if (!PostProcessDSM(mod))
			return;

		const int newModelIndex = m_modelrefs.append(mod);

		// Start a new LOD. LOD has a name of reference
		GenLODList& lodModel = m_modelLodLists.append();
		lodModel.lodmodels.append(newModelIndex);
		lodModel.name = modelName;

		const int nVerts = GetTotalVertsOfDSM(mod.model);

		Msg("Adding reference %s as '%s' with %d triangles (in %d meshes), %d bones\n",
			mod.model->name.ToCString(),
			modelName,
			nVerts / 3,
			mod.model->meshes.numElem(),
			mod.model->bones.numElem());
	}
}

//************************************
// Loads reference ESM files
//************************************
int CEGFGenerator::ParseAndLoadModels(const KVSection* pKeyBase)
{
	Array<EqStringRef> modelNames(PP_SL);
	Array<EqStringRef> shapeByModels(PP_SL);

	EqStringRef refName, modelName, cmdName, shapeName;
	const int argCount = pKeyBase->GetValues(refName, modelName, cmdName, shapeName);

	if (argCount == 0)
	{
		MsgError("Error - model ref name was not specified in %s\n", pKeyBase->GetName());
		return -1;
	}

	if (argCount >= 2)
	{
		modelNames.append(modelName);

		if(!cmdName.CompareCaseIns("shapeBy"))
		{
			Msg("Model shape key is '%s'\n", shapeName.ToCString());
			shapeByModels.append(shapeName);
		}
		else
			shapeByModels.append(nullptr);
	}
	else
	{
		// go through all keys inside model section
		for (const KVSection* sec : pKeyBase->Keys())
		{
			modelNames.append(sec->GetName());
			shapeByModels.append(nullptr);

			Msg("Adding model '%s'\n", sec->GetName());
		}
	}

	// load the models
	Array<GenModel> models(PP_SL);
	for(int i = 0; i < modelNames.numElem(); i++)
	{
		Msg("Loading model '%s'\n", modelNames[i].ToCString());

		GenModel mod;
		if(!LoadModel(modelNames[i], mod))
			continue;

		// set model to as part name
		mod.name = refName;

		if (shapeByModels[i])
		{
			mod.shapeIndex = FindShapeKeyIndex(mod.shapeData, shapeByModels[i]);
			if (mod.shapeIndex == -1)
				MsgError("Error: shapeBy - Can't find shape key %s in model %s (ref %s)\n", shapeByModels[i].ToCString(), mod.model->name.ToCString(), mod.name.ToCString());
		}

		// add finally
		models.append(mod);
	}

	int retModelIdx = -1;

	// merge the models
	if(models.numElem() > 1)
	{
		DSModelPtr merged = CRefPtr_new(DSModel);

		// set model to as part name
		merged->name = refName;

		for(int i = 0; i < models.numElem(); i++)
		{
			DSModel* model = models[i].model;

			if(i == 0)
			{
				merged->bones.append(model->bones);
				model->bones.clear();
			}

			merged->meshes.append(model->meshes);
			model->meshes.clear();

			FreeModel(models[i]);
		}

		GenModel mref;
		mref.model = merged;
		mref.shapeData = nullptr;
		mref.name = merged->name;

		retModelIdx = m_modelrefs.append(mref);

		GenLODList& lodList = m_modelLodLists.append();
		lodList.lodmodels.append(retModelIdx);
		lodList.name = mref.name;

		const int nVerts = GetTotalVertsOfDSM(mref.model);
		Msg("Adding reference %s '%s' with %d triangles (in %d meshes), %d bones\n",
			modelNames[0].ToCString(),
			mref.name.ToCString(),
			nVerts / 3,
			mref.model->meshes.numElem(),
			mref.model->bones.numElem());
	}
	else if(models.numElem() > 0)
	{
		retModelIdx = m_modelrefs.append(models[0]);

		GenLODList& lodList = m_modelLodLists.append();
		lodList.lodmodels.append(retModelIdx);
		lodList.name = models[0].name;

		const int nVerts = GetTotalVertsOfDSM(models[0].model);
		Msg("Adding reference %s '%s' with %d triangles (in %d meshes), %d bones\n",
			modelNames[0].ToCString(),
			models[0].name.ToCString(),
			nVerts / 3,
			models[0].model->meshes.numElem(),
			models[0].model->bones.numElem());
	}

	if(retModelIdx)
		MsgError("got model definition '%s', but nothing added\n", refName.ToCString());

	return retModelIdx;
}

//************************************
// Loads reference models
//************************************
bool CEGFGenerator::ParseModels(const KVSection* pSection)
{
	MsgWarning("\nLoading models\n");

	for(const KVSection* keyBase : pSection->Keys())
	{
		if(!keyBase->name.CompareCaseIns("global_scale"))
		{
			// try apply global scale
			m_modelScale = KV_GetVector3D(keyBase, 0, Vector3D(1.0f));
		}
		if(!keyBase->name.CompareCaseIns("global_offset"))
		{
			// try apply global offset
			m_modelOffset = KV_GetVector3D(keyBase, 0, vec3_zero);
		}
		else if (!keyBase->name.CompareCaseIns("FBXSource"))
		{
			LoadModelsFromFBX(keyBase);
		}
		else if(!keyBase->name.CompareCaseIns("model"))
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
	GenModel mod{ CRefPtr_new(DSModel), nullptr, identity4, "_dummy" };
	mod.model->name = "_dummy";
	const int modelIdx = m_modelrefs.append(mod);

	GenLODList& dummyLodModel = m_modelLodLists.append();
	dummyLodModel.lodmodels.append(modelIdx);
	dummyLodModel.name = mod.name;

	return true;
}

//************************************
// Parses LOD data
//************************************
void CEGFGenerator::ParseLodData(const KVSection* pSection, int lodIdx)
{
	for(KVSection* lodModelSec : pSection->Keys("replace"))
	{
		EqStringRef target, replaceBy;
		if (lodModelSec->GetValues(target, replaceBy) < 2)
		{
			MsgError("replace - insufficient args\n  Example: 'replace target_model replace_to'\n");
			continue;
		}

		GenLODList* lodgroup = FindModelLodGroupByName(target);
		if (!lodgroup)
		{
			MsgError("No such reference named %s\n", target.ToCString());
			continue;
		}

		lodModelSec->SetValue(EqString::Format("%s_l%d", target, lodIdx));
		
		const int replaceByExisting = FindModelIndexByName(replaceBy);
		const int replaceByModel = replaceByExisting != -1 ? replaceByExisting : ParseAndLoadModels(lodModelSec);
		lodgroup->lodmodels.append(replaceByModel);
	}

	for (const KVSection* lodModelSec : pSection->Keys("simplify"))
	{
		EqStringRef target;
		float simplifyThreshold = 0.0f;
		if (lodModelSec->GetValues(target, simplifyThreshold) < 2)
		{
			MsgError("simplify - insufficient args\n  Example: 'simplify target_model 0.25'\n");
			continue;
		}

		GenLODList* lodgroup = FindModelLodGroupByName(target);
		if (!lodgroup)
		{
			MsgError("No such reference named %s\n", target.ToCString());
			continue;
		}

		GenModel newModel = m_modelrefs[lodgroup->lodmodels[0]];
		newModel.simplifyThreshold = simplifyThreshold;
		newModel.name.Append(EqString::Format("_lod_%g", simplifyThreshold * 100));

		const int replaceByModel = m_modelrefs.append(newModel);
		lodgroup->lodmodels.append(replaceByModel);
	}
}

//************************************
// Parses LODs
//************************************
void CEGFGenerator::ParseLods(const KVSection* pSection)
{
	MsgWarning("\nLoading LODs\n");

	// always add first default lod
	studioLodParams_t lod;
	lod.distance = 0.0f;
	lod.flags = 0;
	m_lodparams.append(lod);

	for(const KVSection* lodKey : pSection->Keys("lod", KV_FLAG_SECTION))
	{
		if (m_lodparams.numElem() + 1 >= MAX_MODEL_LODS)
		{
			MsgError("Reached max lod count (MAX_MODEL_LODS = %d)!", MAX_MODEL_LODS);
			return;
		}

		const float lodDist = KV_GetValueFloat(lodKey, 0, 1.0f);
		const int lodIdx = m_lodparams.numElem();

		studioLodParams_t& newlod = m_lodparams.append();
		newlod.distance = lodDist;
		newlod.flags = 0;

		const char* lodFlagStr = KV_GetValueString(lodKey, 1, nullptr);
		if (lodFlagStr && !CString::CompareCaseIns(lodFlagStr, "manual"))
		{
			newlod.flags |= STUDIO_LOD_FLAG_MANUAL;
		}

		ParseLodData(lodKey, lodIdx);

		Msg("Added lod %d, distance: %2f\n", lodIdx, lodDist);
	}
}


//************************************
// Load body groups
//************************************
bool CEGFGenerator::ParseBodyGroups(const KVSection* pSection)
{
	for(const KVSection* keyBase : pSection->Keys("bodygroup"))
	{
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

			studioBodyGroup_t& bodygroup = m_bodygroups.append();
			strcpy(bodygroup.name, bodyGroupName);
			bodygroup.lodModelIndex = lodIndex;

			++m_modelLodLists[lodIndex].used;

			Msg("Added body group '%s'\n", bodygroup.name);
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
bool CEGFGenerator::ParseMaterialGroups(const KVSection* pSection)
{
	MsgInfo("* Default materialGroup:\n\t");
	for (int i = 0; i < m_materials.numElem(); i++)
		MsgInfo("%s ", m_materials[i].materialname);
	MsgInfo("\n");

	for (KVKeyIterator it(pSection, "materialGroup"); !it.atEnd(); ++it)
	{
		const KVSection* keyBase = *it;
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

		GenMaterialGroup* group = PPNew GenMaterialGroup();
		m_matGroups.append(group);

		MsgInfo("Added materialGroup: ");

		for (int j = 0; j < keyBase->values.numElem(); j++)
		{
			// create new material
			GenMaterialDesc desc;
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
bool BoneListCheckForBone(const char* pszName, const Array<DSBone*> &pBones)
{
	for(int i = 0; i < pBones.numElem(); i++)
	{
		if(!pBones[i]->name.CompareCaseIns(pszName))
			return true;
	}

	return false;
}

//************************************
// returns bone index for availablity in list
//************************************
int BoneListGetBoneIndex(const char* pszName, const Array<DSBone*> &pBones)
{
	for(int i = 0; i < pBones.numElem(); i++)
	{
		if(!pBones[i]->name.CompareCaseIns(pszName))
			return i;
	}

	return -1;
}


//************************************
// Remaps vertex bone indices new_bones
//************************************
void BoneRemapDSMGroup(DSMesh* pMesh, const Array<DSBone*> &old_bones, Array<DSBone*> &new_bones)
{
	for(int i = 0; i < pMesh->verts.numElem(); i++)
	{
		for(int j = 0; j < pMesh->verts[i].weights.numElem(); j++)
		{
			int boneIdx = pMesh->verts[i].weights[j].bone;

			// find bone in new list by bone name in old list
			int new_bone_id = BoneListGetBoneIndex(old_bones[boneIdx]->name, new_bones);

			// point to bone in new list
			pMesh->verts[i].weights[j].bone = new_bone_id;
		}
	}
}

//************************************
// Remaps vertex bone indices and merges skeleton
//************************************
void BoneMergeRemapDSM(DSModel* pDSM, Array<DSBone*> &new_bones)
{
	// remap meshes
	for(int i = 0; i < pDSM->meshes.numElem(); i++)
	{
		BoneRemapDSMGroup(pDSM->meshes[i], pDSM->bones, new_bones);
	}

	// reset skeleton
	for (int i = 0; i < pDSM->bones.numElem(); ++i)
		delete pDSM->bones[i];
	pDSM->bones.clear(true);

	// and copy it
	for(int i = 0; i < new_bones.numElem(); i++)
	{
		// copy bone
		DSBone* pBone = PPNew DSBone;
		*pBone = *new_bones[i];

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
	Array<DSBone*> allBones(PP_SL);

	for(const GenModel& gm : m_modelrefs)
	{
		for(DSBone* bone : gm.model->bones)
		{
			// not found in new list? add
			if(!BoneListCheckForBone(bone->name, allBones))
			{
				// copy bone
				DSBone* cloneBone = PPNew DSBone;
				*cloneBone = *bone;

				// set new bone id
				cloneBone->boneIdx = allBones.numElem();

				allBones.append(cloneBone);
			}
		}
	}

	// relink parent bones
	for(int i = 0; i < allBones.numElem(); i++)
	{
		allBones[i]->parentIdx = BoneListGetBoneIndex(allBones[i]->parentName, allBones);
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
	DSModel* model = m_modelrefs[0].model;

	// make bone list first
	for(int i = 0; i < model->bones.numElem(); i++)
	{
		GenBone& cbone = m_bones.append();
		cbone.refBone = model->bones[i];

		Msg(" %s\n", cbone.refBone->name.ToCString());
	}

	Msg("  total bones: %d\n", m_bones.numElem());

	// set parents
	for(int i = 0; i < m_bones.numElem(); i++)
	{
		const int parentIdx = model->bones[i]->parentIdx;

		if(parentIdx == -1)
			m_bones[i].parent = nullptr;
		else
			m_bones[i].parent = &m_bones[parentIdx];
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
bool CEGFGenerator::ParseMaterialPaths(const KVSection* pSection)
{
	MsgWarning("\nAdding material paths\n");

	for(const KVSection* keyBase : pSection->Keys())
	{
		if(!keyBase->name.CompareCaseIns("materialpath"))
		{
			materialPathDesc_t& desc = m_matpathes.append();

			const EqString path = KV_GetValueString(keyBase);
			const int sp_len = path.Length()-1;

			if(sp_len >= 0 && (path[sp_len] != '/' || path[sp_len] != '\\'))
				strcpy(desc.searchPath, EqString::Format("%s/", path.ToCString()).ToCString());
			else
				strcpy(desc.searchPath, path.ToCString());

			Msg("   '%s'\n", desc.searchPath);			
		}

		if(	!keyBase->name.CompareCaseIns("notextures") ||
			!keyBase->name.CompareCaseIns("nomaterials"))
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
bool CEGFGenerator::ParseMotionPackagePaths(const KVSection* pSection)
{
	for(const KVSection* keyBase : pSection->Keys("addMotionPackage"))
	{
		if(m_lodparams.numElem() + 1 >= MAX_MOTIONPACKAGES)
		{
			MsgError("Exceeded motion packages count (MAX_MOTIONPACKAGES = %d)!", MAX_MOTIONPACKAGES);
			return false;
		}

		motionPackageDesc_t& desc = m_motionpacks.append();
		strcpy(desc.packageName, KV_GetValueString(keyBase));
	}

	if(m_motionpacks.numElem() > 0)
		Msg("Added %d external motion packages\n", m_motionpacks.numElem());

	return true;
}

//************************************
// Parses ik chain info from section
//************************************
void CEGFGenerator::ParseIKChain(const KVSection* pSection)
{
	if(pSection->values.numElem() < 2)
	{
		MsgError("Too few arguments for 'ikchain'\n");
		MsgWarning("usage: ikchain (bone name) (effector bone name)\n");
		return;
	}

	GenIKChain ikCh;

	char effector_name[44];
	
	strcpy(ikCh.name, KV_GetValueString(pSection, 0));
	strcpy(effector_name, KV_GetValueString(pSection, 1));

	GenBone* effector_chain = FindBoneByName(effector_name);

	if(!effector_chain)
	{
		MsgError("ikchain: effector bone %s not found\n", effector_name);
		return;
	}

	// fill link list
	GenBone* cparent = effector_chain;
	do
	{
		GenIKLink& link = ikCh.link_list.append();

		link.damping = 1.0f;
		link.bone = cparent;
		link.mins = Vector3D(-360);
		link.maxs = Vector3D(360);

		// advance parent
		cparent = cparent->parent;
	} while(cparent != nullptr/* && cparent->parent != nullptr*/);

	for(const KVSection* sec : pSection->Keys())
	{
		if(!sec->name.CompareCaseIns("damping"))
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
				if(!ikCh.link_list[j].bone->refBone->name.CompareCaseIns(link_name))
				{
					ikCh.link_list[j].damping = fDamp;
					break;
				}
			}
		}
		else if(!sec->name.CompareCaseIns("link_limits"))
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
				if(!ikCh.link_list[j].bone->refBone->name.CompareCaseIns(link_name))
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
void CEGFGenerator::ParseIKChains(const KVSection* pSection)
{
	MsgWarning("\nLoading IK chains\n");

	for(const KVSection* chainSec : pSection->Keys("ikchain", KV_FLAG_SECTION))
	{
		ParseIKChain(chainSec);
	}

	if(m_ikchains.numElem() > 0)
		Msg("Parsed %d IK chains\n", m_ikchains.numElem());
}

//************************************
// Loads attachments if available
//************************************
void CEGFGenerator::ParseAttachments(const KVSection* pSection)
{
#if 0
	// before we do that we add each used model transform
	for (const GenModel& gm : m_modelrefs)
	{
		if (!gm.used)
			continue;

		studiotransform_t& attach = m_transforms.append();
		strcpy(attach.name, gm.name);

		attach.transform = gm.transform;
		attach.attachBoneIdx = EGF_INVALID_IDX;
	}
#endif
	MsgWarning("\nLoading attachments\n");

	for(const KVSection* attachSec : pSection->Keys("attachment"))
	{
		if(attachSec->values.numElem() < 8)
		{
			MsgError("Invalid attachment definition\n");
			MsgWarning("usage: attachment (name) (boneName or \"none\") (position x y z) (rotation x y z)\n");
			continue;
		}

		const char* attachmentName = KV_GetValueString(attachSec, 0);
		const char* attachBoneName = KV_GetValueString(attachSec, 1);

		GenBone* pBone = nullptr;
		if (CString::CompareCaseIns(attachBoneName, "none"))
		{
			GenBone* pBone = FindBoneByName(attachBoneName);
			if (!pBone)
			{
				MsgError("Can't find bone %s for attachment %s\n", attachBoneName, attachmentName);
				continue;
			}
		}

		const int existingTransform = arrayFindIndexF(m_transforms, [&](const studioTransform_t& tr) 
		{
			return !CString::CompareCaseIns(tr.name, attachmentName);
		});

		if (existingTransform != -1)
		{
			MsgError("Updating transform '%s' with bone attachment '%s'\n", attachBoneName, attachmentName);
			m_transforms[existingTransform].attachBoneIdx = pBone ? pBone->refBone->boneIdx : EGF_INVALID_IDX;
			continue;
		}

		studioTransform_t& attach = m_transforms.append();
		strcpy(attach.name, attachmentName);

		attach.transform = identity4;
		attach.transform.setRotation(DEG2RAD(KV_GetVector3D(attachSec, 2)));
		attach.transform.setTranslation(m_modelScale * KV_GetVector3D(attachSec, 5) + m_modelOffset);
		attach.attachBoneIdx = pBone ? pBone->refBone->boneIdx : EGF_INVALID_IDX;

		MsgInfo("Adding custom transform attachment %s\n", attach.name);
	}

	if(m_transforms.numElem())
		Msg("Total transforms: %d\n", m_transforms.numElem());
}

//************************************
// main function of script compilation
//************************************
bool CEGFGenerator::GeneratePOD()
{
	if(!m_physModels.HasObjects())
		return true;

	MsgWarning("\nWriting physics objects...\n");

	const EqString physDataFilename = fnmPathApplyExt(m_outputFilename, s_egfPhysicsObjectExt);
	m_physModels.SaveToFile(physDataFilename);

	return true;
}

void CEGFGenerator::Cleanup()
{
	for (int i = 0; i < m_matGroups.numElem(); ++i)
		delete m_matGroups[i];
	m_matGroups.clear();

	for (int i = 0; i < m_modelrefs.numElem(); i++)
		FreeModel(m_modelrefs[i]);

	m_modelrefs.clear(true);
	m_modelLodLists.clear(true);
	m_lodparams.clear(true);
	m_motionpacks.clear(true);
	m_matpathes.clear(true);
	m_ikchains.clear(true);
	m_bones.clear(true);
	m_transforms.clear(true);
	m_bodygroups.clear(true);
	m_materials.clear(true);
	m_usedMaterials.clear(true);
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
		SetRefsPath(fnmPathStripName(filename));

		// strip filename to set reference models path
		MsgWarning("\nCompiling script \"%s\"\n", filename);

		// load all script data
		return InitFromKeyValues(scriptFile.GetRootSection());
	}

	MsgError("Can't open %s\n", filename);
	return false;
}

void CEGFGenerator::ParsePhysModels(const KVSection* mainsection)
{
	for(const KVSection* physObjectSec : mainsection->Keys("physics"))
	{
		if(!physObjectSec->IsSection())
		{
			MsgError("*ERROR* key 'physics' must be a section\n");
			continue;
		}

		DSModel* physModel = nullptr;

		const KVSection* modelNamePair = physObjectSec->FindSection("model");
		if(modelNamePair)
		{
			GenModel* foundRef = FindModelByName( KV_GetValueString(modelNamePair) );

			if(!foundRef)
			{
				// scaling already performed here
				GenModel model;
				if(!LoadModel(KV_GetValueString(modelNamePair), model))
				{
					MsgError("*ERROR* Cannot find model reference '%s'\n", KV_GetValueString(modelNamePair));
					continue;
				}

				m_modelrefs.append(model);

				physModel = model.model;
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
bool CEGFGenerator::InitFromKeyValues(const KVSection* mainsection)
{
	const KVSection* pSourcePath = mainsection->FindSection("source_path");

	// set source path if defined by script
	if(pSourcePath)
		fnmPathCombine(m_refsPath, m_refsPath.ToCString(), KV_GetValueString(pSourcePath, 0, ""));

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