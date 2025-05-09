//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////-

#include "core/core_common.h"
#include "utils/KeyValues.h"

#include "dsm_esm_loader.h"
#include "dsm_obj_loader.h"
#include "dsm_fbx_loader.h"

#include "dsm_loader.h"

namespace SharedModel
{

// sorts bones
int SortAndBalanceBones( int count, int maxCount, int* bones, float* weights, const float minWeight)
{
	if (!count)
		return 0;

	// collapse duplicate bone weights
	for (int i = 0; i < count-1; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			if (bones[i] == bones[j])
			{
				weights[i] += weights[j];
				weights[j] = 0.0;
			}
		}
	}

	// sort in order
	bool needSort;
	do 
	{
		needSort = false;
		for (int i = 0; i < count-1; i++)
		{
			if (weights[i+1] > weights[i])
			{
				QuickSwap(bones[i + 1], bones[i]);
				QuickSwap(weights[i + 1], weights[i]);

				needSort = true;
			}
		}
	} while (needSort);

	// throw away all weights less than 1/20th
	while (count > 1 && weights[count-1] < minWeight)
		count--;

	// clip to the top iMaxCount bones
	if (count > maxCount)
		count = maxCount;

	float t = 0.0f;
	for (int i = 0; i < count; i++)
		t += weights[i];

	if (t <= 0.0f)
	{
		// missing weights?, go ahead and evenly share?
		// FIXME: shouldn't this error out?
		t = 1.0f / count;
		for (int i = 0; i < count; i++)
			weights[i] = t;
	}
	else
	{
		// scale to sum to 1.0
		t = 1.0f / t;
		for (int i = 0; i < count; i++)
			weights[i] *= t;
	}

	return count;
}

bool LoadSharedModel(DSModel& model, const char* filename)
{
	const EqString ext = fnmPathExtractExt(filename);

	if (ext == "esm")
		return LoadESM(model, filename);

	if (ext == "obj")
		return LoadOBJ(model, filename);

	if (ext == "fbx")
		return LoadFBXCompound(model, filename);

	return false;
}

bool SaveSharedModel(const DSModel& model, const char* filename)
{
	const EqString ext = fnmPathExtractExt(filename);

	if(ext == "obj")
		return SaveOBJ(model, filename);

	return false;
}

DSModel::~DSModel()
{
	for (DSMesh* mesh : meshes)
		delete mesh;

	bones.clear(true);
	meshes.clear(true);
}

DSMesh* DSModel::FindMeshByName(const char* pszGroupname)
{
	for(DSMesh* mesh : meshes)
	{
		if(!mesh->texture.CompareCaseIns(pszGroupname))
			return mesh;
	}

	return nullptr;
}

DSBone* DSModel::FindBone(const char* pszName)
{
	const int idx = arrayFindIndexF(bones, [pszName](const DSBone& bone) {
		return bone.name.CompareCaseIns(pszName) == 0;
	});
	if(idx == -1)
		return nullptr;
	return &bones[idx];
}

int GetTotalVertsOfDSM(const DSModel& model)
{
	int numVerts = 0;
	for(const DSMesh* mesh : model.meshes)
		numVerts += mesh->verts.numElem();

	return numVerts;
}

}