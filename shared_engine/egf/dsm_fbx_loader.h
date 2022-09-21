//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "egf/model.h"

namespace SharedModel
{
struct dsmmodel_t;
struct esmshapedata_t;

// Loads FBX as a single model. For editor use.
bool LoadFBXCompound(dsmmodel_t* model, const char* filename);
bool LoadFBXShapes(dsmmodel_t* model, esmshapedata_t* data, const char* filename);

// loads multiple FBX geometries
bool LoadFBX(Array<dsmmodel_t*>& models, Array<esmshapedata_t*>& shapes, const char* filename);
bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename);

} // namespace
