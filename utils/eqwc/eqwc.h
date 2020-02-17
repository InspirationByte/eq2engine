//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: World compiler main header
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQWC_H
#define EQWC_H

#include "eqlevel.h"
#include "core_base_header.h"
#include "DebugInterface.h"
#include "IMaterialSystem.h"
#include "Math/BoundingBox.h"
#include "Math/Volume.h"
#include "IVirtualStream.h"
#include "cmd_pacifier.h"

// compiler globals
struct world_globals_t
{
	EqString		worldName;

	bool			bOnlyPhysics;				// rebuild physics only
	bool			bOnlyEnts;					// build entites only?
	bool			bBrushCSG;					// brush CSG
	bool			bSectorDivision;			// sector subdivisions
	bool			bSectorsInXZ;				// sector subdivisions in 2D?
	bool			bNoLightmap;				// no lightmap UV?
	float			fSectorSize;				// sector size to be compiled
	float			fLuxelsPerMeter;			// luxel scale, also depends on final lightmap size
	float			fPickThreshold;				// pick angle thershold
	uint			nPackThreshold;				// pack thresh distance
	uint			lightmapSize;				// lightmap size
	int				numLightmaps;				// lightmap counter

	BoundingBox		worldBox;
};

// compiler globals
extern world_globals_t worldGlobals;

enum CompilerSurfaceFlags_e
{
	CS_FLAG_NODRAW			= (1 << 0),
	CS_FLAG_NOCOLLIDE		= (1 << 1),

	CS_FLAG_OCCLUDER		= (1 << 2),		// occlusion surface

	CS_FLAG_NOFACE			= (CS_FLAG_NOCOLLIDE | CS_FLAG_NODRAW),	// removes face

	CS_FLAG_USEDINSECTOR	= (1 << 3),		// surface put to the sector
	CS_FLAG_SUBDIVIDE		= (1 << 4),		// surface must be subdivided
	CS_FLAG_DONTSUBDIVIDE	= (1 << 5),

	CS_FLAG_MULTIMATERIAL	= (1 << 6),
	CS_FLAG_NOLIGHTMAP		= (1 << 7),

	CS_FLAG_WATER			= (1 << 8),
};

// Editor surface texture flag
// must be same with compiler
enum SurftexFlags_e
{
	STFL_SELECTED		= (1 << 0),	// dummy for game
	STFL_TESSELATED		= (1 << 1), // tesselation
	STFL_NOSUBDIVISION	= (1 << 2),	// non-subdividable
	STFL_NOCOLLIDE		= (1 << 3),	// not collideable
	STFL_CUSTOMTEXCOORD	= (1 << 4),	// custom texture coordinates, EDITABLE_SURFACE only
};

enum SurfaceType_e
{
	SURFTYPE_TERRAINPATCH = 0,	// only terrainpatch type can be tesselated
	SURFTYPE_STATIC,			// static models, fixed mesh, no tesselation
};

#define DEFAULT_PACK_THRESH		2
#define DEFAULT_PICK_THRESH		0.15f
#define DEFAULT_LIGHTMAP_SIZE	1024;
#define DEFAULT_LUXEL			10.0f // for now is fixed size
#define MAX_POINTS_ON_WINDING	128
#define WINDING_WELD_DIST		0.5f
#define MAX_BRUSH_PLANES		256
#define AUTOSMOOTHGROUP_THRESH	0.75f

#define MAIN_SECTOR				0

struct cwbrush_t;
struct brushwinding_t;

// TODO: refactoring begins there

// world layer
struct cwlayer_t
{
	EqString name;	// name of layer for scripting
	bool isActive;	// layer is activated in game?
};

// brush face
struct cwbrushface_t
{
	// face material
	IMaterial*						pMaterial;

	// texture matrix and surface data
	Plane							UAxis;		// tangent plane
	Plane							VAxis;		// binormal plane

	Plane							Plane;		// normal plane

	Vector2D						vScale;		// texture scale

	// flags
	int								flags;

	// smoothing group index
	ubyte							nSmoothingGroup;

	brushwinding_t*					winding;
	cwbrush_t*						brush;

	brushwinding_t*					visibleHull;

	int								planenum;
};

//------------------------------------------------------------------------------------
// ROOM volumes
//------------------------------------------------------------------------------------

