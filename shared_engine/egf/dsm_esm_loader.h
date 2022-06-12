//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once
class Tokenizer;

namespace SharedModel
{

struct dsmmodel_t;

struct esmshapevertex_t
{
	Vector3D	position;
	Vector3D	normal;

	int			vertexId;
};

struct esmshapekey_t
{
	Array<esmshapevertex_t>		verts{ PP_SL };
	EqString					name;
	int							time;
};

struct esmshapedata_t
{
	Array<esmshapekey_t*>	shapes{ PP_SL };
	EqString				reference;
	int						time;
};


bool	ReadBones(Tokenizer& tok, dsmmodel_t* pModel);

void	FreeShapes( esmshapedata_t* data );
int		FindShapeKeyIndex( esmshapedata_t* data, const char* shapeKeyName );
void	AssignShapeKeyVertexIndexes(dsmmodel_t* mod, esmshapedata_t* shapeData);

bool	LoadESM(dsmmodel_t* model, const char* filename);
bool	LoadESXShapes(esmshapedata_t* data, const char* filename);

} // namespace
