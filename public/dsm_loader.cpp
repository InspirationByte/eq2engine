//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////-

#include "dsm_loader.h"
#include "core_base_header.h"

#pragma todo("Remove old DSM loader")

extern bool LoadOBJ(dsmmodel_t* model, const char* filename);
extern bool LoadESM(dsmmodel_t* model, const char* filename);
extern bool SaveOBJ(dsmmodel_t* model, const char* filename);

bool LoadSharedModel(dsmmodel_t* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());

	if(!stricmp(ext.GetData(), "obj"))
		return LoadOBJ(model, filename);
	else if(!stricmp(ext.GetData(), "esm"))
		return LoadESM(model, filename);
	
	return false;
}

bool SaveSharedModel(dsmmodel_t* model, const char* filename)
{
	EqString file(filename);
	EqString ext(file.Path_Extract_Ext());

	if(!stricmp(ext.GetData(), "obj"))
		return SaveOBJ(model, filename);
	else
		return SaveDSM(model, filename);
}

int xstrsplitws(char* str, char **pointer_array)
{
	char c = *str;

	int num_indices = 0;

	bool bAdd = true;

	while(c != '\0')
	{
		c = *str;

		if(bAdd)
		{
			pointer_array[num_indices] = str;
			num_indices++;
			bAdd = false;
		}

		if( isspace(c) )
		{
			bAdd = true;
			*str = '\0'; // make null-string
		}

		str++;
	}

	return num_indices;
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

extern int SortAndBalanceBones( int nCount, int nMaxCount, int* bones, float* weights );

bool LoadDSM_Version2(dsmmodel_t* model, kvkeybase_t* pSection)
{
	kvkeybase_t* pSkeletonSec = pSection->FindKeyBase( "skeleton", KV_FLAG_SECTION );
	if(pSkeletonSec)
	{
		int numBones = pSkeletonSec->keys.numElem();

		for(int i = 0; i < numBones; i++)
		{
			dsmskelbone_t* pBone = new dsmskelbone_t;

			strcpy(pBone->name, pSkeletonSec->keys[i]->name);

			int args = sscanf(
				KV_GetValueString(pSkeletonSec->keys[i]), 
				"%d %f %f %f %f %f %f %d",

				&pBone->bone_id, 
				&pBone->position.x, &pBone->position.y, &pBone->position.z, 
				&pBone->angles.x, &pBone->angles.y, &pBone->angles.z,
				&pBone->parent_id);

			pBone->parent_name[0] = '\0';

			if(args != 8)
			{
				MsgError("Invalid skeleton format\n");
				delete pBone;

				goto skipskeleton;
			}

			// add bone to list
			model->bones.append(pBone);
		}

		for(int i = 0; i < numBones; i++)
		{
			int parnt = model->bones[i]->parent_id;
			if(parnt != -1)
				strcpy(model->bones[i]->parent_name, model->bones[parnt]->name);
		}
	}

skipskeleton:

	kvkeybase_t* pTrianglesSec = pSection->FindKeyBase( "triangles", KV_FLAG_SECTION );
	if(pTrianglesSec)
	{
		int numKeys = pTrianglesSec->keys.numElem();

		char curr_texture[44];
		curr_texture[0] = 0;

		dsmgroup_t* cur_group = NULL;

		int args = 0;

		for(int i = 0; i < numKeys; i++)
		{
			// if group is changes...
			if(!stricmp(pTrianglesSec->keys[i]->name, "texture"))
			{
				// if group is changed
				if(stricmp(curr_texture, KV_GetValueString(pTrianglesSec->keys[i])))
				{
					strcpy(curr_texture, KV_GetValueString(pTrianglesSec->keys[i]));

					// find or create group
					cur_group = model->FindGroupByName(curr_texture);

					if(!cur_group)
					{
						cur_group = new dsmgroup_t;
						//cur_group->usetriangleindices = false; // DSM1 doesn't uses indices

						strcpy(cur_group->texture, curr_texture);

						model->groups.append(cur_group);
					}
				}
				continue;
			}

			// if vertex adds
			if(!stricmp(pTrianglesSec->keys[i]->name, "vertex"))
			{
				dsmvertex_t newvertex;
				memset(&newvertex, 0, sizeof(dsmvertex_t));

				int nWeights = 0;
				float tempWeights[4];
				int tempWeightBones[4];

				// TODO: NEW DSM: DO NOT USE SSCANF!
				args = sscanf(
					KV_GetValueString(pTrianglesSec->keys[i]), 
					"%f " // pos x
					"%f " // pos y
					"%f " // pos z
					"%f " // nrm x
					"%f " // nrm y
					"%f " // nrm z
					"%f " // txc s
					"%f " // txc t
					"%d " // num weights
					"%d " // bone 1
					"%f " // weight 1
					"%d " // bone 2
					"%f " // weight 2
					"%d " // bone 3
					"%f " // weight 3
					"%d " // bone 4
					"%f " // weight 4
					, 

					&newvertex.position.x, &newvertex.position.y, &newvertex.position.z, 
					&newvertex.normal.x, &newvertex.normal.y, &newvertex.normal.z,
					&newvertex.texcoord.x, &newvertex.texcoord.y,

					&nWeights,
					&tempWeightBones[0], &tempWeights[0],
					&tempWeightBones[1], &tempWeights[1],
					&tempWeightBones[2], &tempWeights[2],
					&tempWeightBones[3], &tempWeights[3]
				);

				if(args < 9)
				{
					MsgError("Invalid vertex! Re-export model (%d)\n", args);
					i+=2;
					continue;
				}
				else
				{
					// HACK: ugly format
					if(nWeights == 1 && tempWeightBones[0] == -1)
					{
						nWeights = 0;
					}
					else
					{
						// sort em all
						int nNewWeights = SortAndBalanceBones(nWeights, 4, tempWeightBones, tempWeights);

						nWeights = nNewWeights;
					}

					// copy weights
					for(int j = 0; j < nWeights; j++)
					{
						dsmweight_t w;
						w.bone = tempWeightBones[j];
						w.weight = tempWeights[j];

						newvertex.weights.append(w);
					}

					cur_group->verts.append(newvertex);
				}
			}
		}
	}
	else
	{
		Msg("No triangles section found, model is empty\n");
	}

	return true;
}

bool LoadDSM(dsmmodel_t* model, const char* filename)
{
	KeyValues loader;

	if(loader.LoadFromFile(filename))
	{
		// load dsm from key-values

		kvkeybase_t *mainsec = loader.GetRootSection();

		if(mainsec)
		{
			kvkeybase_t* version_key = mainsec->FindKeyBase("version");

			if(!version_key)
				MsgWarning("DSM error: No version\n");

			int version = KV_GetValueInt(version_key);

			switch(version)
			{
				case 2:
				{
					if(!LoadDSM_Version2(model, mainsec))
					{
						return false;
					}
					break;
				}
				// ...
				default:
				{
					MsgError("DSM version %i not supported!\n   Please update SDK and plugins\n", version);
					return false;
				}
			}
		}
		else
		{
			MsgError("%s is not a DSM file\n", filename);
			return false;
		}
	}
	else
	{
		MsgError("Can't open %s\n", filename);
		return true;
	}

	return true;
}

bool SaveDSM(dsmmodel_t* model, const char* filename)
{
	

	return true;
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