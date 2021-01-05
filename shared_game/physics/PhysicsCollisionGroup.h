//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech physics collision group
//////////////////////////////////////////////////////////////////////////////////

#ifndef PHYSICSCOLLISIONGROUP_H
#define PHYSICSCOLLISIONGROUP_H

// Collide with all (by default)
#define	COLLISION_GROUP_ALL				(0xFFFFFFFF)

enum CollsionGroup_e
{
	// visible objects
	COLLISION_GROUP_WORLD			=	(1 << 1),	// world
	COLLISION_GROUP_ACTORS			=	(1 << 2),	// actors and npcs
	COLLISION_GROUP_PLAYER			=	(1 << 3),	// exclusively for player
	COLLISION_GROUP_OBJECTS			=	(1 << 4),	// interactive objects
	COLLISION_GROUP_RAGDOLLBONES	=	(1 << 5),	// ragdoll bones to identify the damage of actors
	COLLISION_GROUP_DEBRIS			=	(1 << 6),	// debris and ragdolls
	COLLISION_GROUP_WATER			=	(1 << 7),	// water surfaces
	COLLISION_GROUP_PROJECTILES		=	(1 << 8),	// projectile objects (grenades, etc)

	// invisible/potentially invisible
	COLLISION_GROUP_LADDER			=	(1 << 9),	// ladder surface
	COLLISION_GROUP_PLAYERCLIP		=	(1 << 10),	// player clip
	COLLISION_GROUP_NPCCLIP			=	(1 << 11),	// non-player character clip
	COLLISION_GROUP_PHYSCLIP		=	(1 << 12),	// physic objects clip
	COLLISION_GROUP_SKYBOX			=	(1 << 13),	// skybox
};

// static world
#define LAST_VISIBLE_OBJECTS			COLLISION_GROUP_PROJECTILES
#define LAST_INVISIBLE_OBJECTS			COLLISION_GROUP_SKYBOX

// what collides with
#define COLLIDE_OBJECT					(COLLISION_GROUP_WORLD | COLLISION_GROUP_ACTORS | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES | COLLISION_GROUP_PLAYER | COLLISION_GROUP_DEBRIS | COLLISION_GROUP_PHYSCLIP) // debris props
#define COLLIDE_DEBRIS					(COLLISION_GROUP_WORLD | COLLISION_GROUP_PROJECTILES | COLLISION_GROUP_DEBRIS | COLLISION_GROUP_OBJECTS) // debris props
#define COLLIDE_ACTOR					(COLLISION_GROUP_WORLD | COLLISION_GROUP_NPCCLIP | COLLISION_GROUP_ACTORS | COLLISION_GROUP_PLAYER | COLLISION_GROUP_OBJECTS) // actor (player and characters)
#define COLLIDE_PLAYER					(COLLISION_GROUP_WORLD | COLLISION_GROUP_PLAYERCLIP | COLLISION_GROUP_ACTORS | COLLISION_GROUP_PLAYER | COLLISION_GROUP_OBJECTS) // player props
#define COLLIDE_PROJECTILES				(COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYER | COLLISION_GROUP_DEBRIS | COLLISION_GROUP_PROJECTILES | COLLISION_GROUP_RAGDOLLBONES | COLLISION_GROUP_WATER) // projectiles collides with world

#endif // PHYSICSCOLLISIONGROUP_H