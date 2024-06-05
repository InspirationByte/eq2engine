//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace SharedModel
{

struct DSWeight
{
	int		bone{ 0 };
	float	weight{ 0.0f };
};
using DSWeightList = FixedArray<DSWeight, 48>;

struct DSVertex
{
	Vector3D		position{ 0.0f };
	Vector3D		normal{ 0.5f };
	MColor			color{ color_white };

	Vector2D		texcoord{ 0.0f };
	int				vertexId{ 0 };

	DSWeightList	weights;
};

struct DSMesh
{
	EqString		texture;
	Array<DSVertex>	verts{ PP_SL };
	Array<int>		indices{ PP_SL };
};

struct DSBone
{
	EqString		name;
	EqString		parentName;

	int				boneIdx{ -1 };
	int				parentIdx{ -1 };

	Vector3D		position{ 0 };
	Vector3D		angles{ 0 };
};

struct DSModel : public RefCountedObject<DSModel>
{
	~DSModel();
	EqString		name;

	Array<DSMesh*>	meshes{ PP_SL };
	Array<DSBone*>	bones{ PP_SL };

	DSBone*			FindBone(const char* name);
	DSMesh*			FindMeshByName(const char* name);
};
using DSModelPtr = CRefPtr<DSModel>;

struct DSAnimFrame
{
	Vector3D		position;
	Vector3D		angles;
};

struct DSBoneFrames
{
	DSAnimFrame*	keyFrames{ nullptr };
	int				numFrames{ 0 };
};

struct DSAnimData
{
	EqString		name;
	DSBoneFrames*	bones{ nullptr };
};

//------------------------------------------------------------------------------------

int		SortAndBalanceBones(int nCount, int nMaxCount, int* bones, float* weights);

bool	LoadSharedModel(DSModel* model, const char* filename);
bool	SaveSharedModel(DSModel* model, const char* filename);

int		GetTotalVertsOfDSM(DSModel* model);

} // namespace
