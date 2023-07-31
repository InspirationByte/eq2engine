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

	Vector2D		texcoord{ 0.0f };
	int				vertexId{ 0 };

	DSWeightList	weights;
};

struct DSGroup
{
	EqString		texture;
	Array<DSVertex>	verts{ PP_SL };
	Array<int>		indices{ PP_SL };
};

struct DSBone
{
	EqString		name;
	EqString		parent_name;

	int				bone_id{ -1 };
	int				parent_id{ -1 };

	Vector3D		position{ 0 };
	Vector3D		angles{ 0 };
};

struct DSModel : public RefCountedObject<DSModel>
{
	~DSModel();
	EqString		name;

	Array<DSGroup*>	groups{ PP_SL };
	Array<DSBone*>	bones{ PP_SL };

	DSBone*			FindBone(const char* pszName);
	DSGroup*		FindGroupByName(const char* pszGroupname);
};
using DSModelPtr = CRefPtr<DSModel>;

//------------------------------------------------------------------------------------

int		SortAndBalanceBones(int nCount, int nMaxCount, int* bones, float* weights);

bool	LoadSharedModel(DSModel* model, const char* filename);
bool	SaveSharedModel(DSModel* model, const char* filename);

int		GetTotalVertsOfDSM(DSModel* model);

} // namespace
