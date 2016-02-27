//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model processor
//////////////////////////////////////////////////////////////////////////////////

#ifndef PHYSGEN_PROCESS_H
#define PHYSGEN_PROCESS_H

#include "core_base_header.h"
#include "dsm_loader.h"
#include "model.h"

/*
 physgen usage rules:

 use "subdivide_on_shapes" parameter in key/values file to divide model on multiple geometry infos.
 In that case model must be divided on groups, or use joints.
*/

// input parameters for model processing

struct physgenmakeparams_t
{
	char			outputname[256];

	dsmmodel_t*		pModel;
	kvkeybase_t*	phys_section;

	bool			forcegroupdivision;
};

void GeneratePhysicsModel( physgenmakeparams_t* params);

#endif