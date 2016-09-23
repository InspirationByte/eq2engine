//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Model Script compilation
//////////////////////////////////////////////////////////////////////////////////

#ifndef SCRIPTCOMPILE_H
#define SCRIPTCOMPILE_H

#include "dsm_loader.h"
#include "dsm_obj_loader.h"
#include "model.h"

struct cbone_t
{
	dsmskelbone_t*		referencebone;

	DkList<cbone_t*>	childs;
	cbone_t*			parent;
};

struct ciklink_t
{
	Vector3D			mins;
	Vector3D			maxs;

	cbone_t*			bone;

	float				damping;
};

struct ikchain_t
{
	char name[44];

	DkList<ciklink_t> link_list;
};

struct clodmodel_t
{
	dsmmodel_t*			lodmodels[MAX_MODELLODS];
};

struct esmshapedata_t;

struct egfcamodel_t
{
	egfcamodel_t()
	{
		model = NULL;
		shapeData = NULL;
		shapeBy = -1;
	}

	dsmmodel_t*			model;
	esmshapedata_t*		shapeData;

	int					shapeBy;	// shape index
};

void FreeEGFCaModel( egfcamodel_t& mod );

int GetMaterialIndex(const char* pszName);
int GetReferenceIndex(dsmmodel_t* pRef);

bool CompileESCScript(const char* name);

#endif // SCRIPTCOMPILE_H