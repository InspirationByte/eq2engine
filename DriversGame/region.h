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
		if(staticObst || cellStates )
			Cleanup();

		wide = w;
		tall = h;

		staticObst = new ubyte[w*h];
		memset(staticObst, 0x4, w*h);

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

		wide = 0;
		tall = 0;

		dirty = false;
	}

	ubyte*		staticObst;		///< A* static navigation grid
	navcell_t*	cellStates;		///< A* open/close state list

	ushort		wide;
	ushort		tall;
	
	bool		dirty;
};

//----------------------------------------------------------------------

struct regionObject_t
{
	PPMEM_MANAGED_OBJECT()

	regionObject_t()
	{
		game_object = NULL;
		def = NULL;
	}

	~regionObject_t();

	CLevObjectDef*	def;

	CGameObject*	game_object;

	Vector3D		position;
	Vector3D		rotation;

	Matrix4x4		transform;

	short			tile_x;
	short			tile_y;
	bool			tile_dependent;

	int				level;

	BoundingBox		bbox;
	EqString		name;

	DkList<CEqCollisionObject*> collisionObjects;
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

	void							WriteRegionData(IVirtualStream* stream, DkList<CLevObjectDef*>& models, bool final);
	void							ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& models);

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
	int								Ed_ReplaceDefs(CLevObjectDef* whichReplace, CLevObjectDef* replaceTo);

	bool							m_modified; // needs saving?
#endif

	BoundingBox						m_bbox;

	CHeightTileFieldRenderable*		m_heightfield[ENGINE_REGION_MAX_HFIELDS];	///< heightfield used on region

	levroadcell_t*					m_roads;			///< road data. Must correspond with heightfield
	navGrid_t						m_navGrid;

	DkList<regionObject_t*>			m_objects;			///< complex and non-complex models
	DkList<levOccluderLine_t>		m_occluders;		///< occluders

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