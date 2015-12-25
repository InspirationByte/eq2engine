//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level data and renderer
//				You might consider it as some ugly code :(
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVEL_H
#define LEVEL_H

#include "utils/eqthread.h"

using namespace Threading;

#include "eqPhysics/eqPhysics.h"
#include "heightfield.h"
#include "dsm_loader.h"
#include "levfile.h"
#include "occluderSet.h"
#include "imaging/ImageLoader.h"

#ifdef EDITOR
#include "../DriversEditor/level_generator.h"
#endif // EDITOR

#define USE_INSTANCING

#define MAX_MODEL_INSTANCES 1024

#define AI_NAVIGATION_GRID_SCALE	2

#define ENGINE_REGION_MAX_HFIELDS	4	// you could make more

#define ROADNEIGHBOUR_OFFS_X(x)		{x, x-1, x, x+1}		// non-diagonal
#define ROADNEIGHBOUR_OFFS_Y(y)		{y-1, y, y+1, y}

//-----------------------------------------------------------------------------------------

struct lmodeldrawvertex_s
{
	lmodeldrawvertex_s()
	{
		position = vec3_zero;
		normal = vec3_up;
		texcoord = vec2_zero;
	}

	lmodeldrawvertex_s(Vector3D& p, Vector3D& n, Vector2D& t )
	{
		position = p;
		normal = n;
		texcoord = t;
	}

	Vector3D	position;
	Vector2D	texcoord;

	Vector3D	normal;
	//Matrix3x3	tbn;
};

// comparsion operator
inline bool operator == (const lmodeldrawvertex_s &u, const lmodeldrawvertex_s &v)
{
	if(u.position == v.position && u.texcoord == v.texcoord && u.normal == v.normal)
		return true;

	return false;
}

ALIGNED_TYPE(lmodeldrawvertex_s,4) lmodeldrawvertex_t;

struct lmodel_batch_t
{
	PPMEM_MANAGED_OBJECT()

	lmodel_batch_t()
	{
		startVertex = 0;
		numVerts = 0;
		startIndex = 0;
		numIndices = 0;

		pMaterial = NULL;
	}

	uint32		startVertex;
	uint32		numVerts;
	uint32		startIndex;
	uint32		numIndices;

	IMaterial*	pMaterial;
};

//-----------------------------------------------------------------------------------------

struct regobjectref_t;
class CLevelRegion;
class CEqBulletIndexedMesh;

class CLevelModel : public RefCountedObject
{
	friend class CGameLevel;

public:
	PPMEM_MANAGED_OBJECT()

							CLevelModel();
	virtual					~CLevelModel();

	void					Ref_DeleteObject() {}

	void					Cleanup();

	bool					CreateFrom(dsmmodel_t* pModel);

	void					Load(IVirtualStream* stream);
	void					Save(IVirtualStream* stream) const;

	// releases data but keeps batchs and VBO
	void					ReleaseData();

	bool					GenereateRenderData();

	void					GeneratePhysicsData(bool isGround = false);

	void					PreloadTextures();

	void					Render(int nDrawFlags, const BoundingBox& aabb);

	void					CreateCollisionObjects( CLevelRegion* reg, regobjectref_t* ref );

	// to create the physics
	DkList<CEqBulletIndexedMesh*>	m_batchMeshes;

	//-------------------------------------------------------------------

#ifdef EDITOR
	float					Ed_TraceRayDist(const Vector3D& start, const Vector3D& dir);
#endif

	BoundingBox				m_bbox;

	int						m_level;	// editor parameter - model layer location (0 - ground, 1 - sand, 2 - trees, 3 - others)

protected:
	CEqBulletIndexedMesh*	CreateBulletTriangleMeshFromModelBatch(lmodel_batch_t* batch);

	lmodel_batch_t*			m_batches;
	int						m_numBatches;

	lmodeldrawvertex_t*		m_verts;
	int*					m_indices;

	int						m_numVerts;
	int						m_numIndices;

	IVertexBuffer*			m_vertexBuffer;
	IIndexBuffer*			m_indexBuffer;
	IVertexFormat*			m_format;
};

