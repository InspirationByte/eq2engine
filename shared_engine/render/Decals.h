//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal geometry
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum MakeDecalFlags_e
{
	MAKEDECAL_FLAG_TEX_NORMAL		= (1 << 0),		// texture coordinates by normal
	MAKEDECAL_FLAG_CLIPPING			= (1 << 1),
};

class IMaterial;
class IPhysicsObject;

// decal make info structure
struct decalmakeinfo_t
{
	decalmakeinfo_t() : material(nullptr), flags(0), texRotation(0.0f), texScale(1.0f)
	{
	}

	IMaterial*			material;

	int					flags; // MakeDecalFlags_e

	Vector3D			origin;
	Vector3D			size;
	Vector3D			normal;

	Vector2D			texScale;
	float				texRotation;
};

enum DecalFlags_e
{
	DECAL_FLAG_VISIBLE			= (1 << 0),
	DECAL_FLAG_TRANSLUCENTSURF	= (1 << 1),	// engine supports that
	DECAL_FLAG_STUDIODECAL		= (1 << 2), // this decal is on model
};

//-------------------------------------------------------
// temporary decal itself
//-------------------------------------------------------
struct tempdecal_t
{
	tempdecal_t() 
		: verts(nullptr), indices(nullptr), numVerts(0), numIndices(0), material(nullptr)
	{
	}

	~tempdecal_t()
	{
		PPFree(verts);
		PPFree(indices);
	}

	void*				verts;
	uint16*				indices;

	uint16				numVerts;
	uint16				numIndices;

	IMaterial*			material;

	int					flags; // DecalFlags_e

	BoundingBox			bbox;
};

struct eqlevelvertex_t;

// static decal, that comes with shared VBO
struct staticdecal_t
{
	~staticdecal_t()
	{
		PPFree(pVerts);
		PPFree(pIndices);
	}

	BoundingBox			box;

	int					firstVertex;
	int					firstIndex;

	int					numVerts;
	int					numIndices;

	eqlevelvertex_t*	pVerts;
	int*				pIndices;

	IMaterial*			material;
	/*
	// the decal attachment to avoid sorting
	int					room_id[2];
	int					room_volume;
	int					surf_id;	// -1 if multiple surfaces
	*/
	int					flags; // DecalFlags_e
};
