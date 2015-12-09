//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef MODELLOADER_SHARED_H
#define MODELLOADER_SHARED_H

#include "model.h"
#include "ppmem.h"

studiohdr_t*			Studio_LoadModel(const char* pszPath);
studiomotiondata_t*		Studio_LoadMotionData(const char* pszPath, int numBones);

bool					Studio_LoadPhysModel(const char* pszPath, physmodeldata_t* pModel);

void					Studio_FreeMotionData(studiomotiondata_t* pData, int numBones);
void					Studio_FreePhysModel(physmodeldata_t* pModel);

#endif //MODELLOADER_SHARED_H
