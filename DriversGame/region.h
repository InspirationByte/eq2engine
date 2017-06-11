//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
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

		staticObst = new ubyte[w*h];
		memset(staticObst, 0x4, w*h);

		dynamicObst = new ubyte[w*h];
		memset(dynamicObst, 0x4, w*h);

		cellStates = new navcell_t[w*h];
		memset(cellStates, 0, w*h);

		dirty = true;
	}

	void Cleanup()
	{
		delete [] staticObst;
		staticObst = NULL;

		delete [] cellStates;
		cellStates = NULL;

		delete [] dynamicObst;
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

struct regionObject_t
{
	regionObject_t()
	{
		game_object = NULL;
		def = NULL;
		physObject = NULL;
#ifdef EDITOR
		hide = false;
#endif // EDITOR
	}

	~regionObject_t();

	void CalcBoundingBox();

	CLevObjectDef*			def;
	CGameObject*			game_object;
	CEqCollisionObject*		physObject;

	Matrix4x4		transform;

	ushort			tile_x;
	ushort			tile_y;

	BoundingBox		bbox;

	Vector3D		position;
	Vector3D		rotation;

	EqString		name;

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

	void							Init();
	void							InitRoads();
	virtual void					Cleanup();

	bool							IsRegionEmpty();	///< returns true if no models or placed tiles of heightfield found here

	void							CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition);
	virtual void					Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	Vector3D						CellToPosition(int x, int y) const;
	IVector2D						GetTileAndNeighbourRegion(int x, int y, CLevelRegion** reg) const;

	void							ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& models);
	void							ReadLoadRoads(IVirtualStream* stream);
	void							ReadLoadOccluders(IVirtualStream* stream);

	void							RespawnObjects();

	CHeightTileFieldRenderable*		GetHField( int idx = 0 ) const;
	int								GetNumHFields() const;
	int								GetNumNomEmptyHFields() const;

	void							GetDecalPolygons(decalprimitives_t& polys, occludingFrustum_t* frustum);

	void							UpdateDebugMaps();

	//----------------------------------------------------

	BoundingBox						m_bbox;

	CHeightTileFieldRenderable*		m_heightfield[ENGINE_REGION_MAX_HFIELDS];	///< heightfield used on region

	levroadcell_t*					m_roads;			///< road data. Must correspond with heightfield
	navGrid_t						m_navGrid;

	DkList<regionObject_t*>			m_objects;			///< complex and non-complex models
	DkList<levOccluderLine_t>		m_occluders;		///< occluders

	DkList<CLevObjectDef*>			m_regionDefs;		///< region object defs

	DkList<regZone_t>				m_zones;

	// TODO: road array, visibility data and other....
	bool							m_render;
	bool							m_isLoaded;
	int								m_regionIndex;

	bool							m_scriptEventCallbackCalled;
	bool							m_hasTransparentSubsets;

	CEqInterlockedInteger			m_queryTimes;		///< render (client) or physics engine query for server

	CGameLevel*						m_level;
};

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regionObject_t* ref);


#endif // REGION_H