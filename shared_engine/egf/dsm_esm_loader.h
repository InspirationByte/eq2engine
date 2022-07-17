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

bool isNotWhiteSpace(const char ch);
float readFloat(Tokenizer& tok);

struct dsmmodel_t;

struct esmshapevertex_t
{
	Vector3D	position;
	Vector3D	normal;

	int			vertexId{ -1 };
};

struct esmshapekey_t
{
	Array<esmshapevertex_t>		verts{ PP_SL };
	EqString					name;
	int							time{ 0 };
};

struct esmshapedata_t
{
	Array<esmshapekey_t*>	shapes{ PP_SL };
	EqString				reference;
};


bool	ReadBones(Tokenizer& tok, dsmmodel_t* pModel);

void	FreeShapes( esmshapedata_t* data );
int		FindShapeKeyIndex( esmshapedata_t* data, const char* shapeKeyName );
void	AssignShapeKeyVertexIndexes(dsmmodel_t* mod, esmshapedata_t* shapeData);

bool	LoadESM(dsmmodel_t* model, const char* filename);
bool	LoadESXShapes(esmshapedata_t* data, const char* filename);

} // namespace
