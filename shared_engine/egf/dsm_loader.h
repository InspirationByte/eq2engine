//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace SharedModel
{

struct dsmweight_t
{
	int		bone{ 0 };
	float	weight{ 0.0f };
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
	char					texture[256]{ 0 };
	Array<dsmvertex_t>		verts{ PP_SL };
	Array<int>				indices{ PP_SL };
};

struct dsmskelbone_t
{
	char			name[44]{ 0 };
	char			parent_name[44]{ 0 };

	int				bone_id{ -1 };
	int				parent_id{ -1 };

	Vector3D		position{ 0 };
	Vector3D		angles{ 0 };
};

struct dsmmodel_t
{
	char					name[64]{ 0 };

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