struct cwlitsurface_t;

// room volume
struct cwroomvolume_t
{
	DkList<Plane>			volumePlanes;	// volume planes
	DkList<Vector3D>		volumePoints;
	cwbrush_t*				brush;			// brush that creates this volume

	BoundingBox				bounds;
	bool					checked;

	DkList<cwlitsurface_t*>	surfaces;

	struct cwroom_t*		room;			// room linked to this volume
};

struct cwlitsurface_t;

// room
struct cwroom_t
{
	DkList<cwroomvolume_t*> volumes;		// volume list

	int						numPortals;
	struct cwareaportal_t*	portals[MAX_LINKED_PORTALS];	// the portals that links other rooms

	BoundingBox				bounds;

	int						room_idx;
};

// portal/connector
struct cwareaportal_t
{
	int					numRooms;
	cwroom_t*			rooms[2]; // rooms linked to this areaportal

	cwbrush_t*			brush;	// a brush that creates this area portal. Used to generate occlusion triangles

	BoundingBox			bounds;

	brushwinding_t*		windings[2];	// windings that used for this portal

	int					firstroom_volume_idx;
	int					firstroom_volplane_idx;

	DkList<Vector3D>	volumePoints;
};

//------------------------------------------------------------------------------------
// Water volumes
//------------------------------------------------------------------------------------

// room volume
struct cwwatervolume_t
{
	DkList<Plane>			volumePlanes;	// volume planes
	DkList<Vector3D>		volumePoints;
	cwbrush_t*				brush;			// brush that creates this volume

	BoundingBox				bounds;
	bool					checked;

	DkList<cwlitsurface_t*>	surfaces;

	struct cwwaterinfo_t*	waterinfo;			// water info linked to this volume
};

// room
struct cwwaterinfo_t
{
	DkList<cwwatervolume_t*>	volumes;		// volume list

	BoundingBox					bounds;

	float						waterHeight;

	int							water_idx;
	IMaterial*					pMaterial;
};

//------------------------------------------------------------------------------------

// triangle-fan winding especially for brushes
struct brushwinding_t
{
	// this is a triangle fan
	eqlevelvertex_t		vertices[MAX_POINTS_ON_WINDING];
	int					numVerts;

	cwbrushface_t*		face;
};

enum BrushUsageFlag_e
{
	BRUSH_FLAG_ROOMFILLER		= (1 << 0),		// room filler
	BRUSH_FLAG_AREAPORTAL		= (1 << 1),		// area portal that joins room fillers
	BRUSH_FLAG_WATER			= (1 << 2),
};

// brush
struct cwbrush_t
{
	cwbrush_t*			next;

	// 
	cwbrushface_t*		faces;
	int					numfaces;

	// compiler brush flags
	int					flags;
};

// world compilation structures and classes
struct cwsurface_t
{
	cwsurface_t()
	{
		numIndices = 0;
		numVertices = 0;

		pVerts = NULL;
		pIndices = NULL;

		pMaterial = NULL;
		flags = 0;
		surfflags = 0;

		nTesselation = 0;
		nWide = nTall = 0;

		type = SURFTYPE_STATIC;
		nSmoothingGroup = 0;
		bbox.Reset();

		layer_index = 0;
	}

	// basic surface data
	int	numIndices;
	int numVertices;

	eqlevelvertex_t*	pVerts;
	int*				pIndices;

	// surface group material
	IMaterial*			pMaterial;

	int					flags;
	int					surfflags;

	int					nTesselation;
	int					nWide;
	int					nTall;

	SurfaceType_e		type;

	ubyte				nSmoothingGroup;

	BoundingBox			bbox;

	int					layer_index;

	int					surf_index;
};

// lightmapped surface base
// TODO: chart information inside this surface
struct cwlitsurface_t
{
	cwlitsurface_t()
	{
		numIndices = 0;
		numVertices = 0;

		pVerts = NULL;
		pIndices = NULL;

		pMaterial = NULL;
		flags = 0;

		lightmap_id = -1;

		bbox.Reset();

		layer_index = 0;
	}

	// basic surface data
	int	numIndices;
	int numVertices;

	eqlevelvertexlm_t*	pVerts;
	int*				pIndices;

	// surface group material
	IMaterial*			pMaterial;

