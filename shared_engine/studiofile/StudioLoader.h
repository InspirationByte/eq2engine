//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

studiohdr_t*		Studio_LoadModel(const char* pszPath);
studioMotionData_t*	Studio_LoadMotionData(const char* pszPath, int numBones);
bool				Studio_LoadPhysModel(const char* pszPath, studioPhysData_t* pModel);

void				Studio_FreeModel(studiohdr_t* pModel);
void				Studio_FreeMotionData(studioMotionData_t* pData, int numBones);
void				Studio_FreePhysModel(studioPhysData_t* pModel);
