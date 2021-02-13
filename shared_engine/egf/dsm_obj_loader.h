//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader, obj support
//////////////////////////////////////////////////////////////////////////////////

#ifndef DSM_OBJ_LOADER_H
#define DSM_OBJ_LOADER_H

#include "dsm_loader.h"

// Loads OBJ model, as DSM
bool LoadOBJ(dsmmodel_t* model, const char* filename);
bool SaveOBJ(dsmmodel_t* model, const char* filename);

#endif // DSM_OBJ_LOADER_H