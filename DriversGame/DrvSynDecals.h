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

struct decalsettings_t
{
	decalsettings_t()
	{
		avoidMaterialFlags = 0;
		facingDir = vec3_up;
		customClipVolume = false;
		userData = nullptr;
	}

	Vector3D	facingDir;
	Volume		clipVolume;
	int			avoidMaterialFlags;
	bool		customClipVolume;
	void*		userData;
};

typedef bool (*DECALPROCESSTRIANGLEFN)(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

bool DefaultDecalTriangleProcessFunc(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);
bool LightDecalTriangleProcessFunc(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

struct decalPrimitives_t
{
	decalPrimitives_t();

	void AddTriangle(const Vector3D& p1, const Vector3D& p2, const Vector3D& p3);

	BoundingBox				bbox;

	DkList<PFXVertex_t>		verts;
	DkList<int16>			indices;

	decalsettings_t			settings;
	DECALPROCESSTRIANGLEFN	processFunc;
};

inline bool decalVertComparator(const PFXVertex_t& a, const PFXVertex_t& b)
{
	return a.point == b.point;
}

// primitive reference in the sprite builder
struct decalPrimitivesRef_t
{
	decalPrimitivesRef_t();

	PFXVertex_t*		verts;
	int16*				indices;

	uint16				numVerts;
	uint16				numIndices;

	void*				userData;		// references decalsettings_t's userData
};

void DecalClipAndTexture(decalPrimitives_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color);
decalPrimitivesRef_t ProjectDecalToSpriteBuilder( decalPrimitives_t& emptyDecal, CSpriteBuilder<PFXVertex_t>* group, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color);

#endif // DRVSYNDECALS_H