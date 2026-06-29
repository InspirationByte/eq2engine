//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqRigidBody;

// max world size is +/-32768, limited by FReal
static constexpr const float EQPHYS_MAX_WORLDSIZE = 32767.0f;

static constexpr const float PHYSICS_DEFAULT_FRICTION = 0.5f;
static constexpr const float PHYSICS_DEFAULT_RESTITUTION = 0.25f;
static constexpr const float PHYSICS_DEFAULT_TIRE_FRICTION = 0.2f;
static constexpr const float PHYSICS_DEFAULT_TIRE_TRACTION = 1.0f;

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
	int			collideMask{ -1 };
	int			contents{ 0 };

	float		friction{ PHYSICS_DEFAULT_FRICTION };
	float		restitution{ PHYSICS_DEFAULT_RESTITUTION };

	float		tireFriction{ PHYSICS_DEFAULT_TIRE_FRICTION };
	float		tireTraction{ PHYSICS_DEFAULT_TIRE_TRACTION };

	char		word{ 'C' };
	char		_zero{ 0 };
};