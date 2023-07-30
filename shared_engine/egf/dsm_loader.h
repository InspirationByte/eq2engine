//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

struct DSVertex
{
	Vector3D					position{ 0.0f };
	Vector3D					normal{ 0.5f };

	Vector2D					texcoord{ 0.0f };
	FixedArray<DSWeight, 48>	weights;

	int							vertexId{ 0 };
};

struct DSGroup
{
	char			texture[256]{ 0 };
	Array<DSVertex>	verts{ PP_SL };
	Array<int>		indices{ PP_SL };
};

struct DSBone
{
	char			name[44]{ 0 };
	char			parent_name[44]{ 0 };

	int				bone_id{ -1 };
	int				parent_id{ -1 };

	Vector3D		position{ 0 };
	Vector3D		angles{ 0 };
};

struct DSModel : public RefCountedObject<DSModel>
{
	~DSModel();
	char			name[64]{ 0 };

	Array<DSGroup*>	groups{ PP_SL };
	Array<DSBone*>	bones{ PP_SL };

	DSBone*			FindBone(const char* pszName);
	DSGroup*		FindGroupByName(const char* pszGroupname);
};

//------------------------------------------------------------------------------------

int		SortAndBalanceBones(int nCount, int nMaxCount, int* bones, float* weights);

bool	LoadSharedModel(DSModel* model, const char* filename);
bool	SaveSharedModel(DSModel* model, const char* filename);

int		GetTotalVertsOfDSM(DSModel* model);

} // namespace
