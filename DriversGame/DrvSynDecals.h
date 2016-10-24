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

struct decalprimitives_t
{
	decalprimitives_t()
	{
		avoidMaterialFlags = MATERIAL_FLAG_RECEIVESHADOWS;
	}

	DkList<PFXVertex_t>	verts;
	DkList<int16>		indices;
	int					avoidMaterialFlags;
};

inline bool decalVertComparator(const PFXVertex_t& a, const PFXVertex_t& b)
{
	return a.point == b.point;
}

void DecalClipAndTexture(decalprimitives_t* decal, const Volume& clipVolume, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, float alpha = 1.0f);

#endif // DRVSYNDECALS_H