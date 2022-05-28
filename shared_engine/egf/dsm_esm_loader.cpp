//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ESM model loader
//////////////////////////////////////////////////////////////////////////////////

#include "dsm_esm_loader.h"
#include "core/DebugInterface.h"
#include "utils/strtools.h"

namespace SharedModel
{

bool isNotWhiteSpace(const char ch)
{
	return (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n');
}

float readFloat(Tokenizer &tok)
{
	char *str = tok.next( isNotWhiteSpace );
	//Msg("readFloat: str=%s\n", str);

	return (float)atof(str);
}

int readInt(Tokenizer &tok)
{
	char *str = tok.next( isNotWhiteSpace );
	//Msg("readInt: str=%s\n", str);

	return atoi(str);
}

bool isQuotes(const char ch)
{
	return (ch != '\"');
}

bool ReadBones(Tokenizer& tok, dsmmodel_t* pModel)
{
	char *str;

	bool bCouldRead = false;

	dsmskelbone_t* pBone = NULL;

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '{')
		{
			bCouldRead = true;
		}
		else if(str[0] == '}')
		{
			return true;
		}
		else if(bCouldRead)
		{
			// read bone definition
			pBone = PPNew dsmskelbone_t;

			char* str2 = tok.next(isQuotes);

			strcpy(pBone->name, str2);
			tok.next(); // skip quote

			pBone->bone_id = readInt(tok);

			pBone->parent_id = readInt(tok);

			pBone->parent_name[0] = '\0';

			pBone->position.x = readFloat(tok);
			pBone->position.y = readFloat(tok);
			pBone->position.z = readFloat(tok);

			pBone->angles.x = readFloat(tok);
			pBone->angles.y = readFloat(tok);
			pBone->angles.z = readFloat(tok);

			pModel->bones.append(pBone);
		}
		else
			tok.goToNextLine();
	}

	return false;
}


bool ReadFaces(Tokenizer& tok, dsmmodel_t* pModel)
{
	char *str;

	bool bCouldRead = false;

	dsmgroup_t* pCurrentGroup = NULL;
	char material_name[256];
	material_name[0] = '\0';

	//int nVertexIdAccum = 0;

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '{')
		{
			bCouldRead = true;
		}
		else if(str[0] == '}')
		{
			return true;
		}
		else if(bCouldRead)
		{
			if(!stricmp(str, "mat"))
			{
				char* matName = tok.next();

				// read tokens if we have one
				if(*matName == '\"')
					matName = tok.next(isQuotes); 

				// read and copy material name
				strcpy(material_name, matName);

				// find or create group
				pCurrentGroup = pModel->FindGroupByName(material_name);

				if(!pCurrentGroup)
				{
					pCurrentGroup = PPNew dsmgroup_t;

					strcpy(pCurrentGroup->texture, material_name);
					pCurrentGroup->verts.resize(1024);

					pModel->groups.append( pCurrentGroup );
				}
			}
			else if(!stricmp(str, "vtx"))
			{
				// parse vertex

				if(!pCurrentGroup)
				{
					MsgError("ESM parse error, the vtx is specified, but there is no material\n");
					return false;
				}

				dsmvertex_t newvertex;

				newvertex.position.x = readFloat(tok);
				newvertex.position.y = readFloat(tok);
				newvertex.position.z = readFloat(tok);

				newvertex.normal.x = readFloat(tok);
				newvertex.normal.y = readFloat(tok);
				newvertex.normal.z = readFloat(tok);

				newvertex.texcoord.x = readFloat(tok);
				newvertex.texcoord.y = readFloat(tok);

				int numWeights = readInt(tok);

				if(numWeights)
				{
					float* tempWeights = PPNew float[numWeights];
					int* tempWeightBones = PPNew int[numWeights];
					
					for(int i = 0; i < numWeights; i++)
					{
						tempWeightBones[i] = readInt(tok);
						tempWeights[i] = readFloat(tok);
					}

					// HACK: ugly format
					if(numWeights == 1 && tempWeightBones[0] == -1)
					{
						numWeights = 0;
					}
					else
					{
						// sort em all and clip.
						int nNewWeights = SortAndBalanceBones(numWeights, 4, tempWeightBones, tempWeights);

						numWeights = nNewWeights;
					}

					// copy weights
					for(int i = 0; i < numWeights; i++)
					{
						dsmweight_t w;
						w.bone = tempWeightBones[i];
						w.weight = tempWeights[i];

						newvertex.weights.append(w);
					}

					delete [] tempWeights;
					delete [] tempWeightBones;
				}

				newvertex.vertexId = -1;

				//nVertexIdAccum++;

				pCurrentGroup->verts.append(newvertex);
			}
		}
		else
			tok.goToNextLine();
	}

	return false;
}

