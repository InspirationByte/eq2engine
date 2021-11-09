//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////-

#include "dsm_loader.h"
#include "dsm_esm_loader.h"
#include "dsm_obj_loader.h"
#include "dsm_fbx_loader.h"

#include "core/DebugInterface.h"
#include "utils/KeyValues.h"
#include "utils/strtools.h"

#include <stdio.h>

namespace SharedModel
{

// sorts bones
int SortAndBalanceBones( int nCount, int nMaxCount, int* bones, float* weights )
{
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
				// swap
				int j = bones[i+1]; 
				bones[i+1] = bones[i]; 
				bones[i] = j;

				float w = weights[i+1];
				weights[i+1] = weights[i]; 
				weights[i] = w;

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

bool LoadSharedModel(dsmmodel_t* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());


	if (!stricmp(ext.GetData(), "esm"))
		return LoadESM(model, filename);

	if(!stricmp(ext.GetData(), "obj"))
		return LoadOBJ(model, filename);

	if (!stricmp(ext.GetData(), "fbx"))
		return LoadFBX(model, filename);

	return false;
}

bool SaveSharedModel(dsmmodel_t* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());

	if(!stricmp(ext.GetData(), "obj"))
		return SaveOBJ(model, filename);

	return false;
}

dsmgroup_t* dsmmodel_t::FindGroupByName(const char* pszGroupname)
{
	for(int i = 0; i < groups.numElem(); i++)
	{
		if(!stricmp(groups[i]->texture, pszGroupname))
			return groups[i];
	}

	return NULL;
}

dsmskelbone_t* dsmmodel_t::FindBone(const char* pszName)
{
	for(int i = 0; i < bones.numElem(); i++)
	{
		if(!stricmp(bones[i]->name, pszName))
			return bones[i];
	}

	return NULL;
}

void FreeDSMBones(dsmmodel_t* model)
{
	for(int i = 0; i < model->bones.numElem(); i++)
		delete model->bones[i];

	model->bones.clear();
}

void FreeDSM(dsmmodel_t* model)
{
	FreeDSMBones(model);

	for(int i = 0; i < model->groups.numElem(); i++)
		delete model->groups[i];

	model->groups.clear();
}

int GetTotalVertsOfDSM(dsmmodel_t* model)
{
	int numVerts = 0;

	for(int i = 0; i < model->groups.numElem(); i++)
		numVerts +=  model->groups[i]->verts.numElem();

	return numVerts;
}

}