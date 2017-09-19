//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal geometry
//////////////////////////////////////////////////////////////////////////////////

#ifndef DECALS_H
#define DECALS_H

#include "eqlevel.h"
#include "model.h"

#include "math/BoundingBox.h"
#include "ppmem.h"

enum MakeDecalFlags_e
{
	DECALFLAG_TEXCOORD_BYNORMAL	= (1 << 0),	// texture coordinates by normal
	DECALFLAG_CULLING			= (1 << 1),
};

class IMaterial;
class IPhysicsObject;

// decal make info structure
struct decalmakeinfo_t
{
	decalmakeinfo_t()
	{
		flags		= 0;
		texRotation = 0.0f;
		texScale	= Vector2D(1,1);
		pMaterial	= NULL;
	}

	IMaterial*			pMaterial;

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

class IMaterial;

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

	IMaterial*			pMaterial;
	/*
	// the decal attachment to avoid sorting
	int					room_id[2];
	int					room_volume;
	int					surf_id;	// -1 if multiple surfaces
	*/
	int					flags; // DecalFlags_e
};

// temp decal
struct tempdecal_t
{
	PPMEM_MANAGED_OBJECT();

	~tempdecal_t()
	{
		PPFree(pVerts);
		PPFree(pIndices);
	}

	BoundingBox			box;

	eqlevelvertex_t*	pVerts;
	uint16*				pIndices;

	int					numVerts;
	int					numIndices;

	IMaterial*			pMaterial;
	/*
	// the decal attachment to avoid sorting
	int					room_id[2];
	int					room_volume;
	int					surf_id;	// -1 if multiple surfaces
	*/
	int					flags; // DecalFlags_e
};

// studio temp decal
// FACTS:
//		- it's software-skinned
//		- doesn't need bbox
//		- back-transformed
struct studiotempdecal_t
{
	PPMEM_MANAGED_OBJECT();

	~studiotempdecal_t()
	{
		PPFree(pVerts);
		PPFree(pIndices);
	}

	EGFHwVertex_t*		pVerts;
	uint16*				pIndices;

	int					numVerts;
	int					numIndices;

	IMaterial*			pMaterial;
	/*
	// the decal attachment to avoid sorting
	int					room_id[2];
	int					room_volume;
	int					surf_id;	// -1 if multiple surfaces
	*/
	int					flags; // DecalFlags_e
};

#endif // DECALS_H
