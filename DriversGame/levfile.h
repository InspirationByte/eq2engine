//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level file
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVFILE_H
#define LEVFILE_H

#include "math/DkMath.h"
#include "dktypes.h"

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
	LEVLUMP_REGIONMAPINFO,		// region map info

	LEVLUMP_OBJECTDEFS,			// object definition cache ()

	LEVLUMP_ZONES,				// region zone name lists of levZoneRegions_t

	// TODO: curves?

	LEVLUMP_ENDMARKER = 255,
};

#define LEVEL_IDENT			MCHAR4('D','L','V','L')		// Drivers LEVEL
#define LEVEL_VERSION		1

#define LEV_OBJECT_NAME_LENGTH		 80
#define LEV_REG_ZONE_NAME_LENGTH	 80

struct levHdr_s
{
	int ident;
	int version;
};

ALIGNED_TYPE(levHdr_s,4) levHdr_t;

struct levLump_s
{
	int type; // ELevelLumps
	int size; // size in bytes
};

ALIGNED_TYPE(levLump_s,4) levLump_t;

// region data header
struct levRegionMapInfo_s
{
	// regions grid
	int numRegionsWide;
	int numRegionsTall;

	// heightfield tile/cell grid size
	int cellsSize;

	int numRegions;
};

ALIGNED_TYPE(levRegionMapInfo_s,4) levRegionMapInfo_t;

struct levRegionDataInfo_s
{
	int		numObjectDefs;
	int		numCellObjects;
	int		size;
};

ALIGNED_TYPE(levRegionDataInfo_s,4) levRegionDataInfo_t;

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
	LMODEL_FLAG_UNIQUE			= (1 << 3),		// model is unique so it stored in global model list/lump

	LMODEL_FLAG_GENERATED		= (1 << 4),
};

struct levObjectDefInfo_s
{
	int8	type;			// ELevObjectType

	int8	modelflags;		// ELevModelFlags (unused if OBJECT_CFG )
	short	level;			// placement level index (unused if OBJECT_CFG )
	int		size;			// internal model size (unused if OBJECT_CFG )
};

ALIGNED_TYPE(levObjectDefInfo_s,4) levObjectDefInfo_t;

enum ECellObjectFlags
{
	CELLOBJ_REGION_DEF		= (1 << 0),	// region def
	CELLOBJ_GENERATED		= (1 << 1),	// could be regenerated in editor
};

// region cell model object
struct levCellObject_s
{
	FVector3D	position;
	Vector3D	rotation;

	// tile position
	ushort		tile_x;
	ushort		tile_y;

	ushort		objectDefId : 14; // 4096 max
	ushort		flags : 2;

	char		name[LEV_OBJECT_NAME_LENGTH];
};

ALIGNED_TYPE(levCellObject_s,4) levCellObject_t;

//-----------------------------------------------------------------------------------

// zones build by regions
struct levZoneRegions_s
{
	int		numRegions;
	char	name[LEV_REG_ZONE_NAME_LENGTH];
};

ALIGNED_TYPE(levZoneRegions_s,4) levZoneRegions_t;

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
	ROADTYPE_PARKINGLOT,
	ROADTYPE_PAVEMENT		// where the peds walking on. Junction
};

enum ERoadFlags
{
	ROAD_FLAG_TRAFFICLIGHT		= (1 << 0),	// regulated straight by traffic light
	ROAD_FLAG_PARKING			= (1 << 1),
	ROAD_FLAG_HIGHSPEED			= (1 << 2), // high speeds (70 mph)
};

inline bool IsJunctionType(ERoadType type)				{ return (type == ROADTYPE_JUNCTION);  }
inline bool IsJunctionOrPavementType(ERoadType type)	{ return (type == ROADTYPE_JUNCTION || type == ROADTYPE_PAVEMENT);  }

struct levroadcell_s
{
	levroadcell_s()
	{
		type = ROADTYPE_NOROAD;
		flags = 0;
		direction = ROADDIR_NORTH;
	}

	uint8	type:4;			// ERoadType
	uint8	flags:4;		// ERoadFlags

	uint8	direction;		// ERoadDir

	// position on region
	ushort	posX;
	ushort	posY;
};

ALIGNED_TYPE(levroadcell_s,4) levroadcell_t;

//-----------------------------------------------------------------------------------

struct levOccluderLine_s
{
	Vector3D	start;
	Vector3D	end;
	float		height;
};

ALIGNED_TYPE(levOccluderLine_s,4) levOccluderLine_t;

#endif // LEVFILE_H
