//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level data and renderer
//				You might consider it as some ugly code :(
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVEL_H
#define LEVEL_H

#include "dsm_loader.h"
#include "region.h"
#include "levelobject.h"

#include "road_defs.h"

#include "DrvSynDecals.h"

#include "imaging/ImageLoader.h"

#define AI_NAV_DETAILED_SCALE		2
#define LEVELS_PATH					"levels/"

struct cellpoint_t
{
	IVector2D	point;
	navcell_t*	cell;
};

typedef void (*RegionLoadUnloadCallbackFunc)(CLevelRegion* reg, int regionIdx);

enum ECellClearStateMode
{
	NAV_CLEAR_WORKSTATES = 0,
	NAV_CLEAR_DYNAMIC_OBSTACLES = 1,
};

static int s_navGridScales[2] = { AI_NAV_DETAILED_SCALE, 1 };
static float s_invNavGridScales[2] = { 1.0f/float(AI_NAV_DETAILED_SCALE), 1.0f };

//-----------------------------------------------------------------------------------------
// Game level renderer, utilities
//-----------------------------------------------------------------------------------------
class CGameLevel : public CEqThread
{
	friend class CLevelRegion;
	friend class CGameWorld;
	friend class CModelListRenderPanel;
public:
	CGameLevel();
	virtual ~CGameLevel() {}

	void							Init(int wide, int tall, int cells, bool isCleanLevel);
	void							Cleanup(); // unloads level

	void							UnloadRegions();

	bool							Load(const char* levelname);

	//-------------------------------------------------------------------------
	// region spooling

	void							PreloadRegion(int x, int y);

	// queries regions for loading. Returns the center region at exact point
	CLevelRegion*					QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);
	CLevelRegion*					QueryNearestRegions(const IVector2D& point, bool waitLoad = false);

	//-------------------------------------------------------------------------
	// rendering

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	void							Render(const Vector3D& cameraPosition, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	int								UpdateRegions( RegionLoadUnloadCallbackFunc func = NULL);
	void							RespawnAllObjects();

	void							UpdateDebugMaps();

	//-------------------------------------------------------------------------
	// objects stuff

	// searches for object on level
	// useful if you need to find spawn point on non-loaded level
	bool							FindObjectOnLevel(levCellObject_t& objectInfo, const char* name, const char* defName = NULL) const;

	bool							LoadObjectDefs(const char* name);

	CLevObjectDef*					FindObjectDefByName(const char* name) const;

	//-------------------------------------------------------------------------
	// 2D grid stuff

	CHeightTileFieldRenderable*		GetHeightFieldAt(const IVector2D& XYPos, int idx = 0) const;		// returns heightfield if region at global 2D point

	bool							PositionToRegionOffset(const Vector3D& pos, IVector2D& outXYPos) const;			// calculates region index from 3D position

	CLevelRegion*					GetRegionAt(const IVector2D& XYPos) const;							// returns region at global 2D point
	CLevelRegion*					GetRegionAtPosition(const Vector3D& pos) const;						// returns region which belongs to 3D position

	//
	// Global (per tile) indexes
	//
	bool							GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const;	// returns global tile XYPos which belongs to 3D position
	bool							GetRegionAndTile(const Vector3D& pos, CLevelRegion** outReg, IVector2D& outLocalXYPos) const;	// returns a tile XYPos and region which belongs to 3D position
	bool							GetRegionAndTileAt(const IVector2D& tilePos, CLevelRegion** outReg, IVector2D& outLocalXYPos) const; // returns a tile at region

	float							GetWaterLevel(const Vector3D& pos) const;			// water tile height
	float							GetWaterLevelAt(const IVector2D& tilePos) const;	// water tile height

	//
	// conversions
	//
	IVector2D						PositionToGlobalTilePoint( const Vector3D& pos ) const;				// converts 3D position to global tile 2D point
	Vector3D						GlobalTilePointToPosition( const IVector2D& point ) const;			// converts global tile 2D point to 3D position (3D tile center)

	IVector2D						GlobalPointToRegionPosition(const IVector2D& point) const;			// converts global tile to region position

	void							GlobalToLocalPoint( const IVector2D& globalPoint, IVector2D& outLocalPoint, CLevelRegion** outReg ) const;		// converts global tile 2D point to region and local tile
	void							LocalToGlobalPoint( const IVector2D& localPoint, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;	// converts local point at region to global tile 2D point

	//-------------------------------------------------------------------------
	// roadmap grid

	levroadcell_t*					Road_GetGlobalTile(const Vector3D& pos, CLevelRegion** pRegion = NULL) const;		// returns road cell which belongs to 3D position
	levroadcell_t*					Road_GetGlobalTileAt(const IVector2D& point, CLevelRegion** pRegion = NULL) const;	// returns road cell by global tile 2D point

	straight_t						Road_GetStraightAtPoint( const IVector2D& point, int numIterations = 4 ) const;		// calculates straight starting from global tile 2D point
	straight_t						Road_GetStraightAtPos( const Vector3D& pos, int numIterations = 4 ) const;			// calculates straight starting from 3D position

	roadJunction_t					Road_GetJunctionAtPoint( const IVector2D& point, int numIterations = 4 ) const;		// calculates junction starting from global tile 2D point
	roadJunction_t					Road_GetJunctionAtPos( const Vector3D& pos, int numIterations = 4 ) const;			// calculates junction starting from 3D position

	// current road lane
	int								Road_GetLaneIndexAtPoint( const IVector2D& point, int numIterations = 8 );			// calculates lane number from road at global tile 2D point
	int								Road_GetLaneIndexAtPos( const Vector3D& pos, int numIterations = 8 );				// calculates lane number from road at 3D position

	// road width in lane count
	int								Road_GetNumLanesAtPoint( const IVector2D& point, int numIterations = 8 );			// calculates lane count from lane at global tile 2D point
	int								Road_GetNumLanesAtPos( const Vector3D& pos, int numIterations = 8 );					// calculates lane count from lane at 3D position

	int								Road_GetWidthInLanesAtPoint( const IVector2D& point, int numIterations = 16, int iterationsOnEmpty = 0 );	// calculates road width in lanes from road at global tile 2D point
	int								Road_GetWidthInLanesAtPos( const Vector3D& pos, int numIterations = 16 );		// calculates road width in lanes from road at 3D position

	bool							Road_FindBestCellForTrafficLight( IVector2D& out, const Vector3D& origin, int trafficDir, int juncIterations = 16, bool leftHanded = false );

	//-------------------------------------------------------------------------
	// navigation grid

	void							Nav_ClearDynamicObstacleMap();

	void							Nav_AddObstacle(CLevelRegion* reg, regionObject_t* ref);						// adds navigation obstacle

	//
	// conversions
	//
	void							Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2, float offs = 1.5f) const;		// calculates bounds

	IVector2D						Nav_PositionToGlobalNavPoint(const Vector3D& pos) const;	// converts 3D position to navigation grid 2D point

	Vector3D						Nav_GlobalPointToPosition(const IVector2D& point) const;	// converts 2D navigation point to 3D position

	bool							Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit = 256, bool fast = false);
	bool							Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit = 256, bool fast = false);

	float							Nav_TestLine(const Vector3D& start, const Vector3D& end, bool obstacles = false);
	float							Nav_TestLine2D(const IVector2D& start, const IVector2D& end, bool obstacles = false);

	navcell_t&						Nav_GetCellStateAtGlobalPoint(const IVector2D& point);

	ubyte&							Nav_GetTileAtGlobalPoint(const IVector2D& point, bool obstacles = false);
	ubyte&							Nav_GetTileAtPosition(const Vector3D& position, bool obstacles = false);

	navcell_t&						Nav_GetTileAndCellAtGlobalPoint(const IVector2D& point, ubyte& tile);

	//----------------------------------

	void							GetDecalPolygons( decalPrimitives_t& polys, occludingFrustum_t* frustum = NULL);

	// TODO: render code
	DkList<CLevObjectDef*>			m_objectDefs;

	int								m_wide;
	int								m_tall;

	int								m_cellsSize;

	int								m_navGridSelector;

	CEqMutex&						m_mutex;

