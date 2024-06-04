//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

studioHdr_t*		Studio_LoadModel(const char* pszPath);
StudioMotionData*	Studio_LoadMotionData(const char* pszPath, int numBones);
bool				Studio_LoadPhysModel(const char* pszPath, StudioPhysData* pModel);

void				Studio_FreeModel(studioHdr_t* pModel);
void				Studio_FreeMotionData(StudioMotionData* pData, int numBones);
void				Studio_FreePhysModel(StudioPhysData* pModel);
