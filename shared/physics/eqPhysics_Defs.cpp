//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_Defs.h"

eqPhysCollisionFilter::eqPhysCollisionFilter(const CEqRigidBody* obj)
{
	objectPtrs.append(reinterpret_cast<const void*>(obj));
}

eqPhysCollisionFilter::eqPhysCollisionFilter(ArrayCRef<CEqRigidBody> objs)
{
	objectPtrs.append((const void**)objs.ptr(), objs.numElem());
}

void eqPhysCollisionFilter::AddObject(const void* ptr)
{
	if (ptr && objectPtrs.numElem() < MAX_COLLISION_FILTER_OBJECTS)
		objectPtrs.append(ptr);
}

bool eqPhysCollisionFilter::HasObject(const void* ptr) const
{
	return arrayFindIndex(objectPtrs, ptr) != -1;
}