bool ReadShapes(Tokenizer& tok, esmshapedata_t* data)
{
	char *str;

	bool bCouldRead = false;

	esmshapekey_t* curShapeKey = NULL;
	char key_name[256];
	key_name[0] = '\0';

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '{')
		{
			bCouldRead = true;
		}
		else if(str[0] == '}')
		{
			return true;
		}
		else if(bCouldRead)
		{
			if(!stricmp(str, "key"))
			{
				curShapeKey = PPNew esmshapekey_t;

				curShapeKey->time = readInt(tok);

				strcpy(key_name, tok.next());
				curShapeKey->name = key_name;
				curShapeKey->verts.resize(1024);

				data->shapes.append( curShapeKey );
			}
			else if(!stricmp(str, "vtx"))
			{
				if(!curShapeKey)
				{
					MsgError("ESX parse error, the vtx is specified, but there is no shape key specified ('key')\n");
					return false;
				}

				// parse shape vertex
				esmshapevertex_t newvertex;
				memset(&newvertex, 0, sizeof(esmshapevertex_t));

				newvertex.vertexId = readInt(tok);

				newvertex.position.x = readFloat(tok);
				newvertex.position.y = readFloat(tok);
				newvertex.position.z = readFloat(tok);

				newvertex.normal.x = readFloat(tok);
				newvertex.normal.y = readFloat(tok);
				newvertex.normal.z = readFloat(tok);

				curShapeKey->verts.append(newvertex);
			}
		}
		else
			tok.goToNextLine();
	}

	return false;
}

bool LoadESM(dsmmodel_t* model, const char* filename)
{
	ASSERT(model);

	Tokenizer tok;

	if (!tok.setFile(filename))
	{
		MsgError("Couldn't open ESM file '%s'\n", filename);
		return false;
	}

	char* str;

	// find faces section
	while ((str = tok.next()) != NULL)
	{
		if (!stricmp(str, "bones"))
		{
			if (!ReadBones(tok, model))
				return false;
		}
		else if (!stricmp(str, "faces"))
		{
			if (!ReadFaces(tok, model))
				return false;
		}

		tok.goToNextLine();
	}

	return true;
}

bool LoadESXShapes( esmshapedata_t* data, const char* filename )
{
	ASSERT(data);

	Tokenizer tok;

	if (!tok.setFile(filename))
	{
		MsgError("Couldn't open ESX file '%s'\n", filename);
		return false;
	}

	char *str;

	// find faces section
	while ((str = tok.next()) != NULL)
	{
		if(!stricmp(str, "reference"))
		{
			data->reference = tok.next(isNotWhiteSpace);
		}
		else if(!stricmp(str, "verts"))
		{
			if(!ReadShapes(tok, data))
				return false;
		}
			
		tok.goToNextLine();
	}

	return true;
}

void FreeShapes( esmshapedata_t* data )
{
	if(!data)
		return;

	for(int i = 0; i < data->shapes.numElem(); i++)
		delete data->shapes[i];
}

int FindShapeKeyIndex( esmshapedata_t* data, const char* shapeKeyName )
{
	if(!data)
		return -1;

	for(int i = 0; i < data->shapes.numElem(); i++)
	{
		if(!strcmp(data->shapes[i]->name.ToCString(), shapeKeyName))
			return i;
	}

	return -1;
}

void AssignShapeKeyVertexIndexes(dsmmodel_t* mod, esmshapedata_t* shapeData)
{
	esmshapekey_t* basis = shapeData->shapes[0];

	for(int i = 0; i < mod->groups.numElem(); i++)
	{
		dsmgroup_t* grp = mod->groups[i];

		for(int j = 0; j < grp->verts.numElem(); j++)
		{
			for(int k = 0; k < basis->verts.numElem(); k++)
			{
				if(	grp->verts[j].position == basis->verts[k].position &&
					grp->verts[j].normal == basis->verts[k].normal)
				{
					grp->verts[j].vertexId = basis->verts[k].vertexId;
				}
			}
		}
	}
}

} // namespace