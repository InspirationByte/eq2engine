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
	}

	Vector3D	facingDir;
	Volume		clipVolume;
	int			avoidMaterialFlags;
	bool		customClipVolume;

};

typedef bool (*DECALPROCESSTRIANGLEFN)(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

bool DefaultDecalTriangleProcessFunc(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);
bool LightDecalTriangleProcessFunc(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3);

struct decalprimitives_t
{
	decalprimitives_t();

	void AddTriangle(const Vector3D& p1, const Vector3D& p2, const Vector3D& p3);

	DkList<PFXVertex_t>		verts;
	DkList<int16>			indices;

	decalsettings_t			settings;
	DECALPROCESSTRIANGLEFN	processFunc;
};

inline bool decalVertComparator(const PFXVertex_t& a, const PFXVertex_t& b)
{
	return a.point == b.point;
}

void DecalClipAndTexture(decalprimitives_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color);
void ProjectDecalToSpriteBuilder( decalprimitives_t& emptyDecal, CSpriteBuilder<PFXVertex_t>* group, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color);

#endif // DRVSYNDECALS_H