//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate decal generator code
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRVSYNDECALS_H
#define DRVSYNDECALS_H

#include "dktypes.h"
#include "utils/DkList.h"
#include "IMaterialSystem.h"
#include "EqParticles.h"

typedef bool(*DECALPROCESSTRIANGLEFN)(struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

bool DefaultDecalTriangleProcessFunc(struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);
bool LightDecalTriangleProcessFunc(struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

struct decalSettings_t
{
	decalSettings_t();

	DECALPROCESSTRIANGLEFN	processFunc;

	Vector3D	facingDir;

	Volume		clipVolume;
	bool		customClipVolume;

	bool		skipTexCoords;

	int			avoidMaterialFlags;
	
	void*		userData;
};


struct decalPrimitives_t
{
	decalPrimitives_t();

	void Clear();

	void AddTriangle(const Vector3D& p1, const Vector3D& p2, const Vector3D& p3);

	BoundingBox				bbox;

	bool					dirty;

	DkList<PFXVertex_t>		verts;
	DkList<int16>			indices;

	decalSettings_t			settings;
};

inline bool decalVertComparator(const PFXVertex_t& a, const PFXVertex_t& b)
{
	return a.point == b.point;
}

//-----------------------------------------

// primitive reference in the sprite builder
struct decalPrimitivesRef_t
{
	decalPrimitivesRef_t();

	PFXVertex_t*		verts;
	int16*				indices;

	uint16				numVerts;
	uint16				numIndices;

	void*				userData;		// references decalSettings_t's userData
};

void DecalTexture(decalPrimitives_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color = color4_white);
void DecalTexture(decalPrimitivesRef_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color = color4_white);

// makes a decal
decalPrimitivesRef_t ProjectDecalToSpriteBuilder( decalPrimitives_t& targetDecal, CSpriteBuilder<PFXVertex_t>* targetBuilder, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color = color4_white);
void ProjectDecalToSpriteBuilderAddJob(const decalSettings_t& settings, CSpriteBuilder<PFXVertex_t>* group, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color);

#endif // DRVSYNDECALS_H