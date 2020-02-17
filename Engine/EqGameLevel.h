//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game level data
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQGAMELEVEL_H
#define EQGAMELEVEL_H

#include "Utils/eqstring.h"
#include "Utils/KeyValues.h"
#include "IEngineWorld.h"
#include "ppmem.h"

class IMaterial;
class ITexture;

class IVertexFormat;
class IVertexBuffer;
class IIndexBuffer;

class CWorldDecal;

struct eqdrawsurfaceunion_t
{
	int material_id;

	int first_vertex;
	int first_index;
	int numindices;
	int numverts;

	int flags;

	BoundingBox bbox;
};

//---------------------------------------------------------------------------------------------

// room volume
class CEqRoomVolume
{
public:
	PPMEM_MANAGED_OBJECT();

	CEqRoomVolume();

	void					LoadRenderData(int room_id, eqroomvolume_t* volume, eqworldlump_t* pSurfaceLumps, eqworldlump_t* pPlanes);
	void					FreeData();

	bool					IsPointInsideVolume(const Vector3D& position, float radius = 0.0f);

	eqroomvolume_t			volume_info;

	eqlevelsurf_t*			m_pDrawSurfaceGroups;
	int						m_numDrawSurfaceGroups;

	DkList<eqdrawsurfaceunion_t*> m_drawSurfaceUnions;

	BoundingBox				m_bounding;

	Plane*					m_pPlanes;

	DkList<staticdecal_t*>	m_surfaceDecals;
};

// room
class CEqRoom
{
public:
	PPMEM_MANAGED_OBJECT();

	CEqRoom();

	void					Init(eqroom_t* info);

	void					FreeData();

	bool					IsPointInsideRoom(const Vector3D& position, float radius = 0.0f);

	eqroom_t				room_info;

	CEqRoomVolume*			m_pRoomVolumes;
	int						m_numRoomVolumes;

	// portal side index for the room (to correct portal usage)
	ubyte					m_pPortalSides[MAX_LINKED_PORTALS];

	BoundingBox				m_bounding;

	DkList<staticdecal_t*>	m_roomDecals;
};

// area portal
class CEqAreaPortal
{
public:
	PPMEM_MANAGED_OBJECT();

	CEqAreaPortal();

	void					Init(eqareaportal_t *portal, eqworldlump_t* pVerts, eqworldlump_t* pPlanes);
	void					FreeData();

	Vector3D*				m_vertices[2];
	Plane					m_planes[2];
	eqareaportal_t			portal_info;

	BoundingBox				m_bounding;
};

//---------------------------------------------------------------------------------------------

// water volume
class CEqWaterVolume
{
public:
	PPMEM_MANAGED_OBJECT();

	CEqWaterVolume();

	void					LoadData(int water_id, eqroomvolume_t* volume, eqworldlump_t* pPlanes);
	void					FreeData();

	bool					IsPointInsideVolume(const Vector3D& position, float radius = 0.0f);

	eqroomvolume_t			volume_info;

	BoundingBox				m_bounding;

	Plane*					m_pPlanes;
};

// room
class CEqWaterInfo : public IWaterInfo
{
public:
	PPMEM_MANAGED_OBJECT();

	CEqWaterInfo();

	void					Init(eqwaterinfo_t* info);

	void					FreeData();

	bool					IsPointInsideWater(const Vector3D& position, float radius = 0.0f);

	float					GetWaterHeightLevel();

	BoundingBox				GetBounds();

	eqwaterinfo_t			water_info;

	CEqWaterVolume*			m_pWaterVolumes;
	int						m_numWaterVolumes;

	BoundingBox				m_bounding;
};

//---------------------------------------------------------------------------------------------

// eq level data
class CEqLevel : public IEqLevel
{
	friend class IEngineGame;

public:
	CEqLevel();
	~CEqLevel();

	bool							IsInitialized() const {return true;}
	const char*						GetInterfaceName() const {return IEQLEVEL_INTERFACE_VERSION;};

	bool							CheckLevelExist(const char* pszLevelName);

	bool							LoadLevel(const char* pszLevelName);

	bool							LoadCompatibleWorldFile(const char* pszFileName);

	void							InitMaterials(eqworldlump_t* pMaterials);
	void							InitRenderBuffers(eqworldlump_t* pVerts, eqworldlump_t* pIndices);
	void							LoadPhysicsData(eqworldlump_t* pSurf, eqworldlump_t* pVerts, eqworldlump_t* pIndices);
	void							LoadOcclusion(eqworldlump_t* pSurf, eqworldlump_t* pVerts, eqworldlump_t* pIndices);

	bool							LoadEntities(eqworldlump_t* pLump);

	void							LoadRoomsData(eqworldlump_t* pRooms, eqworldlump_t* pVolumes, eqworldlump_t* pPlanes, eqworldlump_t** pSurfaceLumps);
	void							LoadPortals(eqworldlump_t* pPortals, eqworldlump_t* pVerts, eqworldlump_t* pPlanes);

