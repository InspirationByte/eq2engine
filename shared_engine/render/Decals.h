//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal geometry
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EDecalMakeFlags
{
	DECAL_MAKE_FLAG_TEX_NORMAL	= (1 << 0),		// texture coordinates by normal
	DECAL_MAKE_FLAG_CLIPPING	= (1 << 1),		// not supported on EGFs
};

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

// decal make info structure
struct DecalMakeInfo
{
	Vector3D			origin;
	Vector3D			size;
	Vector3D			normal;

	Vector2D			texScale{ 1.0f };
	float				texRotation{ 0.0f };
	int					flags{ 0 };			// EDecalMakeFlags

	IMaterialPtr		material;
};

enum EDecalDataFlags
{
	DECAL_FLAG_STUDIODECAL = (1 << 0)
};

struct DecalData : public RefCountedObject<DecalData>
{
	~DecalData()
	{
		PPFree(verts);
		PPFree(indices);
	}

	BoundingBox			bbox;
	IMaterialPtr		material;
	void*				verts{ nullptr };
	uint16*				indices{ nullptr };
	
	uint16				numVerts{ 0 };
	uint16				numIndices{ 0 };

	int					flags{ 0 };		// EDecalDataFlags
};
