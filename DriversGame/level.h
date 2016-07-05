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

#include "imaging/ImageLoader.h"

#define AI_NAVIGATION_GRID_SCALE	2

#define ROADNEIGHBOUR_OFFS_X(x)		{x, x-1, x, x+1}		// non-diagonal
#define ROADNEIGHBOUR_OFFS_Y(y)		{y-1, y, y+1, y}

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
	virtual ~CGameLevel() {}

	void					Init(int wide, int tall, int cells, bool isCleanLevel);	// number of height fields
	void					Cleanup();

	bool					Load(const char* levelname, kvkeybase_t* kvDefs);

	//---------------------------------

	//
	// Region indexes
	//

	CHeightTileFieldRenderable*		GetHeightFieldAt(const IVector2D& XYPos, int idx = 0) const;
	CLevelRegion*					GetRegionAt(const IVector2D& XYPos) const;

	// resolving indexes
	bool					GetPointAt(const Vector3D& pos, IVector2D& outXYPos) const;
	CLevelRegion*			GetRegionAtPosition(const Vector3D& pos) const;

	//
	// Global (per tile) indexes
	//
	bool					GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const;

	bool					GetRegionAndTile(const Vector3D& pos, CLevelRegion** pReg, IVector2D& outLocalXYPos) const;
	bool					GetRegionAndTileAt(const IVector2D& point, CLevelRegion** pReg, IVector2D& outLocalXYPos) const;

	//
	// conversions
	//
	IVector2D				PositionToGlobalTilePoint( const Vector3D& pos ) const;
	Vector3D				GlobalTilePointToPosition( const IVector2D& point ) const;

	void					GlobalToLocalPoint( const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion ) const;
	void					LocalToGlobalPoint( const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	levroadcell_t*			GetGlobalRoadTile(const Vector3D& pos, CLevelRegion** pRegion = NULL) const;
	levroadcell_t*			GetGlobalRoadTileAt(const IVector2D& point, CLevelRegion** pRegion = NULL) const;

	//---------------------------------

	straight_t				GetStraightAtPoint( const IVector2D& point, int numIterations = 4 ) const;
	straight_t				GetStraightAtPos( const Vector3D& pos, int numIterations = 4 ) const;

	roadJunction_t			GetJunctionAtPoint( const IVector2D& point, int numIterations = 4 ) const;
	roadJunction_t			GetJunctionAtPos( const Vector3D& pos, int numIterations = 4 ) const;

	// current road lane
	int						GetLaneIndexAtPoint( const IVector2D& point, int numIterations = 8 );
	int						GetLaneIndexAtPos( const Vector3D& pos, int numIterations = 8 );

	// road width in lane count
	int						GetNumLanesAtPoint( const IVector2D& point, int numIterations = 8 );
	int						GetNumLanesAtPos( const Vector3D& pos, int numIterations = 8 );

	int						GetRoadWidthInLanesAtPoint( const IVector2D& point, int numIterations = 16 );
	int						GetRoadWidthInLanesAtPos( const Vector3D& pos, int numIterations = 16 );


	//---------------------------------

	void					PreloadRegion(int x, int y);

	void					QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);
	void					QueryNearestRegions(const IVector2D& point, bool waitLoad = false);

	void					CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	void					Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	int						UpdateRegions( RegionLoadUnloadCallbackFunc func = NULL);
	void					RespawnAllObjects();

	void					Nav_AddObstacle(CLevelRegion* reg, regionObject_t* ref);

	//
	// conversions
	//
	void					Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2) const;

	IVector2D				Nav_PositionToGlobalNavPoint(const Vector3D& pos) const;

	Vector3D				Nav_GlobalPointToPosition(const IVector2D& point) const;

	void					Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const;
	void					Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const;

	navcell_t&				Nav_GetCellStateAtGlobalPoint(const IVector2D& point);
	ubyte&					Nav_GetTileAtGlobalPoint(const IVector2D& point);

	bool					Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);
	bool					Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit = 256, bool cellPriority = false);

	void					Nav_ClearCellStates();

	// TODO: render code

	DkList<CLevObjectDef*>	m_objectDefs;

	int						m_wide;
	int						m_tall;

	int						m_cellsSize;

	CEqMutex&				m_mutex;

protected:

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
};

#endif // LEVEL_H
