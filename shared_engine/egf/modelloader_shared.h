//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef MODELLOADER_SHARED_H
#define MODELLOADER_SHARED_H

#include "egf/model.h"
#include "core/ppmem.h"

studiohdr_t*						Studio_LoadModel(const char* pszPath);
studioHwData_t::motionData_t*		Studio_LoadMotionData(const char* pszPath, int numBones);

bool								Studio_LoadPhysModel(const char* pszPath, studioPhysData_t* pModel);

void								Studio_FreeMotionData(studioHwData_t::motionData_t* pData, int numBones);
void								Studio_FreePhysModel(studioPhysData_t* pModel);

#endif //MODELLOADER_SHARED_H
