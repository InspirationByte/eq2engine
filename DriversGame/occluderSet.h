//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: occluder set
//////////////////////////////////////////////////////////////////////////////////

#ifndef OCCLUDERSET_H
#define OCCLUDERSET_H

#include "math/DkMath.h"
#include "utils/DkList.h"
#include "ppmem.h"

#include "levfile.h"

class CLevelRegion;

struct occludingVolume_t
{
	PPMEM_MANAGED_OBJECT()

	occludingVolume_t() {}
	occludingVolume_t(const Vector3D& cameraPos, CLevelRegion* srcReg, levOccluderLine_t* srcOcc);

	// check funcs
	bool IsBeingOccluded( const BoundingBox& bbox, float eps) const;
	bool IsBeingOccluded( const Vector3D& pos, float radius) const;

	Plane planes[4]; // near, top, left, right

#ifdef EDITOR
	levOccluderLine_t*	sourceOccluder;
	CLevelRegion*		sourceRegion;
#endif // EDITOR
};

// occluding frustum
struct occludingFrustum_t
{
	occludingFrustum_t() : occluderSets(32), bypass(false)
	{
	}

	~occludingFrustum_t()
	{
		Clear();
	}

	void Clear()
	{
		for(int i = 0; i < occluderSets.numElem(); i++)
			delete occluderSets[i];

		occluderSets.clear(false);
	}

	bool IsBoxVisible( const BoundingBox& bbox) const;
	bool IsSphereVisible( const Vector3D& pos, float radius ) const;

	bool IsBoxVisibleThruOcc( const BoundingBox& bbox) const;
	bool IsSphereVisibleThruOcc( const Vector3D& pos, float radius ) const;

	Volume						frustum;
	DkList<occludingVolume_t*>	occluderSets;
	bool						bypass;
};

#endif // OCCLUDERSET_H