	int					flags;

	int					lightmap_id;

	BoundingBox			bbox;

	int					layer_index;
};

cwlitsurface_t* LoadLitSurface(IVirtualStream* pStream);
bool			SaveLitSurface(cwlitsurface_t* pSurf, IVirtualStream* pStream);

struct ekeyvalue_t
{
	char name[128];
	char value[512];
};

// compiler entity TODO: attach surfaces to entities
struct cwentity_t
{
	// TODO: make more interactive implementation
	DkList<ekeyvalue_t> keys;

	ekeyvalue_t* FindKey( const char* pszKeyName )
	{
		for( int i = 0; i < keys.numElem(); i++ )
		{
			if(!stricmp(keys[i].name, pszKeyName))
				return &keys[i];
		}

		return NULL;
	}

	int	layer_index;
};

// sector nodes
struct cwsectornode_t
{
	cwlitsurface_t*				surface;			// surface assigned to this box
	BoundingBox					bbox;
};

// sectors
struct cwsector_t
{
	DkList<cwsectornode_t>		nodes;
	BoundingBox					bbox;
	bool						used;
};

// decals
struct cwdecal_t
{
	Vector3D	origin;
	Vector3D	size;

	IMaterial*	pMaterial;

	Vector3D	projaxis;
	Vector3D	uaxis;
	Vector3D	vaxis;

	Vector2D	texscale;
	float		rotate;

	bool		projectcoord;
};

// data at load.
extern DkList<cwsurface_t*>			g_surfacemodels;	// surface models
//extern DkList<cwbrush_t*>			g_brushes;			// brush models that will be casted to surface models automatically
extern cwbrush_t*					g_pBrushList;
extern int							g_numBrushes;
extern DkList<cwdecal_t*>			g_decals;			// loaded decals

// pre-final data
extern DkList<cwsurface_t*>			g_surfaces;			// surface models, to be here may be tesselated if terrain patch
extern DkList<cwentity_t*>			g_entities;			// world entities
extern DkList<cwsector_t>			g_sectors;			// sectors
extern DkList<cwlitsurface_t*>		g_litsurfaces;

extern DkList<eqworldlayer_t*>		g_layers;

extern DkList<IMaterial*>			g_materials;

extern DkList<eqlevelphyssurf_t>	g_physicssurfs;
extern DkList<eqphysicsvertex_t>	g_physicsverts;
extern DkList<int>					g_physicsindices;


extern DkList<cwsurface_t*>			g_occlusionsurfaces;

extern DkList<cwareaportal_t*>		g_portalList;
extern DkList<cwroom_t*>			g_roomList;

extern DkList<cwwaterinfo_t*>		g_waterInfos;

//
// Compiler functions
//

int				ComputeCompileSurfaceFlags(IMaterial* pMaterial);

brushwinding_t* AllocWinding();
brushwinding_t* CopyWinding(const brushwinding_t* src);
void			FreeWinding(brushwinding_t* pWinding);

// splits the face by this face, and results a
void			Split(const brushwinding_t *w, Plane& plane, brushwinding_t **front, brushwinding_t **back );

void			FreeBrush(cwbrush_t* brush);

void			FreeSurface(cwsurface_t* surf);
void			FreeLitSurface(cwlitsurface_t* surf);
void			FreeDecal(cwlitsurface_t* surf);

void			ComputeSurfaceBBOX(cwsurface_t* pSurf);
void			ComputeSurfaceBBOX(cwlitsurface_t* pSurf);
void			MergeSurfaceIntoList(cwsurface_t* pSurface, DkList<cwsurface_t*> &surfaces, bool checkTranslucents = false);
void			SplitSurfaceByPlane(cwlitsurface_t* surf, Plane &plane, cwlitsurface_t* front, cwlitsurface_t* back);

// stages
void			ProcessBrushes();
void			ProcessSurfaces();
void			ParametrizeLightmapUVs();
void			MakeSectors();
void			MakeWaterInfosFromVolumes();
void			GenerateDecals();

// parses world data
void			ParseWorldData(kvkeybase_t* pLevelSection);

// builds world
void			BuildWorld();

// builds physics geometry
void			BuildPhysicsGeometry();

// writes world
void			WriteWorld();

#endif // EQWC_H