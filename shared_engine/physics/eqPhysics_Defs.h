//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqRigidBody;

enum EPhysFilterType
{
	EQPHYS_FILTER_TYPE_EXCLUDE = 0,		// excludes objects
	EQPHYS_FILTER_TYPE_INCLUDE_ONLY,		// includes only objects
};

enum EPhysFilterFlags
{
	EQPHYS_FILTER_FLAG_STATICOBJECTS = (1 << 0),	// filters only static objects
	EQPHYS_FILTER_FLAG_DYNAMICOBJECTS = (1 << 1), // filters only dynamic objects
	EQPHYS_FILTER_FLAG_CHECKUSERDATA = (1 << 2), // filter uses userdata comparison instead of objects

	EQPHYS_FILTER_FLAG_DISALLOW_STATIC = (1 << 3),
	EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC = (1 << 4),

	EQPHYS_FILTER_FLAG_FORCE_RAYCAST = (1 << 5), // for raycasting - ignores COLLOBJ_NO_RAYCAST flags
};

static constexpr const int COLLISION_MASK_ALL = 0xFFFFFFFF;

#define MAX_COLLISION_FILTER_OBJECTS 8

struct eqPhysCollisionFilter
{
	eqPhysCollisionFilter();
	eqPhysCollisionFilter(CEqRigidBody* obj);
	eqPhysCollisionFilter(CEqRigidBody** obj, int cnt);

	void AddObject(void* ptr);
	bool HasObject(void* ptr) const;

	void*	objectPtrs[MAX_COLLISION_FILTER_OBJECTS];

	int		type;
	int		flags;
	int		numObjects;
};