//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Contact pair and properties
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqCollisionObject;

enum ECollPairFlag
{
	COLLPAIRFLAG_NO_SOUND = (1 << 0),
	COLLPAIRFLAG_OBJECTA_STATIC = (1 << 1),

	COLLPAIRFLAG_OBJECTA_NO_RESPONSE = (1 << 2),
	COLLPAIRFLAG_OBJECTB_NO_RESPONSE = (1 << 3),

	COLLPAIRFLAG_NO_RESPONSE = (COLLPAIRFLAG_OBJECTA_NO_RESPONSE | COLLPAIRFLAG_OBJECTB_NO_RESPONSE),

	COLLPAIRFLAG_CONCAVE = (1 << 4),

	// other flags
	COLLPAIRFLAG_USER_PROCESSED = (1 << 16),		// special flag for user needs
	COLLPAIRFLAG_USER_PROCESSED2 = (1 << 17),		// special flag for user needs
	COLLPAIRFLAG_USER_PROCESSED3 = (1 << 18),		// special flag for user needs
	COLLPAIRFLAG_USER_PROCESSED4 = (1 << 19),		// special flag for user needs
};

struct CollisionData_t
{
	CollisionData_t()
	{
		fract = 1.0f;
		hitobject = nullptr;
		materialIndex = -1;
		pad = 0;
	}

	FVector3D			position;			// position in world
	Vector3D			normal;

	CEqCollisionObject* hitobject;

	float				fract;				// collision depth (if RayTest or SweepTest - factor between start[Transform] and end[Transform])
	int					materialIndex;
	int					pad;
};

struct ContactPair_t
{
	CEqCollisionObject* GetOppositeTo(CEqCollisionObject* obj) const;

	Vector3D			normal;
	FVector3D			position;

	CEqCollisionObject* bodyA;
	CEqCollisionObject* bodyB;

	float				restitutionA;
	float				restitutionB;

	float				frictionA;
	float				frictionB;

	float				dt;
	float				depth;

	int					flags;
};

struct CollisionPairData_t
{
	CEqCollisionObject* GetOppositeTo(CEqCollisionObject* obj) const;

	FVector3D			position;			// position in world
	Vector3D			normal;

	CEqCollisionObject* bodyA;
	CEqCollisionObject* bodyB;

	float				fract;				// collision depth (if RayTest or SweepTest - factor between start[Transform] and end[Transform])
	float				appliedImpulse;
	float				impactVelocity;
	int					flags{ 0 };
};