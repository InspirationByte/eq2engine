//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqRigidBody;

static constexpr const int COLLISION_MASK_ALL = 0xFFFFFFFF;
static constexpr const int MAX_COLLISION_FILTER_OBJECTS = 8;

enum EPhysFilterType
{
	EQPHYS_FILTER_TYPE_EXCLUDE = 0,		// excludes objects
	EQPHYS_FILTER_TYPE_INCLUDE_ONLY,	// includes only objects
};

enum EPhysFilterFlags
{
	EQPHYS_FILTER_FLAG_STATICOBJECTS	= (1 << 0),	// filters static objects
	EQPHYS_FILTER_FLAG_DYNAMICOBJECTS	= (1 << 1),	// filters dynamic objects
	EQPHYS_FILTER_FLAG_BY_USERDATA		= (1 << 2),	// filter uses userdata comparison instead of objects

	EQPHYS_FILTER_FLAG_FORCE_RAYCAST	= (1 << 5), // for raycasting - ignores COLLOBJ_NO_RAYCAST flags
};

struct eqPhysCollisionFilter
{
	eqPhysCollisionFilter() = default;
	eqPhysCollisionFilter(const CEqRigidBody* obj);
	eqPhysCollisionFilter(ArrayCRef<CEqRigidBody> objs);

	void AddObject(const void* ptr);
	bool HasObject(const void* ptr) const;

	FixedArray<const void*, MAX_COLLISION_FILTER_OBJECTS>	objectPtrs;

	EPhysFilterType		type{ EQPHYS_FILTER_TYPE_EXCLUDE };
	int					flags{ EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS };
	int					ignoreContentsMask{ 0 };
};

struct eqPhysSurfParam
{
	EqString	name;

	int			id{ -1 };
	int			contentsMask{ -1 };	// this mask is applied on top of the object contents and collision mask

	float		restitution{ 0.0f };
	float		friction{ 0.0f };

	float		tirefriction{ 1.0f };
	float		tirefriction_traction{ 1.0f };

	char		word{ 0 };
};