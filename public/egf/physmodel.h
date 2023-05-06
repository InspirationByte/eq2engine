//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics data Format
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define PHYSFILE_VERSION	2
#define PHYSFILE_ID			MCHAR4('P','O','D','I')

static constexpr const int MAX_PHYS_NAME_LENGTH			= 64;
static constexpr const int MAX_PHYS_SURF_NAME_LENGTH	= 64;
static constexpr const int MAX_PHYS_COMMENT_STRING		= 256;

static constexpr const int MAX_PHYS_GEOM_PER_OBJECT		= 32;			// maximum shapes allowed to be in one physics object
static constexpr const float PHYS_DEFAULT_MASS			= (5.0f);

enum EPhysModelUsage
{
	PHYSMODEL_USAGE_INVALID	= 0,	// invalid usage
	PHYSMODEL_USAGE_RIGID_COMP,		// standard rigid body with/without compoudness, static or non-static
	PHYSMODEL_USAGE_RAGDOLL,		// ragdoll model
	PHYSMODEL_USAGE_DYNAMIC,		// dynamic model, that use joints, but for animation such as non-standard doors, etc.
};

enum EPhysLump
{
	PHYSFILE_PROPERTIES		= 0,	// shared model property lump
	PHYSFILE_GEOMETRYINFO	= 1,	// geometrical info lump
	PHYSFILE_JOINTDATA		= 2,	// joint data
	PHYSFILE_OBJECTS		= 3,	// objects in this model
	PHYSFILE_VERTEXDATA		= 4,	// vertex data, Vector3D format
	PHYSFILE_INDEXDATA		= 5,	// vertex indices data, int format
	PHYSFILE_OBJECTNAMES	= 6,	// object names lump

	PHYSFILE_LUMPS,
};

// NOTE: When you change these constants, change them in engine
enum EPhysShapeType
{
	PHYSSHAPE_TYPE_CONCAVE = 0,
	PHYSSHAPE_TYPE_MOVABLECONCAVE,
	PHYSSHAPE_TYPE_CONVEX,
};

struct physmodellump_s
{
	int type;	// EPhysLump
	int	size;	// size excluding this structure
};
ALIGNED_TYPE(physmodellump_s, 4) physmodellump_t;

struct physmodelhdr_s
{
	int ident;
	int version;

	int num_lumps;
};
ALIGNED_TYPE(physmodelhdr_s, 4) physmodelhdr_t;

struct physjoint_s
{
	char name[MAX_PHYS_NAME_LENGTH]; // joint name

	int object_indexA; // physobject_t indices
	int object_indexB;

	Vector3D  position;

	Vector3D  minLimit;
	Vector3D  maxLimit;
};
ALIGNED_TYPE(physjoint_s, 4) physjoint_t;

struct physmodelprops_s
{
	int		model_usage;
	char	comment_string[MAX_PHYS_COMMENT_STRING];
};
ALIGNED_TYPE(physmodelprops_s, 4) physmodelprops_t;

struct physgeominfo_s
{
	int		startIndices;
	int		numIndices;

	int		type;
};
ALIGNED_TYPE(physgeominfo_s, 4) physgeominfo_t;

struct physobject_s
{
	char		surfaceprops[MAX_PHYS_SURF_NAME_LENGTH];// flesh, brick, etc.
	float		mass;									// mass of local object

	int			numShapes;								// shape count
	int			shape_indexes[MAX_PHYS_GEOM_PER_OBJECT];// indexes of geomdata

	Vector3D	offset;									// object initial offset
	Vector3D	mass_center;							// mass center of object
	int			body_part;								// body part index set in script
};
ALIGNED_TYPE(physobject_s, 4) physobject_t;
