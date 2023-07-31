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
struct DSModel;

struct DSShapeVert
{
	Vector3D	position;
	Vector3D	normal;

	int			vertexId{ -1 };
};

struct DSShapeKey
{
	Array<DSShapeVert>	verts{ PP_SL };
	EqString			name;
	int					time{ 0 };
};

struct DSShapeData : RefCountedObject<DSShapeData>
{
	~DSShapeData();

	Array<DSShapeKey*>	shapes{ PP_SL };
	EqString			reference;
};
using DSShapeDataPtr = CRefPtr<DSShapeData>;

bool	ReadBones(Tokenizer& tok, DSModel* pModel);

int		FindShapeKeyIndex( DSShapeData* data, const char* shapeKeyName );
void	AssignShapeKeyVertexIndexes(DSModel* mod, DSShapeData* shapeData);

bool	LoadESM(DSModel* model, const char* filename);
bool	LoadESXShapes(DSShapeData* data, const char* filename);

} // namespace
