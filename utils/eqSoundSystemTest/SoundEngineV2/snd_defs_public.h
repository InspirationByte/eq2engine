//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine types
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_DEFS_PUBLIC_H
#define SND_DEFS_PUBLIC_H

#include "dktypes.h"
#include "math/Vector.h"
#include "math/Matrix.h"

enum EChanSpatializeMethod
{
	SPATIAL_METHOD_NONE = 0,

	SPATIAL_METHOD_MONO,
	SPATIAL_METHOD_STEREO,

	SPATIAL_METHOD_MONO_CHANNEL_ATTEN	// method where left channel used as closest source and right as far source
};

struct ListenerInfo_t {
	Vector3D	origin;
	Matrix3x3	orient;
};

#endif // SND_DEFS_PUBLIC_H