protected:

	bool					_Load(IFile* levFile);

	void					Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const;
	void					Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	void					Nav_ClearCellStates(ECellClearStateMode mode);

	int						UpdateRegionLoading();

	int						Run();

	void					InitObjectDefFromKeyValues(CLevObjectDef* def, kvkeybase_t* defDesc);

	void					ReadObjectDefsLump(IVirtualStream* stream);
	void					ReadHeightfieldsLump(IVirtualStream* stream);
	void					ReadRoadsLump(IVirtualStream* stream);

	void					ReadRegionInfo(IVirtualStream* stream);

	void					LoadRegionAt(int regionIndex, IVirtualStream* stream); // loads region

	//--------------------------------------------------------
#ifdef EDITOR
public:
	CEditorLevelRegion*		m_regions;

	int						m_objectDefIdCounter;
protected:
#else
	CLevelRegion*			m_regions;
#endif // EDITOR

	DkList<CLevObjectDef*>	m_objectDefsCfg;

	int*					m_regionOffsets;			// spooling data offsets in LEVLUMP_REGIONS
	int*					m_occluderOffsets;			// spooling data offsets in LEVLUMP_OCCLUDERS

	int						m_numRegions;				// real region count in this level
	int						m_regionDataLumpOffset;		// region lump offset
	int						m_occluderDataLumpOffset;	// occluders offset

	EqString				m_levelName;

	IVertexBuffer*			m_instanceBuffer;

	DkList<cellpoint_t>		m_navOpenSet;
	ubyte					m_defaultNavTile;
};

void Road_GetJunctionExits(DkList<straight_t>& exits, const straight_t& road, const roadJunction_t& junc);

#endif // LEVEL_H