//-----------------------------------------------------------------------------------------
// Level region data
//-----------------------------------------------------------------------------------------

class CGameObject;

struct objectcont_t;

struct regobjectref_t
{
	PPMEM_MANAGED_OBJECT()

	regobjectref_t()
	{
		object = NULL;
		container = NULL;
	}

	CGameObject*	object;

	objectcont_t*	container;

	Vector3D		position;
	Vector3D		rotation;

	Matrix4x4		transform;

	short			tile_x;
	short			tile_y;
	bool			tile_dependent;

	int				level;

	BoundingBox		bbox;

	DkList<CEqCollisionObject*> collisionObjects;
};

class IEqModel;

struct regObjectInstance_t
{
	Matrix4x4		worldTransform;

	//Quaternion	quat;
	//Vector4D		position;

	//int			lightIndex[16];		// light indexes to fetch from texture
};

struct levObjInstanceData_t
{
	levObjInstanceData_t()
	{
		numInstances = 0;
	}

	regObjectInstance_t		instances[MAX_MODEL_INSTANCES];
	int						numInstances;
};

// only omni lights
struct wlight_t
{
	Vector4D	position;	// position + radius
	ColorRGBA	color;		// color + intensity
};

struct wglow_t
{
	int			type;
	Vector4D	position;
	ColorRGBA	color;
};

struct wlightdata_t
{
	DkList<wlight_t>	m_lights;
	DkList<wglow_t>		m_glows;
};

void LoadDefLightData( wlightdata_t& out, kvkeybase_t* sec );
bool DrawDefLightData( Matrix4x4& objDefMatrix, const wlightdata_t& data, float brightness );

// model container, for editor
struct objectcont_t
{
	PPMEM_MANAGED_OBJECT()

	objectcont_t()
	{
		m_model = NULL;
#ifdef USE_INSTANCING
		m_instData = NULL;
#endif // USE_INSTANCING
		m_defModel = NULL;

#ifdef EDITOR
		m_preview = NULL;
		m_dirtyPreview = true;
#endif // EDITOR
		memset(&m_info, 0, sizeof(levmodelinfo_t));
	}

	EqString				m_name;

	CLevelModel*			m_model;

	//-----------------------------

	
	IEqModel*				m_defModel;

	EqString				m_defType;
	kvkeybase_t				m_defKeyvalues;
	
	levmodelinfo_t			m_info;

#ifdef EDITOR
	ITexture*				m_preview;
	bool					m_dirtyPreview;
#endif // EDITOR

#ifdef USE_INSTANCING
	wlightdata_t			m_lightData;
	levObjInstanceData_t*	m_instData;
#endif // USE_INSTANCING
};

void FreeLevObjectRef(regobjectref_t* obj);
void FreeLevObjectContainer(objectcont_t* container);

//----------------------------------------------------------------------

struct navcell_t
{
	// Position is not necessary in this form
	navcell_t()
	{
		cost = 0;
		flag = 0;
	}

	navcell_t(int8 parentDir, int8 c)
	{
		pdir = parentDir;
		cost = c;
		flag = 0;
	}

	ubyte		pdir : 3;	// parent direction (1,2,3,4)
	ubyte		cost : 4;	// cost of this point; max is 15
	ubyte		flag : 1;	// 1 = closed, 0 = open
};

//----------------------------------------------------------------------

class CLevelRegion
{
	friend class CGameLevel;
public:
	CLevelRegion();
	~CLevelRegion();

	void							Init();
	void							InitRoads();
	void							Cleanup();

	bool							IsRegionEmpty();	///< returns true if no models or placed tiles of heightfield found here 

#ifdef EDITOR
	void							Ed_Prerender();
#endif // EDITOR

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	void							Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);
	

	Vector3D						CellToPosition(int x, int y) const;
	IVector2D						GetTileAndNeighbourRegion(int x, int y, CLevelRegion** reg) const;

	void							WriteRegionData(IVirtualStream* stream, DkList<objectcont_t*>& models, bool final);
	void							ReadLoadRegion(IVirtualStream* stream, DkList<objectcont_t*>& models);

	void							WriteRegionRoads(IVirtualStream* stream);
	void							ReadLoadRoads(IVirtualStream* stream);

	void							WriteRegionOccluders(IVirtualStream* stream);
	void							ReadLoadOccluders(IVirtualStream* stream);

	void							RespawnObjects();

	CHeightTileFieldRenderable*		GetHField( int idx = 0 ) const;
	int								GetNumHFields() const;
	int								GetNumNomEmptyHFields() const;

	//----------------------------------------------------

