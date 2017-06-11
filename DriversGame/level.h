//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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

#define AI_NAVIGATION_GRID_SCALE	2

struct pathFindResult_t
{
	IVector2D			start;
	IVector2D			end;

	DkList<IVector2D>	points;	// z is distance in cells
};

typedef void (*RegionLoadUnloadCallbackFunc)(CLevelRegion* reg, int regionIdx);

enum ECellClearStateMode
{
	NAV_CLEAR_WORKSTATES = 0,
	NAV_CLEAR_DYNAMIC_OBSTACLES = 1,
};

//-----------------------------------------------------------------------------------------
// Game level renderer, utilities
//-----------------------------------------------------------------------------------------
class CGameLevel : public CEqThread
{
	friend class CLevelRegion;
public:
	CGameLevel();
	virtual ~CGameLevel() {}

	void							Init(int wide, int tall, int cells, bool isCleanLevel);	
	void							Cleanup(); // unloads level

	virtual bool					Load(const char* levelname, kvkeybase_t* kvDefs);

	//-------------------------------------------------------------------------
	// region spooling

	void							PreloadRegion(int x, int y);

	void							QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);
	void							QueryNearestRegions(const IVector2D& point, bool waitLoad = false);

	//-------------------------------------------------------------------------
	// rendering

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	void							Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	int								UpdateRegions( RegionLoadUnloadCallbackFunc func = NULL);
	void							RespawnAllObjects();
	void							UpdateDebugMaps();

	//-------------------------------------------------------------------------
	// 2D grid stuff

	CHeightTileFieldRenderable*		GetHeightFieldAt(const IVector2D& XYPos, int idx = 0) const;		// returns heightfield if region at global 2D point
	
	bool							GetPointAt(const Vector3D& pos, IVector2D& outXYPos) const;			// calculates region index from 3D position

	CLevelRegion*					GetRegionAt(const IVector2D& XYPos) const;							// returns region at global 2D point
	CLevelRegion*					GetRegionAtPosition(const Vector3D& pos) const;						// returns region which belongs to 3D position

	//
	// Global (per tile) indexes
	//
	bool							GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const;	// returns global tile XYPos which belongs to 3D position
	bool							GetRegionAndTile(const Vector3D& pos, CLevelRegion** outReg, IVector2D& outLocalXYPos) const;	// returns a tile XYPos and region which belongs to 3D position
	bool							GetRegionAndTileAt(const IVector2D& tilePos, CLevelRegion** outReg, IVector2D& outLocalXYPos) const; // returns a tile at region 

	float							GetWaterLevel(const Vector3D& pos) const;
	float							GetWaterLevelAt(const IVector2D& tilePos) const;

	//
	// conversions
	//
	IVector2D						PositionToGlobalTilePoint( const Vector3D& pos ) const;				// converts 3D position to global tile 2D point
	Vector3D						GlobalTilePointToPosition( const IVector2D& point ) const;			// converts global tile 2D point to 3D position (3D tile center)

	void							GlobalToLocalPoint( const IVector2D& globalPoint, IVector2D& outLocalPoint, CLevelRegion** outReg ) const;		// converts global tile 2D point to region and local tile
	void							LocalToGlobalPoint( const IVector2D& localPoint, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;	// converts local point at region to global tile 2D point

	//-------------------------------------------------------------------------
	// roads

	levroadcell_t*					GetGlobalRoadTile(const Vector3D& pos, CLevelRegion** pRegion = NULL) const;		// returns road cell which belongs to 3D position
	levroadcell_t*					GetGlobalRoadTileAt(const IVector2D& point, CLevelRegion** pRegion = NULL) const;	// returns road cell by global tile 2D point

	straight_t						GetStraightAtPoint( const IVector2D& point, int numIterations = 4 ) const;		// calculates straight starting from global tile 2D point
	straight_t						GetStraightAtPos( const Vector3D& pos, int numIterations = 4 ) const;			// calculates straight starting from 3D position

	roadJunction_t					GetJunctionAtPoint( const IVector2D& point, int numIterations = 4 ) const;		// calculates junction starting from global tile 2D point
	roadJunction_t					GetJunctionAtPos( const Vector3D& pos, int numIterations = 4 ) const;			// calculates junction starting from 3D position

	// current road lane
	int								GetLaneIndexAtPoint( const IVector2D& point, int numIterations = 8 );			// calculates lane number from road at global tile 2D point
	int								GetLaneIndexAtPos( const Vector3D& pos, int numIterations = 8 );				// calculates lane number from road at 3D position

	// road width in lane count
	int								GetNumLanesAtPoint( const IVector2D& point, int numIterations = 8 );			// calculates lane count from lane at global tile 2D point
	int								GetNumLanesAtPos( const Vector3D& pos, int numIterations = 8 );					// calculates lane count from lane at 3D position

	int								GetRoadWidthInLanesAtPoint( const IVector2D& point, int numIterations = 16 );	// calculates road width in lanes from road at global tile 2D point
	int								GetRoadWidthInLanesAtPos( const Vector3D& pos, int numIterations = 16 );		// calculates road width in lanes from road at 3D position

	bool							FindBestRoadCellForTrafficLight( IVector2D& out, const Vector3D& origin, int trafficDir, int juncIterations = 16 );

	//-------------------------------------------------------------------------
	// navigation

	void							Nav_ClearDynamicObstacleMap();

	void							Nav_AddObstacle(CLevelRegion* reg, regionObject_t* ref);						// adds navigation obstacle

	//
	// conversions
	//
	void							Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2, float offs = 1.5f) const;		// calculates bounds

	IVector2D						Nav_PositionToGlobalNavPoint(const Vector3D& pos) const;	// converts 3D position to navigation grid 2D point

	Vector3D						Nav_GlobalPointToPosition(const IVector2D& point) const;	// converts 2D navigation point to 3D position

	bool							Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);
	bool							Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);

	float							Nav_TestLine(const Vector3D& start, const Vector3D& end, bool obstacles = false);
	float							Nav_TestLine2D(const IVector2D& start, const IVector2D& end, bool obstacles = false);

	navcell_t&						Nav_GetCellStateAtGlobalPoint(const IVector2D& point);
	ubyte&							Nav_GetTileAtGlobalPoint(const IVector2D& point, bool obstacles = false);

	//----------------------------------

	void							GetDecalPolygons( decalprimitives_t& polys, occludingFrustum_t* frustum = NULL);

	// TODO: render code

	DkList<CLevObjectDef*>			m_objectDefs;

	int								m_wide;
	int								m_tall;

	int								m_cellsSize;

	CEqMutex&						m_mutex;