	void							LoadWaterInfos(eqworldlump_t* pWaterInfos, eqworldlump_t* pWaterVolumes, eqworldlump_t* pWaterPlanes);

	void							LoadLightmaps();

	void							UnloadLevel();

	bool							IsLoaded();

	// returns 1 or 2 rooms for point (if inside a split of portal). Returns count of rooms (0 is outside)
	int								GetRoomsForPoint(const Vector3D &point, int room_ids[2]);

	// bbox checking for rooms (can return more than 2 rooms)
	int								GetRoomsForBBox(const Vector3D &point, int* room_ids);

	// bbox checking for rooms (can return more than 2 rooms)
	int								GetRoomsForSphere(const Vector3D &point, float radius, int* room_ids);

	// returns portal between rooms (-1 if none linked)
	int								GetFirstPortalLinkedToRoom( int startRoom, int endRoom, const Vector3D& orient_to_pos, int maxDepth = 2 );

	// returns center of the portal
	Vector3D						GetPortalPosition( int nPortal );

	int								GetRoomCount();

	bool							CheckPortalLink_r( CEqRoom* pPrevious, CEqRoom* pCurRoom, CEqRoom* pCheckRoom, int nDepth, int maxDepth );

	bool							CheckPortalLink( int startroom, int targetroom, int maxDepth );

	// load room materials
	void							LoadRoomMaterials( int targetroom );

	// unloads materials in room
	void							UnloadRoomMaterials(int targetroom);

	// makes new render area list
	renderAreaList_t*				CreateRenderAreaList();

	// releases area list
	void							DestroyRenderAreaList(renderAreaList_t* pList);

	// makes dynamic temporary decal
	tempdecal_t*					MakeTempDecal( const decalmakeinfo_t &info );

	// makes static decal (and updates buffers, slow on creation)
	staticdecal_t*					MakeStaticDecal( const decalmakeinfo_t &info );

	void							FreeStaticDecal( staticdecal_t* pDecal );

	void							UpdateStaticDecals();

	// returns list of translucent renderable objects
	CBaseRenderableObject**			GetTranslucentObjectRenderables();

	int								GetTranslucentObjectCount();

	// returns list of translucent renderable objects
	CBaseRenderableObject**			GetStaticDecalRenderables();

	int								GetStaticDecalCount();

	IVertexFormat*					GetDecalVertexFormat();

	int								GetPhysicsSurfaces(eqphysicsvertex_t** ppVerts, int** ppIndices, eqlevelphyssurf_t** ppSurfs, int &numIndices);

	// water info for physics objects and rendering
	IWaterInfo*						GetNearestWaterInfo(const Vector3D& position);

//protected:
public:

	CEqRoom*						m_pRooms;
	int								m_numRooms;

	CEqAreaPortal*					m_pPortals;
	int								m_numPortals;

	KeyValues						m_EntityKeyValues;

	IVertexFormat*					m_pVertexFormat_Lit;
	IVertexFormat*					m_pVertexFormat_Color;

	IVertexBuffer*					m_pVertexBuffer;
	IIndexBuffer*					m_pIndexBuffer;

	bool							m_bIsLoaded;

	EqString						m_levelpath;

	IMaterial**						m_pMaterials;
	//int*							m_nMaterialRenderCounter;
	int								m_numMaterials;

	int								m_numLightmaps;
	int								m_nLightmapSize;

	ITexture**						m_pLightmaps;
	ITexture**						m_pDirmaps;

	bool							m_bOcclusionAvailable;

	eqocclusionsurf_t*				m_pOcclusionSurfaces;
	int								m_numOcclusionSurfs;
	Vector3D*						m_vOcclusionVerts;
	int*							m_nOcclusionIndices;

	eqlevellight_t*					m_pLevelLights;
	int								m_numLevelLights;

	// this data is needed by physics engine and... maybe... ai mesh builder
	eqphysicsvertex_t*				m_pPhysVertexData;
	int*							m_pPhysIndexData;
	int								m_numPhysIndices;

	eqlevelphyssurf_t*				m_pPhysSurfaces;
	int								m_numPhysSurfaces;

	// decals
	IVertexFormat*					m_pDecalVertexFormat;

	IVertexBuffer*					m_pDecalVertexBuffer_Static;
	IIndexBuffer*					m_pDecalIndexBuffer_Static;

	CEqWaterInfo*					m_pWaterInfos;
	int								m_nWaterInfoCount;

	DkList<CWorldDecal*>			m_StaticDecals;
	DkList<CBaseRenderableObject*>	m_StaticTransparents;

protected:

	eqlevelvertexlm_t*				m_pVertices;
	int*							m_pIndices;

	DkList<renderAreaList_t*>		m_RenderAreas;
	DkList<IPhysicsObject*>			m_pPhysicsObjects;
};

extern CEqLevel* g_pLevel;

#endif // EQGAMELEVEL_H