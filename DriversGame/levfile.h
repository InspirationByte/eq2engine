//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level file
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVFILE_H
#define LEVFILE_H

#include "math/DkMath.h"
#include "Platform.h"

enum ELevelLumps
{
	// server-required lumps
	LEVLUMP_HEIGHTFIELDS = 0,	// heightfield data
	LEVLUMP_VISIBILITY,			// visibility bits for each region and their subdivisions
	LEVLUMP_ROADS,				// roads (straights) which are connected with junctions
	LEVLUMP_NAVIGATIONGRIDS,	// A* navigation grids for each heightfield
	LEVLUMP_OCCLUDERS,			// occlusion lines

	// client and server lumps
	LEVLUMP_REGIONS,			// region data
	LEVLUMP_REGIONINFO,			// regions info

	LEVLUMP_OBJECTDEFS,			// object definition cache ()
	//LEVLUMP_CURVEDFIELDS,		// heightfields with positions and rotations

	// TODO: curves?

	LEVLUMP_ENDMARKER = 255,
};

#define LEVEL_IDENT			MCHAR4('D','L','V','L')		// Drivers LEVEL
#define LEVEL_VERSION		1

struct levelhdr_s
{
	int ident;
	int version;
};

ALIGNED_TYPE(levelhdr_s,4) levelhdr_t;

struct levlump_s
{
	int type; // ELevelLumps
	int size; // size in bytes
};

ALIGNED_TYPE(levlump_s,4) levlump_t;

// region data header
struct levregionshdr_s
{
	// regions grid
	int numRegionsWide;
	int numRegionsTall;

	// heightfield tile/cell grid size
	int cellsSize;

	int numRegions;
};

ALIGNED_TYPE(levregionshdr_s,4) levregionshdr_t;

struct levregiondatahdr_s
{
	int numModels;
	int numModelObjects;
	int size;
};

ALIGNED_TYPE(levregiondatahdr_s,4) levregiondatahdr_t;

enum ELevObjectType
{
	LOBJ_TYPE_INTERNAL_STATIC = 0,	// internal static models, imported from OBJ
	LOBJ_TYPE_OBJECT_CFG,			// object from configuration
};

enum ELevModelFlags
{
	LMODEL_FLAG_ISGROUND		= (1 << 0),
	LMODEL_FLAG_NOCOLLIDE		= (1 << 1),
	LMODEL_FLAG_ALIGNTOCELL		= (1 << 2),		// object matrix is modified by cell
	LMODEL_FLAG_NONUNIQUE		= (1 << 3),		// model is not unique so it stored in global model list/lump
};

struct levmodelinfo_s
{
	int8	type;			// ELevObjectType

	int8	modelflags;		// ELevModelFlags (unused if OBJECT_CFG )
	short	level;			// placement level index (unused if OBJECT_CFG )
	int		size;			// internal model size (unused if OBJECT_CFG )
};

ALIGNED_TYPE(levmodelinfo_s,4) levmodelinfo_t;

#define LEV_OBJECT_NAME_LENGTH 80

// region cell model object
struct levcellmodelobject_s
{
	FVector3D	position;
	Vector3D	rotation;

	// tile position
	int			tile_x;
	int			tile_y;

	int			objIndex;

	char		name[LEV_OBJECT_NAME_LENGTH];
};

ALIGNED_TYPE(levcellmodelobject_s,4) levcellmodelobject_t;

//-----------------------------------------------------------------------------------

enum ERoadDir
{
	ROADDIR_NORTH = 0,
	ROADDIR_EAST,
	ROADDIR_SOUTH,
	ROADDIR_WEST,
};

enum ERoadType
{
	ROADTYPE_NOROAD = 0,

	ROADTYPE_STRAIGHT,
	ROADTYPE_JUNCTION,
};

enum ERoadFlags
{
	ROAD_FLAG_HAS_TRAFFICLIGHT = (1 << 0),

	ROAD_FLAG_RESERVED_3 = (1 << 2),
};

struct levroadcell_s
{
	uint8	type:4;			// ERoadType
	uint8	flags:4;		// ERoadFlags

	int8	direction;		// ERoadDir
	

	// position on region
	ushort	posX;
	ushort	posY;
};

ALIGNED_TYPE(levroadcell_s,4) levroadcell_t;

#define MAX_JUNCTION_ROADS	48

struct levroadjunction_s
{
	short roadLinks[MAX_JUNCTION_ROADS];
};

ALIGNED_TYPE(levroadjunction_s,4) levroadJunction_t;

//-----------------------------------------------------------------------------------

struct levOccluderLine_s
{
	Vector3D	start;
	Vector3D	end;
	float		height;
};

ALIGNED_TYPE(levOccluderLine_s,4) levOccluderLine_t;

#endif // LEVFILE_H