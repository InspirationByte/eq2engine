//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace SharedModel
{
struct DSModel;
using DSModelPtr = CRefPtr<DSModel>;
struct DSShapeData;
struct DSAnimData;

struct DSModelContainer
{
	DSModelPtr		model;
	DSShapeDataPtr	shapeData;
	Matrix4x4		transform{ identity4 };
};

// loads multiple FBX geometries
bool LoadFBX(Array<DSModelContainer>& modelContainerList, const char* filename);
bool LoadFBXAnimations(Array<DSAnimData>& animations, const char* filename, const char* meshFilter);

// Loads FBX as a single model. For editor use.
bool LoadFBXCompound(DSModel* model, const char* filename);

// DEPRECATED
bool LoadFBXShapes(DSModelContainer& modelContainer, const char* filename);

} // namespace
