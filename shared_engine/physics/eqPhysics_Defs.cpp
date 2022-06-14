//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_Defs.h"

eqPhysCollisionFilter::eqPhysCollisionFilter()
{
	type = EQPHYS_FILTER_TYPE_EXCLUDE;
	flags = 0;
	numObjects = 0;
}

eqPhysCollisionFilter::eqPhysCollisionFilter(CEqRigidBody* obj)
{
	objectPtrs[0] = obj;
	numObjects = 1;

	type = EQPHYS_FILTER_TYPE_EXCLUDE;
	flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
}

eqPhysCollisionFilter::eqPhysCollisionFilter(CEqRigidBody** obj, int cnt)
{
	int cpcnt = sizeof(CEqRigidBody*) * cnt;
	cpcnt = min(cpcnt, MAX_COLLISION_FILTER_OBJECTS);

	memcpy(objectPtrs, obj, cpcnt);
	numObjects = cpcnt;

	type = EQPHYS_FILTER_TYPE_EXCLUDE;
	flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
}

void eqPhysCollisionFilter::AddObject(void* ptr)
{
	if (ptr && numObjects < MAX_COLLISION_FILTER_OBJECTS)
		objectPtrs[numObjects++] = ptr;
}

bool eqPhysCollisionFilter::HasObject(void* ptr) const
{
	for (int i = 0; i < numObjects; i++)
	{
		if (objectPtrs[i] == ptr)
			return true;
	}

	return false;
}