#ifdef EDITOR
	int								Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist);
	int								Ed_ReplaceDefs(objectcont_t* whichReplace, objectcont_t* replaceTo);
#endif

	BoundingBox						m_bbox;

	CHeightTileFieldRenderable*		m_heightfield[ENGINE_REGION_MAX_HFIELDS];	///< heightfield used on region

	levroadcell_t*					m_roads;			///< road data. Must correspond with heightfield

	ubyte*							m_navGrid;			///< A* navigation grid
	navcell_t*						m_navGridStateList;	///< A* open/close state list

	bool							m_navAffected;

	int								m_navWide;
	int								m_navTall;

	//levroadcell_t**					m_flatroads;	///< road pointers to m_roads
	int								m_numRoadCells;

	DkList<regobjectref_t*>			m_objects;			///< complex and non-complex models
	DkList<levOccluderLine_t>		m_occluders;		///< occluders

	// TODO: road array, visibility data and other....
	bool							m_render;
	bool							m_isLoaded;
	int								m_regionIndex;

	bool							m_scriptEventCallbackCalled;

	CEqInterlockedInteger			m_queryTimes;		///< render (client) or physics engine query for server

	CGameLevel*						m_level;
};

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regobjectref_t* ref);

//
// Helper struct for road straight
//
struct straight_t
{
	straight_t()
	{
		breakIter = 0;
		dirChangeIter = 0;
		direction = -1;
		lane = -1;
	}

	IVector2D	start;
	IVector2D	end;

	int			breakIter;
	int			dirChangeIter;
	int			direction;
	int			lane;
};

struct roadJunction_t
{
	roadJunction_t()
	{
		startIter = 0;
		breakIter = 0;
	}

	IVector2D	start;
	IVector2D	end;

	int			startIter;
	int			breakIter;
};

bool IsOppositeDirectionTo(int dirA, int dirB);

struct pathFindResult_t
{
	IVector2D			start;
	IVector2D			end;

	DkList<IVector2D>	points;	// z is distance in cells
};

typedef void (*RegionLoadUnloadCallbackFunc)(CLevelRegion* reg, int regionIdx);

//-----------------------------------------------------------------------------------------
// Game level renderer, utilities
//-----------------------------------------------------------------------------------------
class CGameLevel : public CEqThread
{
	friend class CLevelRegion;

public:
	CGameLevel();

	void							Init(int wide, int tall, int cells, bool isCleanLevel);	// number of height fields
	//void							InitOLD(int wide, int tall, IVirtualStream* stream = NULL);	// number of height fields

	void							Cleanup();

	bool							Load(const char* levelname, kvkeybase_t* kvDefs);
	bool							Save(const char* levelname, bool final = false);

	//---------------------------------

	//
	// Region indexes
	//

	CHeightTileFieldRenderable*		GetHeightFieldAt(const IVector2D& XYPos, int idx = 0) const;
	CLevelRegion*					GetRegionAt(const IVector2D& XYPos) const;

	// resolving indexes
	bool							GetPointAt(const Vector3D& pos, IVector2D& outXYPos) const;
	CLevelRegion*					GetRegionAtPosition(const Vector3D& pos) const;

	//
	// Global (per tile) indexes
	//
	bool							GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const;

	bool							GetRegionAndTile(const Vector3D& pos, CLevelRegion** pReg, IVector2D& outLocalXYPos) const;
	bool							GetRegionAndTileAt(const IVector2D& point, CLevelRegion** pReg, IVector2D& outLocalXYPos) const;

	//
	// conversions
	//
	IVector2D						PositionToGlobalTilePoint( const Vector3D& pos ) const;
	Vector3D						GlobalTilePointToPosition( const IVector2D& point ) const;

