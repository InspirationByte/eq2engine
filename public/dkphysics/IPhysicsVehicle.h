//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics vehicle public header
//////////////////////////////////////////////////////////////////////////////////

#ifndef IPHYSICSVEHICLE_H
#define IPHYSICSVEHICLE_H

#include "DebugInterface.h"

#include "utils\DkStr.h"
#include "math\matrix.h"

#define WHEELFLAGS_DRIVE 0x1
#define WHEELFLAGS_HANDBRAKE 0x2
#define WHEELFLAGS_HANDBRAKE 0x2

struct wheelinfo_t
{
	Matrix4x4 wheelTransform;	
	int visual_bone_index;		// bone index for model

};

class IPhysicsVehicle
{
public:
	
};

#endif // IPHYSICSVEHICLE_H