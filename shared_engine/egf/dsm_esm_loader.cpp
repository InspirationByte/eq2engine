//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ESM model loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/Tokenizer.h"
#include "dsm_esm_loader.h"
#include "dsm_loader.h"
#include "egf/studiomodel.h"

namespace SharedModel
{

bool isNotWhiteSpace(const char ch)
{
	return (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n');
}

float readFloat(Tokenizer &tok)
{
	const char *str = tok.next( isNotWhiteSpace );
	return (float)atof(str);
}

int readInt(Tokenizer &tok)
{
	const char *str = tok.next( isNotWhiteSpace );
	return atoi(str);
}

bool isQuotes(const char ch)
{
	return (ch != '\"');
}

bool ReadBones(Tokenizer& tok, DSModel* pModel)
{
	char *str;

	bool bCouldRead = false;

	DSBone* pBone = nullptr;

	while ((str = tok.next()) != nullptr)
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
			pBone = PPNew DSBone;

			pBone->name = tok.next(isQuotes);
			tok.next(); // skip quote

			pBone->boneIdx = readInt(tok);
			pBone->parentIdx = readInt(tok);

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


bool ReadFaces(Tokenizer& tok, DSModel* pModel)
{
	char *str;

	bool bCouldRead = false;

	DSMesh* pCurrentGroup = nullptr;
	while ((str = tok.next()) != nullptr)
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

				// find or create group
				pCurrentGroup = pModel->FindMeshByName(matName);

				if(!pCurrentGroup)
				{
					pCurrentGroup = PPNew DSMesh;

					pCurrentGroup->texture = matName;
					pCurrentGroup->verts.resize(1024);

					pModel->meshes.append( pCurrentGroup );
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

				DSVertex newvertex;

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
					Array<float> tempWeights(PP_SL);
					Array<int> tempWeightBones(PP_SL);

					for(int i = 0; i < numWeights; i++)
					{
						tempWeightBones.append(readInt(tok));
						tempWeights.append(readFloat(tok));
					}

					if (numWeights > MAX_MODEL_VERTEX_WEIGHTS)
					{
						// HACK: ugly format
						if (numWeights == 1 && tempWeightBones[0] == -1)
						{
							numWeights = 0;
						}
						else
						{
							// sort em all and clip.
							numWeights = SortAndBalanceBones(numWeights, MAX_MODEL_VERTEX_WEIGHTS, tempWeightBones.ptr(), tempWeights.ptr());
						}

						for (int i = 0; i < numWeights; i++)
						{
							DSWeight& weight = newvertex.weights.append();
							weight.bone = tempWeightBones[i];
							weight.weight = tempWeights[i];
						}
					}
					else
					{
						// copy weights
						for (int i = 0; i < numWeights; i++)
						{
							DSWeight& weight = newvertex.weights.append();
							weight.bone = tempWeightBones[i];
							weight.weight = tempWeights[i];
						}
					}
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

bool ReadShapes(Tokenizer& tok, DSShapeData* data)
{
	char *str;

	bool bCouldRead = false;

	DSShapeKey* curShapeKey = nullptr;
	char key_name[256];
	key_name[0] = '\0';

	while ((str = tok.next()) != nullptr)
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
				curShapeKey = PPNew DSShapeKey;

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
				DSShapeVert newvertex;
				memset(&newvertex, 0, sizeof(DSShapeVert));

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

bool LoadESM(DSModel* model, const char* filename)
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
	while ((str = tok.next()) != nullptr)
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

bool LoadESXShapes( DSShapeData* data, const char* filename )
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
	while ((str = tok.next()) != nullptr)
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

DSShapeData::~DSShapeData()
{
	for(int i = 0; i < shapes.numElem(); i++)
		delete shapes[i];
}

int FindShapeKeyIndex( DSShapeData* data, const char* shapeKeyName )
{
	if(!data)
		return -1;

	for(int i = 0; i < data->shapes.numElem(); i++)
	{
		if(!stricmp(data->shapes[i]->name.ToCString(), shapeKeyName))
			return i;
	}

	return -1;
}

void AssignShapeKeyVertexIndexes(DSModel* mod, DSShapeData* shapeData)
{
	if (shapeData->shapes.numElem() <= 1)
		return;

	Msg("Assigning vertex indiexes to shape keys\n");

	DSShapeKey* basis = shapeData->shapes[0];

	for(int i = 0; i < mod->meshes.numElem(); i++)
	{
		DSMesh* grp = mod->meshes[i];

		for(int j = 0; j < grp->verts.numElem(); j++)
		{
			DSVertex& grpVert = grp->verts[j];
			for(int k = 0; k < basis->verts.numElem(); k++)
			{
				const DSShapeVert& basisVert = basis->verts[k];

				if(grpVert.position == basisVert.position &&
					grpVert.normal == basisVert.normal)
				{
					grpVert.vertexId = basisVert.vertexId;
				}
			}
		}
	}
}

} // namespace