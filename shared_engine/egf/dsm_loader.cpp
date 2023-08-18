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
int SortAndBalanceBones( int nCount, int nMaxCount, int* bones, float* weights )
{
	if (!nCount)
		return 0;

	// collapse duplicate bone weights
	for (int i = 0; i < nCount-1; i++)
	{
		for (int j = i + 1; j < nCount; j++)
		{
			if (bones[i] == bones[j])
			{
				weights[i] += weights[j];
				weights[j] = 0.0;
			}
		}
	}

	// sort in order
	int bShouldSort;
	do 
	{
		bShouldSort = false;
		for (int i = 0; i < nCount-1; i++)
		{
			if (weights[i+1] > weights[i])
			{
				QuickSwap(bones[i + 1], bones[i]);
				QuickSwap(weights[i + 1], weights[i]);

				bShouldSort = true;
			}
		}
	}while (bShouldSort);

	// throw away all weights less than 1/20th
	while (nCount > 1 && weights[nCount-1] < 0.05f)
		nCount--;

	// clip to the top iMaxCount bones
	if (nCount > nMaxCount)
		nCount = nMaxCount;

	float t = 0.0f;

	for (int i = 0; i < nCount; i++)
		t += weights[i];

	if (t <= 0.0f)
	{
		// missing weights?, go ahead and evenly share?
		// FIXME: shouldn't this error out?
		t = 1.0f / nCount;

		for (int i = 0; i < nCount; i++)
			weights[i] = t;
	}
	else
	{
		// scale to sum to 1.0
		t = 1.0f / t;

		for (int i = 0; i < nCount; i++)
			weights[i] *= t;
	}

	return nCount;
}

bool LoadSharedModel(DSModel* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());

	if (!stricmp(ext.GetData(), "esm"))
		return LoadESM(model, filename);

	if(!stricmp(ext.GetData(), "obj"))
		return LoadOBJ(model, filename);

	if (!stricmp(ext.GetData(), "fbx"))
		return LoadFBXCompound(model, filename);

	return false;
}

bool SaveSharedModel(DSModel* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());

	if(!stricmp(ext.GetData(), "obj"))
		return SaveOBJ(model, filename);

	return false;
}

DSModel::~DSModel()
{
	for (int i = 0; i < bones.numElem(); i++)
		delete bones[i];

	for (int i = 0; i < meshes.numElem(); i++)
		delete meshes[i];

	bones.clear(true);
	meshes.clear(true);
}

DSMesh* DSModel::FindMeshByName(const char* pszGroupname)
{
	for(int i = 0; i < meshes.numElem(); i++)
	{
		if(!stricmp(meshes[i]->texture, pszGroupname))
			return meshes[i];
	}

	return nullptr;
}

DSBone* DSModel::FindBone(const char* pszName)
{
	for(int i = 0; i < bones.numElem(); i++)
	{
		if(!stricmp(bones[i]->name, pszName))
			return bones[i];
	}

	return nullptr;
}

int GetTotalVertsOfDSM(DSModel* model)
{
	int numVerts = 0;

	for(int i = 0; i < model->meshes.numElem(); i++)
		numVerts +=  model->meshes[i]->verts.numElem();

	return numVerts;
}

}