//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Level region
//////////////////////////////////////////////////////////////////////////////////

#ifndef REGION_H
#define REGION_H

#include "utils/eqthread.h"
using namespace Threading;

#include "math/Vector.h"
#include "occluderSet.h"
#include "DrvSynDecals.h"

#include "utils/strtools.h"

#ifdef EDITOR
#include "../DriversEditor/EditorActionHistory.h"
#endif // EDITOR

#define ENGINE_REGION_MAX_HFIELDS		4	// you could make more
#define MAX_MODEL_INSTANCES				1024

class CGameObject;
class CEqCollisionObject;
class CGameLevel;
class CLevObjectDef;
class IVirtualStream;
class CHeightTileFieldRenderable;

//----------------------------------------------------------------------

struct navcell_t
{
	// Position is not necessary in this form
	navcell_t()
	{
		Reset();
	}

	void Reset()
	{
		flags = 0;
		parentDir = 7;

		f = 0.0f;
		g = 0.0f;
		h = 0.0f;
	}

	ubyte		parentDir : 3;	// parent direction (1,2,3,4)
	ubyte		flags : 2;		// 0 = neither, 1 = closed, 2 = in open

	// cost factors
	float		f;
	float		g;
	float		h;
};

struct navGrid_t
{
	navGrid_t()
	{
		staticObst = NULL;
		cellStates = NULL;
		dynamicObst = NULL;
		debugObstacleMap = NULL;
		wide = 0;
		tall = 0;
		dirty = false;
	}

	~navGrid_t()
	{
		Cleanup();
	}

	void Init( int w, int h)
	{
		if(staticObst || cellStates || dynamicObst )
			Cleanup();

		wide = w;
		tall = h;

		staticObst = PPAllocStructArray(ubyte,w*h);
		memset(staticObst, 0x4, w*h);

		dynamicObst = PPAllocStructArray(ubyte, w*h);

		cellStates = PPAllocStructArray(navcell_t, w*h);

		dirty = true;
	}

	void ResetStates()
	{
		if(!cellStates)
			return;

		int count = wide*tall;
		for(int i = 0; i < count; i++)
			cellStates[i].Reset();
	}

	void ResetDynamicObst()
	{
		if(!dynamicObst)
			return;

		int count = wide*tall;
		memset(dynamicObst, 0x4, count);
	}

	void Cleanup()
	{
		PPFree(staticObst);
		staticObst = NULL;

		PPFree(cellStates);
		cellStates = NULL;

		PPFree(dynamicObst);
		dynamicObst = NULL;

		wide = 0;
		tall = 0;

		dirty = false;
	}

	ubyte*		staticObst;		///< A* static navigation grid
	ubyte*		dynamicObst;
	navcell_t*	cellStates;		///< A* open/close state list

	ITexture*	debugObstacleMap;

	ushort		wide;
	ushort		tall;

	bool		dirty;
};

//----------------------------------------------------------------------

class CLevelModel;

#ifdef EDITOR
class regionObject_t : public CUndoableObject
{
protected:
	static CUndoableObject*	_regionObjectFactory(IVirtualStream* stream);

	UndoableFactoryFunc	Undoable_GetFactoryFunc();
	void				Undoable_Remove();
	bool				Undoable_WriteObjectData(IVirtualStream* stream);	// writing object
	void				Undoable_ReadObjectData(IVirtualStream* stream);	// reading object
public:

#else
struct regionObject_t
{
#endif // 
	regionObject_t()
	{
		game_objectId = -1;
		def = NULL;
		static_phys_object = NULL;
		regionIdx = -1;
#ifdef EDITOR
		hide = false;
#endif // EDITOR
	}

	~regionObject_t();

	void RemoveGameObject();
	void CalcBoundingBox();

	Matrix4x4		transform;
	BoundingBox		bbox;

	CLevObjectDef*			def;
	CEqCollisionObject*		static_phys_object;

	Vector3D		position;
	Vector3D		rotation;

	EqString		name;

	int				game_objectId;

	int				regionIdx;

	ushort			tile_x;
	ushort			tile_y;

#ifdef EDITOR
	bool			hide;
#endif // EDITOR
};

struct regZone_t
{
	regZone_t(char* name)
	{
		zoneName = xstrdup(name);
	}

	regZone_t()
	{
		zoneName = NULL;
	}

	char*	zoneName;
	uint	zoneHash;
};

struct buildingSource_t;

//----------------------------------------------------------------------

class CLevelRegion
{
	friend class CGameLevel;

public:
	PPMEM_MANAGED_OBJECT();

	CLevelRegion();
	virtual ~CLevelRegion();

	void							Init(int cellsSize, const IVector2D& regPos, const Vector3D& hfieldPos);

	void							InitNavigationGrid();
	virtual void					Cleanup();

	bool							IsRegionEmpty();	///< returns true if no models or placed tiles of heightfield found here

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	virtual void					Render(const Vector3D& cameraPosition, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	Vector3D						CellToPosition(int x, int y) const;
	IVector2D						PositionToCell(const Vector3D& position) const;
	IVector2D						GetTileAndNeighbourRegion(int x, int y, CLevelRegion** reg) const;

	void							ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& models);
	void							ReadLoadRoads(IVirtualStream* stream);
	void							ReadLoadOccluders(IVirtualStream* stream);

	void							RespawnObjects();

	CHeightTileFieldRenderable*		GetHField( int idx = 0 ) const;
	int								GetNumHFields() const;
	int								GetNumNomEmptyHFields() const;

	void							GetDecalPolygons(decalPrimitives_t& polys, occludingFrustum_t* frustum);

	void							UpdateDebugMaps();

	bool							FindObject(levCellObject_t& objectInfo, const char* name, CLevObjectDef* def = NULL) const;

	//----------------------------------------------------

	static void						NavAddObstacleJob(void *data, int i);
	static void						InitRegionHeightfieldsJob(void *data, int i);

	BoundingBox						m_bbox;

	CHeightTileFieldRenderable*		m_heightfield[ENGINE_REGION_MAX_HFIELDS];	///< heightfield used on region

	levroadcell_t*					m_roads;			///< road data. Must correspond with heightfield]

	// two navigation grids here
	navGrid_t						m_navGrid[2];		///< navigation grid. 1 is detailed, 2 is for fast pathfind

#ifndef EDITOR
	DkList<regionObject_t*>			m_staticObjects;
#endif // EDITOR

	DkList<regionObject_t*>			m_objects;			///< complex and non-complex models
	DkList<levOccluderLine_t>		m_occluders;		///< occluders

	DkList<CLevObjectDef*>			m_regionDefs;		///< region object defs

	DkList<regZone_t>				m_zones;

	// TODO: road array, visibility data and other....
	int								m_regionIndex;

	volatile bool					m_isLoaded;
	bool							m_scriptEventCallbackCalled;
	bool							m_hasTransparentSubsets;

	CEqInterlockedInteger			m_queryTimes;		///< render (client) or physics engine query for server

	CGameLevel*						m_level;
};

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regionObject_t* ref);


#endif // REGION_H