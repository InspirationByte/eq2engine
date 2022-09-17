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

// Loads FBX model, as DSM
bool LoadFBX(dsmmodel_t* model, const char* filename);
bool LoadFBXShapes(dsmmodel_t* model, esmshapedata_t* data, const char* filename);
bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename);

} // namespace
