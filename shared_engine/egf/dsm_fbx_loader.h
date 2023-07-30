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
struct DSModel;
struct DSShapeData;

// Loads FBX as a single model. For editor use.
bool LoadFBXCompound(DSModel* model, const char* filename);
bool LoadFBXShapes(DSModel* model, DSShapeData* data, const char* filename);

// loads multiple FBX geometries
bool LoadFBX(Array<DSModel*>& models, Array<DSShapeData*>& shapes, const char* filename);
bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename);

} // namespace
