//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace SharedModel
{

struct dsmweight_t
{
	int		bone;
	float	weight;
};

struct dsmvertex_t
{
	Vector3D			position{ 0.0f };
	Vector3D			normal{ 0.5f };

	Vector2D			texcoord{ 0.0f };

	//int				numWeights; // 0 means no bone connected
	Array<dsmweight_t>	weights{ PP_SL };

	int					vertexId{ 0 };
};

struct dsmgroup_t
{
	dsmgroup_t()
	{
		texture[0] = '\0';
	};

	char					texture[256];
	Array<dsmvertex_t>		verts{ PP_SL };
	Array<int>				indices{ PP_SL };
};

struct dsmskelbone_t
{
	char			name[44];
	char			parent_name[44];

	int				bone_id;
	int				parent_id;

	Vector3D		position;
	Vector3D		angles;
};

struct dsmmodel_t
{
	char					name[64];

	Array<dsmgroup_t*>		groups{ PP_SL };
	Array<dsmskelbone_t*>	bones{ PP_SL };

	dsmskelbone_t*			FindBone(const char* pszName);
	dsmgroup_t*				FindGroupByName(const char* pszGroupname);
};

//------------------------------------------------------------------------------------

int		SortAndBalanceBones(int nCount, int nMaxCount, int* bones, float* weights);

bool	LoadSharedModel(dsmmodel_t* model, const char* filename);
bool	SaveSharedModel(dsmmodel_t* model, const char* filename);

void	FreeDSM(dsmmodel_t* model);
void	FreeDSMBones(dsmmodel_t* model);

int		GetTotalVertsOfDSM(dsmmodel_t* model);

} // namespace
