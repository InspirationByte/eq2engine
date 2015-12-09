//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#ifndef DSM_ESM_LOADER_H
#define DSM_ESM_LOADER_H

#include "dsm_loader.h"
#include "utils/Tokenizer.h"

struct esmshapevertex_t
{
	Vector3D	position;
	Vector3D	normal;

	int			vertexId;
};

struct esmshapekey_t
{
	DkList<esmshapevertex_t>	verts;
	EqString					name;
	int							time;
};

struct esmshapedata_t
{
	DkList<esmshapekey_t*>	shapes;
	EqString				reference;
	int						time;
};

bool ReadBones(Tokenizer& tok, dsmmodel_t* pModel);
bool ReadFaces(Tokenizer& tok, dsmmodel_t* pModel);

bool LoadESXShapes( esmshapedata_t* data, const char* filename );
void FreeShapes( esmshapedata_t* data );

int FindShapeKeyIndex( esmshapedata_t* data, const char* shapeKeyName );

void AssignShapeKeyVertexIndexes(dsmmodel_t* mod, esmshapedata_t* shapeData);

int readInt(Tokenizer &tok);
float readFloat(Tokenizer &tok);

#endif // DSM_ESM_LOADER_H