protected:

	void					Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const;
	void					Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	void					Nav_ClearCellStates(ECellClearStateMode mode);

	int						UpdateRegionLoading();

	int						Run();

	void					ReadObjectDefsLump(IVirtualStream* stream, kvkeybase_t* kvDefs);
	void					ReadHeightfieldsLump(IVirtualStream* stream);

	void					ReadRegionInfo(IVirtualStream* stream);

	void					LoadRegionAt(int regionIndex, IVirtualStream* stream); // loads region

	//--------------------------------------------------------
#ifdef EDITOR
	CEditorLevelRegion*		m_regions;
#else
	CLevelRegion*			m_regions;
#endif // EDITOR

	int*					m_regionOffsets;			// spooling data offsets in LEVLUMP_REGIONS
	int*					m_roadOffsets;				// spooling data offsets in LEVLUMP_ROADS
	int*					m_occluderOffsets;			// spooling data offsets in LEVLUMP_OCCLUDERS

	int						m_numRegions;				// real region count in this level
	int						m_regionDataLumpOffset;		// region lump offset
	int						m_roadDataLumpOffset;		// road lump offset
	int						m_occluderDataLumpOffset;	// occluders offset

	EqString				m_levelName;

	IVertexBuffer*			m_instanceBuffer;

	DkList<IVector2D>		m_navOpenSet;
	ubyte					m_defaultNavTile;
};

#endif // LEVEL_H
