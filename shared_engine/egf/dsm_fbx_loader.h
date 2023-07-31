//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "egf/model.h"

namespace SharedModel
{
struct DSModel;
using DSModelPtr = CRefPtr<DSModel>;
struct DSShapeData;

struct DSModelContainer
{
	DSModelPtr		model;
	DSShapeDataPtr	shapeData;
	Matrix4x4		transform{ identity4 };
};

// loads multiple FBX geometries
bool LoadFBX(Array<DSModelContainer>& modelContainerList, const char* filename);
bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename);

// Loads FBX as a single model. For editor use.
bool LoadFBXCompound(DSModel* model, const char* filename);

// DEPRECATED
bool LoadFBXShapes(DSModelContainer& modelContainer, const char* filename);

} // namespace
