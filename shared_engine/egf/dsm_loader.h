//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef DSM_LOADER_H
#define DSM_LOADER_H

#include "math/Vector.h"
#include "ds/DkList.h"
#include "ds/eqstring.h"
#include "core/ppmem.h"

namespace SharedModel
{

struct dsmweight_t
{
	int		bone;
	float	weight;
};

struct dsmvertex_t
{
	dsmvertex_t()
	{
		normal = Vector3D(0,1,0);
		texcoord = vec2_zero;
	}

	Vector3D			position;
	Vector3D			normal;

	Vector2D			texcoord;

	//int				numWeights; // 0 means no bone connected
	DkList<dsmweight_t>	weights;

	int					vertexId;
};

struct dsmgroup_t
{
	dsmgroup_t()
	{
		texture[0] = '\0';
	};

	char					texture[256];
	DkList<dsmvertex_t>		verts;
	DkList<int>				indices;
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

	DkList<dsmgroup_t*>		groups;

	DkList<dsmskelbone_t*>	bones;

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

#endif // DSM_LOADER_H
