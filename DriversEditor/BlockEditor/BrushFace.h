//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#ifndef BRUSHFACE_H
#define BRUSHFACE_H

#include "math/Vector.h"
#include "math/Plane.h"

enum EBrushFaceFlags
{
	BRUSH_FACE_SELECTED = (1 << 0),
};

class IMaterial;

struct brushFace_t
{
	brushFace_t()
	{
		nFlags = 0;
	}

	// texture matrix
	Plane							UAxis;		// tangent plane
	Plane							VAxis;		// binormal plane
	Plane							Plane;		// normal plane

	Vector2D						vScale;		// texture scale

	float							fRotation;	// texture rotation

	// flags
	int								nFlags;

	// face material
	IMaterial*						pMaterial;

	// smoothing group index
	ubyte							nSmoothingGroup;
};

#endif // BRUSHFACE_H