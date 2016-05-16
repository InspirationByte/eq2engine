//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game level data
//////////////////////////////////////////////////////////////////////////////////

#ifndef IENGINEWORLD_H
#define IENGINEWORLD_H

#include "eqlevel.h"
#include "math/BoundingBox.h"
#include "Decals.h"

#define IEQLEVEL_INTERFACE_VERSION "IEqLevel_001"

#define MAX_PORTAL_PLANES	20

// render area plane set for engine
struct areaPlaneSet_t
{
	areaPlaneSet_t()
	{
		numPortalPlanes = 0;
	}

	bool IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps = 0.0f);
	bool IsSphereInside(const Vector3D &origin, const float radius);

	Plane		portalPlanes[MAX_PORTAL_PLANES+1];	// culler
	int			numPortalPlanes;

	// TODO: antiportals?
};

inline bool areaPlaneSet_t::IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps)
{
	// just like frustum check
	for (int i = 0; i < numPortalPlanes; i++)
	{
		if (portalPlanes[i].Distance( Vector3D(minX, minY, minZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(minX, minY, maxZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(minX, maxY, minZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(minX, maxY, maxZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(maxX, minY, minZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(maxX, minY, maxZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(maxX, maxY, minZ) ) > -eps) continue;
		if (portalPlanes[i].Distance( Vector3D(maxX, maxY, maxZ) ) > -eps) continue;

		return false;
	}

	return true;
}

inline bool areaPlaneSet_t::IsSphereInside(const Vector3D &origin, const float radius)
{
	// just like frustum check
	for (int i = 0; i < numPortalPlanes; i++)
	{
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;
		if (portalPlanes[i].Distance( origin ) > -radius) continue;

		return false;
	}

	return true;
}

// render area for engine
struct renderArea_t
{
	renderArea_t()
	{
		doRender = false;
		planeSets.resize(4);	// 4 doors maximum to one room
	}

	bool IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps = 0.0f);
	bool IsSphereInside(const Vector3D &origin, const float radius);

	int room;	// room index assigned

	DkList<areaPlaneSet_t>	planeSets;

	// TODO: renderable list sets here!

	bool doRender;
};

inline bool renderArea_t::IsBoxInside(const float minX, const float maxX, const float minY, const float maxY, const float minZ, const float maxZ, const float eps)
{
	if(planeSets.numElem() == 0)
		return true;

	for (int i = 0; i < planeSets.numElem(); i++)
	{
		if (planeSets[i].IsBoxInside(minX, maxX, minY, maxY, minZ, maxZ, eps))
			return true;
	}

	return false;
}

inline bool renderArea_t::IsSphereInside(const Vector3D &origin, const float radius)
{
	if(planeSets.numElem() == 0)
		return true;

	for (int i = 0; i < planeSets.numElem(); i++)
	{
		if (planeSets[i].IsSphereInside(origin, radius))
			return true;
	}

	return false;
}

class CBaseRenderableObject;
class IVertexFormat;

// pre-computed render area list for engine optimization
struct renderAreaList_t
{
	bool updated;
	renderArea_t* area_list;
};

//---------------------------------------------------------------------------------------------

class IWaterInfo
{
public:
	virtual float			GetWaterHeightLevel() = 0;
	virtual bool			IsPointInsideWater(const Vector3D& position, float radius = 0.0f) = 0;
	virtual BoundingBox		GetBounds() = 0;
};

//--------------------------------------------------------------------------------------------------

class IEqLevel : public ICoreModuleInterface
{
public:
	// check level for existance (needs for changelevel)
	virtual bool					CheckLevelExist(const char* pszLevelName) = 0;

	// is level loaded?
	virtual bool					IsLoaded() = 0;

//--------------------------------------------------------------------------------------------------

	// returns list of translucent renderable objects
	virtual CBaseRenderableObject**	GetStaticDecalRenderables() = 0;

	virtual int						GetStaticDecalCount() = 0;

	virtual IVertexFormat*			GetDecalVertexFormat() = 0;

	// makes dynamic temporary decal
	virtual tempdecal_t*			MakeTempDecal( const decalmakeinfo_t &info ) = 0;

	// makes static decal (and updates buffers, slow on creation)
	virtual staticdecal_t*			MakeStaticDecal( const decalmakeinfo_t &info ) = 0;

	virtual void					FreeStaticDecal( staticdecal_t* pDecal ) = 0;

	virtual void					UpdateStaticDecals() = 0;

//--------------------------------------------------------------------------------------------------

	// returns 1 or 2 rooms for point (if inside a split of portal). Returns count of rooms (-1 is outside)
	virtual int						GetRoomsForPoint( const Vector3D &point, int room_ids[2]) = 0;

	// bbox checking for rooms (can return more than 2 rooms)
	virtual int						GetRoomsForBBox( const Vector3D &point, int* room_ids) = 0;

	// bbox checking for rooms (can return more than 2 rooms)
	virtual int						GetRoomsForSphere( const Vector3D &point, float radius, int* room_ids) = 0;

	// returns most inside room volume
	//virtual int					GetRoomVolumeByPoint

	// returns first near portal between rooms (-1 if none linked)
	virtual int						GetFirstPortalLinkedToRoom( int startRoom, int endRoom, Vector3D& orient_to_pos, int maxDepth = 2 ) = 0;

	// checks portal linkage in specified depth
	virtual bool					CheckPortalLink( int startroom, int targetroom, int maxDepth ) = 0;

	// returns center of the portal
	virtual Vector3D				GetPortalPosition( int nPortal ) = 0;

	// returns level room count
	virtual int						GetRoomCount() = 0;

	// load room materials
	virtual void					LoadRoomMaterials( int targetroom ) = 0;

	// unloads materials in room
	virtual void					UnloadRoomMaterials( int targetroom ) = 0;

//--------------------------------------------------------------------------------------------------

	// makes new render area list
	virtual renderAreaList_t*		CreateRenderAreaList() = 0;

	// releases area list
	virtual void					DestroyRenderAreaList(renderAreaList_t* pList) = 0;

//--------------------------------------------------------------------------------------------------

	// returns list of translucent renderable objects
	virtual CBaseRenderableObject**	GetTranslucentObjectRenderables() = 0;

	virtual int						GetTranslucentObjectCount() = 0;

//--------------------------------------------------------------------------------------------------

	// for AI navigation calculation
	virtual int						GetPhysicsSurfaces(eqphysicsvertex_t** ppVerts, int** ppIndices, eqlevelphyssurf_t** ppSurfs, int &numIndices) = 0;

//--------------------------------------------------------------------------------------------------

	// water info for physics objects and rendering
	virtual IWaterInfo*				GetNearestWaterInfo(const Vector3D& position) = 0;
};

extern IEqLevel* eqlevel;

#endif // IENGINEWORLD_H