	void							GlobalToLocalPoint( const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion ) const;
	void							LocalToGlobalPoint( const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	levroadcell_t*					GetGlobalRoadTile(const Vector3D& pos, CLevelRegion** pRegion = NULL) const;
	levroadcell_t*					GetGlobalRoadTileAt(const IVector2D& point, CLevelRegion** pRegion = NULL) const;

	//---------------------------------

	straight_t						GetStraightAtPoint( const IVector2D& point, int numIterations = 4 ) const;
	straight_t						GetStraightAtPos( const Vector3D& pos, int numIterations = 4 ) const;

	roadJunction_t					GetJunctionAtPoint( const IVector2D& point, int numIterations = 4 ) const;
	roadJunction_t					GetJunctionAtPos( const Vector3D& pos, int numIterations = 4 ) const;

	// current road lane
	int								GetLaneIndexAtPoint( const IVector2D& point, int numIterations = 8 );
	int								GetLaneIndexAtPos( const Vector3D& pos, int numIterations = 8 );

	// road width in lane count
	int								GetNumLanesAtPoint( const IVector2D& point, int numIterations = 8 );
	int								GetNumLanesAtPos( const Vector3D& pos, int numIterations = 8 );

	int								GetRoadWidthInLanesAtPoint( const IVector2D& point, int numIterations = 16 );
	int								GetRoadWidthInLanesAtPos( const Vector3D& pos, int numIterations = 16 );


	//---------------------------------

	void							PreloadRegion(int x, int y);

	void							QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);
	void							QueryNearestRegions(const IVector2D& point, bool waitLoad = false);

#ifdef EDITOR
	void							Ed_Prerender(const Vector3D& cameraPosition);
#endif // EDITOR

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	void							Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	int								UpdateRegions( RegionLoadUnloadCallbackFunc func = NULL);
	void							RespawnAllObjects();

	void							Nav_AddObstacle(CLevelRegion* reg, regobjectref_t* ref);

	//
	// conversions
	//
	void							Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2) const;

	IVector2D						Nav_PositionToGlobalNavPoint(const Vector3D& pos) const;

	Vector3D						Nav_GlobalPointToPosition(const IVector2D& point) const;

	void							Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const;
	void							Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	navcell_t&						Nav_GetCellStateAtGlobalPoint(const IVector2D& point);
	ubyte&							Nav_GetTileAtGlobalPoint(const IVector2D& point);

	bool							Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);
	bool							Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);

	void							Nav_ClearCellStates();

#ifdef EDITOR
	int								Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CLevelRegion** reg, float& dist);
	bool							Ed_GenerateFromImage(const CImage& img, int cellsPerRegion, LevelGenParams_t& genParams);
#endif

	// TODO: render code

	DkList<objectcont_t*>			m_objectDefs;

	int								m_wide;
	int								m_tall;

	int								m_cellsSize;

	CEqMutex&						m_mutex;

protected:

	int						UpdateRegionLoading();

	int						Run();

	void					WriteLevelRegions(IVirtualStream* stream, const char* levelname, bool final);
	void					WriteObjectDefsLump(IVirtualStream* stream);
	void					WriteHeightfieldsLump(IVirtualStream* stream);

	void					ReadObjectDefsLump(IVirtualStream* stream, kvkeybase_t* kvDefs);
	void					ReadHeightfieldsLump(IVirtualStream* stream);

	void					ReadRegionInfo(IVirtualStream* stream);

	void					LoadRegionAt(int regionIndex, IVirtualStream* stream); // loads region

	CLevelRegion*			m_regions;

	int*					m_regionOffsets;			// spooling data offsets in LEVLUMP_REGIONS
	int*					m_roadOffsets;				// spooling data offsets in LEVLUMP_ROADS
	int*					m_occluderOffsets;			// spooling data offsets in LEVLUMP_OCCLUDERS

	int						m_numRegions;				// real region count in this level
	int						m_regionDataLumpOffset;		// region lump offset
	int						m_roadDataLumpOffset;		// road lump offset
	int						m_occluderDataLumpOffset;	// occluders offset

	EqString				m_levelName;

	IVertexBuffer*			m_instanceBuffer;
};

#endif // LEVEL_H