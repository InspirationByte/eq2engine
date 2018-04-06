//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game level data
//////////////////////////////////////////////////////////////////////////////////

#include "EqGameLevel.h"
#include "DebugInterface.h"
#include "materialsystem/IMaterialSystem.h"
#include "BaseRenderableObject.h"
#include "ViewRenderer.h"
#include "utils/geomtools.h"
#include "Utils/eqthread.h"
#include "mtriangle_framework.h"
#include "Utils/SmartPtr.h"

//#include "DebugOverlay.h"
#include "idkphysics.h"

extern ConVar r_notempdecals;

using namespace Threading;
using namespace MTriangle;

// the transparents that can be sorted
class CWorldTransparentRenderable : public CBaseRenderableObject
{
public:
	CWorldTransparentRenderable()
	{
		m_nRenderFlags = RF_SKIPVISIBILITYTEST | RF_DISABLE_SHADOWS;
		//m_nVisibilityStatus = VISIBILITY_NOT_TESTED;
	}

	// renders this object
	void Render( int nViewRenderFlags )
	{
		CViewRenderer* pVR = (CViewRenderer*)viewrenderer;

		if( !pVR->GetViewFrustum()->IsBoxInside(info.mins.x,info.maxs.x, info.mins.y,info.maxs.y, info.mins.z,info.maxs.z) )
			return;

		if(pVR->GetAreaList() && pVR->GetAreaList()->area_list[m_nRoomID].doRender)
		{
			g_pShaderAPI->SetVertexFormat(g_pLevel->m_pVertexFormat_Lit);
			g_pShaderAPI->SetVertexBuffer(g_pLevel->m_pVertexBuffer, 0);
			g_pShaderAPI->SetIndexBuffer(g_pLevel->m_pIndexBuffer);

			materials->SetAmbientColor( ColorRGBA(pVR->GetSceneInfo().m_cAmbientColor, 1) );

			materials->SetMatrix(MATRIXMODE_WORLD, identity4());
			materials->SetCullMode(CULL_BACK);

			g_pShaderAPI->ApplyBuffers();

			renderArea_t* pArea = &pVR->GetAreaList()->area_list[m_nRoomID];
			pVR->DrawSurfaceEx( pArea, &info, nViewRenderFlags);
		}
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return identity4();
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return info.mins;
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return info.maxs;
	}

	// retruns 1 or 2 rooms. 0 is outside
	int	GetRoomIndex(int *rooms)
	{
		rooms[0] = m_nRoomID;
		return 1;
	}

	eqlevelsurf_t		info;
	int					m_nRoomID;
};

ConVar r_drawstaticdecals("r_drawstaticdecals", "1", "Draw static decals", CV_CHEAT);

// the transparents that can be sorted
class CWorldDecal : public CBaseRenderableObject
{
public:
	CWorldDecal()
	{
		m_nRenderFlags = RF_SKIPVISIBILITYTEST | RF_DISABLE_SHADOWS;
		
//		m_nVisibilityStatus = VISIBILITY_NOT_TESTED;
	}

	~CWorldDecal()
	{

	}

	void Init()
	{
		m_nRooms = eqlevel->GetRoomsForSphere( decal.box.GetCenter(), length(decal.box.GetSize()), m_pRooms );
	}

	// renders this object
	void Render(int nViewRenderFlags)
	{
		//if(!(decal.flags & DECAL_FLAG_VISIBLE))
		//	return;

		int decalMaterialFlags = decal.material->GetFlags();

		if((decalMaterialFlags & MATERIAL_FLAG_MODULATE) && (nViewRenderFlags & VR_FLAG_NO_MODULATEDECALS))
			return;

		if(!(decalMaterialFlags & MATERIAL_FLAG_MODULATE) && (nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT))
			return;

		if(!r_drawstaticdecals.GetBool())
			return;

		Matrix4x4 wvp;

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		materials->GetWorldViewProjection( wvp );

		Volume frustum;
		frustum.LoadAsFrustum( wvp );

		Vector3D mins = GetBBoxMins();
		Vector3D maxs = GetBBoxMaxs();

		// check frustum visibility
		if(!frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
			return;

		// check visibility
		if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
		{
			Vector3D bbox_center = (mins+maxs)*0.5f;
			float bbox_size = length(mins-maxs);

			int view_rooms[2] = {-1, -1,};
			viewrenderer->GetViewRooms(view_rooms);

			int cntr = 0;

			for(int i = 0; i < m_nRooms; i++)
			{
				if(m_pRooms[i] != -1)
				{
					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
					{
						cntr++;
						continue;
					}

					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(decal.box.GetCenter(), bbox_size))
					{
						cntr++;
						continue;
					}
				}
			}

			if(cntr == m_nRooms)
				return;
		}

		g_pShaderAPI->SetVertexFormat(g_pLevel->m_pDecalVertexFormat);
		g_pShaderAPI->SetVertexBuffer(g_pLevel->m_pDecalVertexBuffer_Static, 0);
		g_pShaderAPI->SetIndexBuffer(g_pLevel->m_pDecalIndexBuffer_Static);

		materials->SetAmbientColor(ColorRGBA(viewrenderer->GetSceneInfo().m_cAmbientColor, 1));

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		materials->SetCullMode(CULL_BACK);

		materials->BindMaterial( decal.material, false );

		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, decal.firstIndex, decal.numIndices, decal.firstVertex, decal.numVerts);
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return identity4();
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return decal.box.minPoint;
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return decal.box.maxPoint;
	}

	int					m_pRooms[32];
	int					m_nRooms;

	staticdecal_t	decal;
};

static CEqLevel s_Level;
CEqLevel* g_pLevel = &s_Level;
IEqLevel* eqlevel = g_pLevel;

CEqLevel::CEqLevel()
{
	m_pVertexFormat_Lit = NULL;

	m_bOcclusionAvailable = false;
	m_pOcclusionSurfaces = NULL;
	m_numOcclusionSurfs = 0;
	m_vOcclusionVerts = 0;
	m_nOcclusionIndices = 0;

	m_pRooms = NULL;
	m_numRooms = 0;

	m_pWaterInfos = NULL;
	m_nWaterInfoCount = 0;

	m_pPortals = NULL;
	m_numPortals = 0;

	m_pMaterials = NULL;
	m_numMaterials = 0;

	m_numLightmaps = 0;

	m_pLightmaps = NULL;

	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	m_pVertices = NULL;
	m_pIndices = NULL;

	m_pLevelLights = NULL;
	m_numLevelLights = 0;

	// decals
	m_pDecalVertexFormat = NULL;

	m_pDecalVertexBuffer_Static = NULL;
	m_pDecalIndexBuffer_Static = NULL;

	m_bIsLoaded = false;

	GetCore()->RegisterInterface( IEQLEVEL_INTERFACE_VERSION, this );
}

CEqLevel::~CEqLevel()
{
	GetCore()->UnregisterInterface( IEQLEVEL_INTERFACE_VERSION );
}

bool CEqLevel::CheckLevelExist(const char* pszLevelName)
{
	EqString leveldir(EqString("worlds/")+pszLevelName);

	// make a file	
	EqString game_level_file(leveldir + "/world.build");

	return g_fileSystem->FileExist(game_level_file.GetData());
		
}

bool CEqLevel::LoadLevel(const char* pszLevelName)
{
	m_levelpath = pszLevelName;

	if(	!LoadCompatibleWorldFile("world.build"))
		return false;

	if(	!g_pLevel->LoadCompatibleWorldFile("geometry.build"))
		return false;

	if(	!LoadCompatibleWorldFile("physics.build"))
		return false;

	// also try to load occlusion data
	//if(!LoadCompatibleWorldFile("occlusion.build"))
	//	MsgWarning("Occlusion not available\n");

	m_bIsLoaded = true;

	return true;
}

// Sector data format lumps
char* EQWORLD_LUMP_NAMES[] = 
{
	"EQWLUMP_SURFACES",
	"EQWLUMP_VERTICES",
	"EQWLUMP_INDICES",
	"EQWLUMP_MATERIALS",
	"EQWLUMP_ENTITIES",				// default entities for the sector

	"EQWLUMP_PHYS_SURFACES",
	"EQWLUMP_PHYS_VERTICES",
	"EQWLUMP_PHYS_INDICES",

	"EQWLUMP_OCCLUSION_SURFACES",
	"EQWLUMP_OCCLUSION_VERTS",
	"EQWLUMP_OCCLUSION_INDICES",

	"EQWLUMP_SECTORINFO",

	"EQWLUMP_LIGHTMAPINFO",			// lightmap information
	"EQWLUMP_LIGHTS",				// lightmap light data

	"EQWLUMP_COUNT",
};

bool CEqLevel::LoadCompatibleWorldFile(const char* pszFileName)
{
	EqString leveldir(EqString("worlds/")+m_levelpath);

	// make a file	
	EqString game_level_file(leveldir + "/" + pszFileName);

	ubyte* pLevelData = (ubyte*)g_fileSystem->GetFileBuffer(game_level_file.GetData(), 0, -1, true);

	if(!pLevelData)
	{
		Msg("File '%s' not found!\n", game_level_file.GetData());
		return false;
	}

	Msg("Loading world file '%s'\n", game_level_file.GetData());

	eqworldhdr_t* pHdr = (eqworldhdr_t*)pLevelData;
	if(pHdr->ident != EQWF_IDENT)
	{
		MsgError("ERROR: Invalid %s world file\n", pszFileName);
		PPFree(pLevelData);
		return false;
	}

	if(pHdr->version < EQWF_VERSION)
	{
		MsgError("ERROR: Invalid %s world version, please rebuild in editor\n", pszFileName);
		PPFree(pLevelData);
		return false;
	}

	if(pHdr->version > EQWF_VERSION)
	{
		MsgError("ERROR: Invalid %s world version, please update engine\n", pszFileName);
		PPFree(pLevelData);
		return false;
	}

	ubyte* pLump = ((ubyte*)pHdr + sizeof(eqworldhdr_t));

	DevMsg(DEVMSG_CORE, "Loading %s lumps (%d)...\n", pszFileName, pHdr->num_lumps);

	int nSectorSurfaceLumps = 0;

	DkList<eqworldlump_t*>			roomVolume_surface_pointers;

	eqworldlump_t* phys_surfaces	= NULL;
	eqworldlump_t* phys_verts		= NULL;
	eqworldlump_t* phys_indices		= NULL;

	eqworldlump_t* verts			= NULL;
	eqworldlump_t* indices			= NULL;

	eqworldlump_t* materialslump	= NULL;

	eqworldlump_t* occ_surfaces		= NULL;
	eqworldlump_t* occ_verts		= NULL;
	eqworldlump_t* occ_indices		= NULL;

	eqworldlump_t* rooms			= NULL;
	eqworldlump_t* volumes			= NULL;
	eqworldlump_t* portals			= NULL;
	eqworldlump_t* portalverts		= NULL;
	eqworldlump_t* planes			= NULL;

	eqworldlump_t* waterPlanes		= NULL;
	eqworldlump_t* waterInfos		= NULL;
	eqworldlump_t* waterVolumes		= NULL;

	// enumerate lumps
	for(int i = 0; i < pHdr->num_lumps; i++)
	{
		eqworldlump_t* lump_data = (eqworldlump_t*)pLump;

		int data_size = lump_data->data_size;

		DevMsg(DEVMSG_CORE, "Lump: %s\n", EQWORLD_LUMP_NAMES[lump_data->data_type]);

		switch(lump_data->data_type)
		{
			case EQWLUMP_ENTITIES:
				if(!LoadEntities(lump_data))
					return false;
				break;
			case EQWLUMP_LIGHTMAPINFO:
				{
				eqlevellightmapinfo_t *lm_info = (eqlevellightmapinfo_t*)(((ubyte*)lump_data)+sizeof(eqworldlump_t));
				m_numLightmaps = lm_info->numlightmaps;
				m_nLightmapSize = lm_info->lightmapsize;

#ifndef EQLC
				LoadLightmaps();
#endif
				}
				break;
#ifndef EQLC
			case EQWLUMP_LIGHTS:
				{
					eqlevellight_t *lights_data = (eqlevellight_t*)(((ubyte*)lump_data)+sizeof(eqworldlump_t));
					m_numLevelLights = lump_data->data_size / sizeof(eqlevellight_t);

					if(m_numLevelLights)
					{
						// copy lump if available
						m_pLevelLights = (eqlevellight_t*)PPAlloc( lump_data->data_size );
						memcpy(m_pLevelLights, lights_data, lump_data->data_size);
					}
				}
				break;
#endif
			case EQWLUMP_SURFACES:
				roomVolume_surface_pointers.append(lump_data);
				nSectorSurfaceLumps++;
				break;
			case EQWLUMP_ROOMS:
				rooms = lump_data;
				break;
			case EQWLUMP_ROOMVOLUMES:
				volumes = lump_data;
				break;
			case EQWLUMP_AREAPORTALS:
				portals	= lump_data;
				break;
			case EQWLUMP_AREAPORTALVERTS:
				portalverts	= lump_data;
				break;
			case EQWLUMP_PLANES:
				planes = lump_data;
				break;
			case EQWLUMP_VERTICES:
				verts = lump_data;
				break;
			case EQWLUMP_INDICES:
				indices = lump_data;
				break;
			case EQWLUMP_MATERIALS:
				materialslump = lump_data;
				break;
			case EQWLUMP_PHYS_SURFACES:
				phys_surfaces = lump_data;
				break;
			case EQWLUMP_PHYS_VERTICES:
				phys_verts = lump_data;
				break;
			case EQWLUMP_PHYS_INDICES:
				phys_indices = lump_data;
				break;
			case EQWLUMP_OCCLUSION_SURFACES:
				occ_surfaces = lump_data;
				break;
			case EQWLUMP_OCCLUSION_VERTS:
				occ_verts = lump_data;
				break;
			case EQWLUMP_OCCLUSION_INDICES:
				occ_indices = lump_data;
				break;
			case EQWLUMP_WATER_INFOS:
				waterInfos = lump_data;
				break;
			case EQWLUMP_WATER_PLANES:
				waterPlanes = lump_data;
				break;
			case EQWLUMP_WATER_VOLUMES:
				waterVolumes = lump_data;
				break;
		}

		// go to next lump
		pLump += sizeof(eqworldlump_t)+data_size;
	}

	if(phys_surfaces && phys_verts && phys_indices)
		LoadPhysicsData(phys_surfaces, phys_verts, phys_indices);

	if(materialslump)
		InitMaterials(materialslump);

	if(verts && indices)
		InitRenderBuffers(verts,indices);

	if(occ_surfaces && occ_verts && occ_indices)
		LoadOcclusion( occ_surfaces, occ_verts, occ_indices );

	if(rooms && volumes && planes && roomVolume_surface_pointers.numElem())
		LoadRoomsData( rooms, volumes, planes, roomVolume_surface_pointers.ptr() );

	if(portals && portalverts && planes)
		LoadPortals( portals, portalverts, planes );

	if(waterInfos && waterVolumes && waterPlanes)
		LoadWaterInfos(waterInfos, waterVolumes, waterPlanes);

	PPFree(pLevelData);

	return true;
}

bool CEqLevel::LoadEntities(eqworldlump_t* pLump)
{
	DevMsg(DEVMSG_CORE, "Loading entities\n");

	// load entities
	ubyte* pEntityData = (ubyte*)pLump + sizeof(eqworldlump_t);
	pEntityData[pLump->data_size] = 0;

	if(!m_EntityKeyValues.LoadFromStream( pEntityData ))
	{
		MsgError("Error while loading world objects\nWorld '%s' needs to be rebuilt with latest EqWC\n", m_levelpath.GetData());

		UnloadLevel();

		return false;
	}

	return true;
}

void CEqLevel::LoadWaterInfos(eqworldlump_t* pWaterInfos, eqworldlump_t* pWaterVolumes, eqworldlump_t* pWaterPlanes)
{
	// load sectors
	m_nWaterInfoCount = pWaterInfos->data_size / sizeof(eqwaterinfo_t);
	m_pWaterInfos = new CEqWaterInfo[m_nWaterInfoCount];

	eqwaterinfo_t* waterLumpData = (eqwaterinfo_t*)(((ubyte*)pWaterInfos) + sizeof(eqworldlump_t));
	eqroomvolume_t* volumeLumpData = (eqroomvolume_t*)(((ubyte*)pWaterVolumes) + sizeof(eqworldlump_t));

	int nVolume = 0;
	
	for(int i = 0; i < m_nWaterInfoCount; i++)
	{
		eqwaterinfo_t* pWater = &waterLumpData[i];

		m_pWaterInfos[i].Init(pWater);
		m_pWaterInfos[i].m_numWaterVolumes = pWater->numVolumes;
		m_pWaterInfos[i].m_pWaterVolumes = new CEqWaterVolume[pWater->numVolumes];

		for(int j = 0; j < waterLumpData[i].numVolumes; j++)
		{
			eqroomvolume_t* pVolume = &volumeLumpData[pWater->firstVolume + j];

			m_pWaterInfos[i].m_pWaterVolumes[j].LoadData( i, pVolume, pWaterPlanes);
			m_pWaterInfos[i].m_pWaterVolumes[j].volume_info = volumeLumpData[pWater->firstVolume + j];

			nVolume++;
		}
	}
}

void CEqLevel::LoadRoomsData(eqworldlump_t* pRooms, eqworldlump_t* pVolumes, eqworldlump_t* pPlanes, eqworldlump_t** pSurfaceLumps)
{
	// load sectors
	m_numRooms = pRooms->data_size / sizeof(eqroom_t);
	m_pRooms = new CEqRoom[m_numRooms];

	eqroom_t* roomLumpData = (eqroom_t*)(((ubyte*)pRooms) + sizeof(eqworldlump_t));
	eqroomvolume_t* volumeLumpData = (eqroomvolume_t*)(((ubyte*)pVolumes) + sizeof(eqworldlump_t));

	int nVolume = 0;
	
	for(int i = 0; i < m_numRooms; i++)
	{
		eqroom_t* pRoom = &roomLumpData[i];

		m_pRooms[i].Init(pRoom);
		m_pRooms[i].m_numRoomVolumes = pRoom->numVolumes;
		m_pRooms[i].m_pRoomVolumes = new CEqRoomVolume[pRoom->numVolumes];

		for(int j = 0; j < roomLumpData[i].numVolumes; j++)
		{
			eqroomvolume_t* pVolume = &volumeLumpData[pRoom->firstVolume + j];

			m_pRooms[i].m_pRoomVolumes[j].LoadRenderData( i, pVolume, pSurfaceLumps[nVolume], pPlanes);
			m_pRooms[i].m_pRoomVolumes[j].volume_info = volumeLumpData[pRoom->firstVolume + j];

			nVolume++;
		}
	}

	//Msg( "--- loaded %d rooms for '%s'\n", m_numRooms, m_levelpath.GetData() );
}

void CEqLevel::LoadPortals(eqworldlump_t* pPortals, eqworldlump_t* pVerts, eqworldlump_t* pPlanes)
{
	eqareaportal_t* portalData = (eqareaportal_t*)(((ubyte*)pPortals) + sizeof(eqworldlump_t));
	m_numPortals = pPortals->data_size / sizeof(eqareaportal_t);

	m_pPortals = new CEqAreaPortal[m_numPortals];

	for(int i = 0; i < m_numPortals; i++)
		m_pPortals[i].Init(&portalData[i], pVerts, pPlanes);
	
	// set portal sides
	for(int i = 0; i < m_numRooms; i++)
	{
		for(int j = 0; j < m_pRooms[i].room_info.numPortals; j++)
		{
			int portal_id = m_pRooms[i].room_info.portals[j];

			ubyte nSide = 0;

			int room_id_at_side = m_pPortals[portal_id].portal_info.rooms[nSide];

			if( room_id_at_side != i )
				nSide = 1;

			m_pRooms[i].m_pPortalSides[j] = nSide;
		}
	}
	
	Msg( "--- loaded %d portals for '%s'\n", m_numPortals, m_levelpath.GetData() );
}

void CEqLevel::LoadLightmaps()
{
	// alloc lightmaps
	if(m_numLightmaps)
	{
		m_pLightmaps = new ITexture*[m_numLightmaps];
		m_pDirmaps = new ITexture*[m_numLightmaps];
	}

	for(int i = 0; i < m_numLightmaps; i++)
	{
		EqString filename(varargs("worlds/%s/lm#%d", m_levelpath.GetData(), i));
		m_pLightmaps[i] = g_pShaderAPI->LoadTexture(filename.GetData(),TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,TEXFLAG_REALFILEPATH | TEXFLAG_NOQUALITYLOD);

		//EqString dirfilename(varargs("worlds/%s/dm#%d", m_levelpath.GetData(), i));
		m_pDirmaps[i] = NULL;//g_pShaderAPI->LoadTexture(dirfilename.GetData(),TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,TEXFLAG_REALFILEPATH | TEXFLAG_NOQUALITYLOD);
	}
}

void CEqLevel::InitMaterials(eqworldlump_t* pMaterials)
{
	// load materials
	m_numMaterials = pMaterials->data_size / sizeof(eqlevelmaterial_t);
	eqlevelmaterial_t* pMaterialStrings = (eqlevelmaterial_t*)(((ubyte*)pMaterials) + sizeof(eqworldlump_t));

	DevMsg(DEVMSG_CORE, "Loading materials...\n");

	m_pMaterials = new IMaterial*[m_numMaterials];
	//m_nMaterialRenderCounter = DNewArray(int, m_numMaterials);

	for(int i = 0; i < m_numMaterials; i++)
	{
		m_pMaterials[i] = materials->GetMaterial(pMaterialStrings[i].material_path);

		if(m_pMaterials[i])
		{
			m_pMaterials[i]->Ref_Grab();

			// set to one to disallow unloading by first frame
			//m_nMaterialRenderCounter[i] = 1;
#ifdef EQLC
			m_pMaterials[i]->GetShader()->InitParams();
#endif
		}
	}
}

// load room materials
void CEqLevel::LoadRoomMaterials( int targetroom )
{
	if(targetroom != -1)
	{
		for(int i = 0; i < m_pRooms[targetroom].m_numRoomVolumes; i++)
		{
			for(int j = 0; j < m_pRooms[targetroom].m_pRoomVolumes[i].m_numDrawSurfaceGroups; j++)
			{
				eqlevelsurf_t* pSurf = &m_pRooms[targetroom].m_pRoomVolumes[i].m_pDrawSurfaceGroups[j];

				if(pSurf->material_id != -1)
					materials->PutMaterialToLoadingQueue( m_pMaterials[pSurf->material_id] );
			}
		}
	}
}

// unloads materials in room
void CEqLevel::UnloadRoomMaterials(int targetroom)
{
	/*
	if(targetroom != -1)
	{
		for(int i = 0; i < m_pRooms[targetroom].m_numRoomVolumes; i++)
		{
			for(int j = 0; j < m_pRooms[targetroom].m_pRoomVolumes[i].m_numDrawSurfaceGroups; j++)
			{
				eqlevelsurf_t* pSurf = &m_pRooms[targetroom].m_pRoomVolumes[i].m_pDrawSurfaceGroups[j];

				if(m_nMaterialRenderCounter[pSurf->material_id] == 0)
					m_pMaterials[pSurf->material_id]->GetShader()->Unload();
			}
		}
	}

	// reset render counters here
	memset(m_nMaterialRenderCounter,0,sizeof(int) * m_numMaterials);
	*/
}

void CEqLevel::InitRenderBuffers(eqworldlump_t* pVerts, eqworldlump_t* pIndices)
{
	if(!m_pVertexFormat_Lit)
	{
		VertexFormatDesc_t pFormat[] = {
			{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0 + lightmap coord

			/*
			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_UBYTE }, // t.xyz + color.r
			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_UBYTE }, // b.xyz + color.g
			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_UBYTE }, // n.xyz + color.b
			*/
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Tangent (TC1)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Binormal (TC2)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // vertex painting data (TC4)
		};

		// create vertex format
		m_pVertexFormat_Lit = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
	}

	// decals has no lightmap coords
	if(!m_pDecalVertexFormat)
	{
		VertexFormatDesc_t pFormat[] = {
			{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
			{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Tangent (TC1)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Binormal (TC2)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // vertex painting data (TC4)
		};

		// create vertex format
		m_pDecalVertexFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
	}

	// create vertex buffer
	int numDrawVerts = pVerts->data_size / sizeof(eqlevelvertexlm_t);
	int numDrawIndices = pIndices->data_size / sizeof(int);

	if(numDrawVerts == 0 || numDrawIndices == 0)
		return;

	void* pVertexData = ((ubyte*)pVerts) + sizeof(eqworldlump_t);

	m_pVertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, numDrawVerts, sizeof(eqlevelvertexlm_t), pVertexData);

	// create index buffer
	void* pIndexData = ((ubyte*)pIndices) + sizeof(eqworldlump_t);

	m_pIndexBuffer = g_pShaderAPI->CreateIndexBuffer(numDrawIndices, sizeof(int), BUFFER_DYNAMIC, pIndexData);

	// copy all geometry to non-hardware buffer (needs for decals)
	m_pVertices = (eqlevelvertexlm_t*)PPAlloc(sizeof(eqlevelvertexlm_t) * numDrawVerts);
	m_pIndices = (int*)PPAlloc(numDrawIndices * sizeof(int));

	memcpy(m_pVertices, pVertexData, numDrawVerts * sizeof(eqlevelvertexlm_t));
	memcpy(m_pIndices, pIndexData, numDrawIndices * sizeof(int));
}

//
// Creates static object from level data.
//
static IPhysicsObject* CreateWorldPhysicsObject(eqphysicsvertex_t* vertices, int* indices, int numIndices, int numVertices, char* surfaceprops, IMaterial* pMaterial = NULL)
{
	physmodelcreateinfo_t info;
	SetDefaultPhysModelInfoParams(&info);

	dkCollideData_t collData;

	collData.pMaterial = pMaterial; // null groups means default material
	collData.surfaceprops = surfaceprops;

	collData.indices = (uint*)indices;
	collData.numIndices = numIndices;
	collData.vertices = vertices;
	collData.vertexPosOffset = offsetOf(eqphysicsvertex_t, position);
	collData.vertexSize = sizeof(eqphysicsvertex_t);
	collData.numVertices = numVertices;

	info.isStatic = true;
	info.flipXAxis = false;
	info.mass = 0.0f;
	info.data = &collData;

	return physics->CreateStaticObject(&info, COLLISION_GROUP_WORLD);
}

int CEqLevel::GetPhysicsSurfaces(eqphysicsvertex_t** ppVerts, int** ppIndices, eqlevelphyssurf_t** ppSurfs, int &numIndices)
{
	*ppVerts = m_pPhysVertexData;
	*ppIndices = m_pPhysIndexData;

	numIndices = m_numPhysIndices;

	*ppSurfs = m_pPhysSurfaces;

	return m_numPhysSurfaces;
}

void CEqLevel::LoadPhysicsData(eqworldlump_t* pSurf, eqworldlump_t* pVerts, eqworldlump_t* pIndices)
{
	DevMsg(DEVMSG_CORE, "Loading physics...\n");

	int numPhysVerts = pVerts->data_size / sizeof(eqphysicsvertex_t);
	eqphysicsvertex_t* pPhysVertexData = (eqphysicsvertex_t*)(((ubyte*)pVerts) + sizeof(eqworldlump_t));

	m_numPhysIndices =  pIndices->data_size / sizeof(int);
	int* pPhysIndexData = (int*)(((ubyte*)pIndices) + sizeof(eqworldlump_t));

	m_numPhysSurfaces = pSurf->data_size / sizeof(eqlevelphyssurf_t);
	eqlevelphyssurf_t* pPhysSurfaces = (eqlevelphyssurf_t*)(((ubyte*)pSurf) + sizeof(eqworldlump_t));

	// alloc and copy physics data except the surfaces
	m_pPhysVertexData = (eqphysicsvertex_t*)PPAlloc(pVerts->data_size);
	memcpy(m_pPhysVertexData, pPhysVertexData, pVerts->data_size);

	m_pPhysIndexData = (int*)PPAlloc(pIndices->data_size);
	memcpy(m_pPhysIndexData, pPhysIndexData, pIndices->data_size);

	m_pPhysSurfaces = (eqlevelphyssurf_t*)PPAlloc(pSurf->data_size);
	memcpy(m_pPhysSurfaces, pPhysSurfaces, pSurf->data_size);

	for(int i = 0; i < m_numPhysSurfaces; i++)
	{
		IPhysicsObject* pObj = CreateWorldPhysicsObject(m_pPhysVertexData, 
														m_pPhysIndexData + pPhysSurfaces[i].firstindex, 
														pPhysSurfaces[i].numindices, numPhysVerts, 
														pPhysSurfaces[i].surfaceprops,
														m_pMaterials[pPhysSurfaces[i].material_id]);

		m_pPhysicsObjects.append(pObj);
	}
}

void CEqLevel::LoadOcclusion(eqworldlump_t* pSurf, eqworldlump_t* pVerts, eqworldlump_t* pIndices)
{
	int numOccVerts = pVerts->data_size / sizeof(Vector3D);
	Vector3D* pOccVertexData = (Vector3D*)(((ubyte*)pVerts) + sizeof(eqworldlump_t));

	int numOccIndices =  pIndices->data_size / sizeof(int);
	int* pOccIndexData = (int*)(((ubyte*)pIndices) + sizeof(eqworldlump_t));

	int numOccSurfaces = pSurf->data_size / sizeof(eqocclusionsurf_t);
	eqocclusionsurf_t* pOccSurfaces = (eqocclusionsurf_t*)(((ubyte*)pSurf) + sizeof(eqworldlump_t));

	m_vOcclusionVerts	= new Vector3D[numOccVerts];
	m_nOcclusionIndices = new int[numOccIndices];
	m_numOcclusionSurfs = numOccSurfaces;
	m_pOcclusionSurfaces = new eqocclusionsurf_t[m_numOcclusionSurfs];

	memcpy(m_pOcclusionSurfaces, pOccSurfaces, numOccSurfaces * sizeof(eqocclusionsurf_t));
	memcpy(m_vOcclusionVerts, pOccVertexData, numOccVerts * sizeof(Vector3D));
	memcpy(m_nOcclusionIndices, pOccIndexData, numOccIndices * sizeof(int));

	m_bOcclusionAvailable = true;
}

bool CEqLevel::IsLoaded()
{
	return m_bIsLoaded;
}

void CEqLevel::UnloadLevel()
{
	if(!m_bIsLoaded)
		MsgWarning("World loading aborted\n");
	else
		Msg("Unloading world\n");

	m_bIsLoaded = false;

	for(int i = 0; i < m_pPhysicsObjects.numElem(); i++)
		physics->DestroyPhysicsObject(m_pPhysicsObjects[i]);

	m_pPhysicsObjects.clear();

	if(m_pMaterials)
	{
		for(int i = 0; i < m_numMaterials; i++)
			materials->FreeMaterial(m_pMaterials[i]);

		delete [] m_pMaterials;
	}

	m_pMaterials = NULL;
	m_numMaterials = 0;

	if(m_pLightmaps)
	{
		for(int i = 0; i < m_numLightmaps; i++)
		{
			g_pShaderAPI->FreeTexture(m_pLightmaps[i]);
			g_pShaderAPI->FreeTexture(m_pDirmaps[i]);
		}

		delete [] m_pLightmaps;
		delete [] m_pDirmaps;
	}

	m_pLightmaps = NULL;
	m_pDirmaps = NULL;

	if(m_pLevelLights)
		PPFree(m_pLevelLights);
	m_pLevelLights = NULL;
	m_numLevelLights = 0;

	if(m_pPhysVertexData)
		PPFree(m_pPhysVertexData);
	m_pPhysVertexData = NULL;

	if(m_pPhysIndexData)
		PPFree(m_pPhysIndexData);
	m_pPhysIndexData = NULL;

	if(m_pPhysSurfaces)
		PPFree(m_pPhysSurfaces);
	m_pPhysSurfaces = NULL;

	m_numLightmaps = 0;

	if(m_pRooms)
	{
		for(int i = 0; i < m_numRooms; i++)
			m_pRooms[i].FreeData();

		m_numRooms = 0;

		delete [] m_pRooms;
	}

	if(m_pPortals)
	{
		for(int i = 0; i < m_numPortals; i++)
			m_pPortals[i].FreeData();

		delete [] m_pPortals;

		m_pPortals = NULL;
	}

	m_numPortals = 0;

	if(m_pWaterInfos)
	{
		for(int i = 0; i < m_nWaterInfoCount; i++)
			m_pWaterInfos[i].FreeData();

		m_nWaterInfoCount = 0;

		delete [] m_pWaterInfos;
	}

	m_pWaterInfos = NULL;

	for(int i = 0; i < m_StaticTransparents.numElem(); i++)
		delete m_StaticTransparents[i];

	m_StaticTransparents.clear();

	for(int i = 0; i < m_StaticDecals.numElem(); i++)
		delete m_StaticDecals[i];

	m_StaticDecals.clear();

	if(m_pDecalVertexFormat)
		g_pShaderAPI->DestroyVertexFormat(m_pDecalVertexFormat);

	m_pDecalVertexFormat = NULL;

	if(m_pVertexFormat_Lit)
		g_pShaderAPI->DestroyVertexFormat(m_pVertexFormat_Lit);

	m_pVertexFormat_Lit = NULL;

	if(m_pVertexBuffer)
		g_pShaderAPI->DestroyVertexBuffer(m_pVertexBuffer);

	if(m_pIndexBuffer)
		g_pShaderAPI->DestroyIndexBuffer(m_pIndexBuffer);

	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	if(m_bOcclusionAvailable)
	{
		delete [] m_pOcclusionSurfaces;
		m_pOcclusionSurfaces = NULL;
		m_numOcclusionSurfs = 0;

		delete [] m_vOcclusionVerts;
		m_vOcclusionVerts = NULL;

		delete [] m_nOcclusionIndices;
		m_nOcclusionIndices = 0;

		m_bOcclusionAvailable = false;
	}

	m_pRooms = NULL;
	m_numRooms = 0;

	m_EntityKeyValues.Reset();

	if(m_pIndices)
		PPFree(m_pIndices);

	if(m_pVertices)
		PPFree(m_pVertices);

	m_pVertices = NULL;
	m_pIndices = NULL;
}

struct decal_geom_data_t
{
	PPMEM_MANAGED_OBJECT();

	DkList<eqlevelvertex_t> verts;
	DkList<int>				indices;
};

void CopyGeom(decal_geom_data_t* pSrc, decal_geom_data_t* pDst)
{
	pDst->indices.setNum(0, false);
	pDst->verts.setNum(0, false);

	pDst->verts.resize(pSrc->verts.numElem());
	pDst->indices.resize(pSrc->indices.numElem());

	pDst->verts.append(pSrc->verts);
	pDst->indices.append(pSrc->indices);
}

int ClipVertsAgainstPlane( Plane &plane, int vertexCount, eqlevelvertex_t** pVerts )
{
	bool* negative = (bool*)stackalloc( vertexCount+2 );

	float t;
	Vector3D v, v1, v2;

	eqlevelvertex_t* vertex = *pVerts;

	// classify vertices
	int negativeCount = 0;
	for (int i = 0; i < vertexCount; i++)
	{
		negative[i] = !(plane.ClassifyPoint(vertex[i].position) == CP_FRONT || plane.ClassifyPoint(vertex[i].position) == CP_ONPLANE);
		negativeCount += negative[i];
	}
	
	if(negativeCount == vertexCount)
	{
		PPFree(vertex);
		return 0;			// completely culled, we have no polygon
	}

	if(negativeCount == 0)
	{
		return -1;			// not clipped
	}

	eqlevelvertex_t* newVertex = (eqlevelvertex_t*)alloca(sizeof(eqlevelvertex_t)*(vertexCount+2));

	int count = 0;

	for (int i = 0; i < vertexCount; i++)
	{
		// prev is the index of the previous vertex
		int	prev = (i != 0) ? i - 1 : vertexCount - 1;
		
		if (negative[i])
		{
			if (!negative[prev])
			{
				Vector3D v1 = vertex[prev].position;
				Vector3D v2 = vertex[i].position;

				// Current vertex is on negative side of plane,
				// but previous vertex is on positive side.
				Vector3D edge = v1 - v2;
				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = InterpolateLevelVertex(vertex[prev],vertex[i],t);
				count++;
			}
		}
		else
		{
			if (negative[prev])
			{
				Vector3D v1 = vertex[i].position;
				Vector3D v2 = vertex[prev].position;

				// Current vertex is on positive side of plane,
				// but previous vertex is on negative side.
				Vector3D edge = v1 - v2;

				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = InterpolateLevelVertex(vertex[i],vertex[prev],t);
				
				count++;
			}
			
			// include current vertex
			newVertex[count] = vertex[i];
			count++;
		}
	}

	PPFree( vertex );
	*pVerts = PPAllocStructArray(eqlevelvertex_t, count);

	memcpy(*pVerts, newVertex, sizeof(eqlevelvertex_t)*count);
	
	// return new clipped vertex count
	return count;
}

void SplitGeomByPlane(decal_geom_data_t* surf, Plane &plane, decal_geom_data_t* front, decal_geom_data_t* back)
{
	S_NEWA(front_vert_remap, int, surf->verts.numElem());
	S_NEWA(back_vert_remap, int, surf->verts.numElem());

	int nNegative = 0;

	for(int i = 0; i < surf->verts.numElem(); i++)
	{
		front_vert_remap[i] = -1;
		back_vert_remap[i] = -1;

		if(plane.ClassifyPointEpsilon( surf->verts[i].position, 0.0f) == CP_BACK)
			nNegative++;
	}

	if(nNegative == surf->verts.numElem())
	{
		if(back)
			CopyGeom(surf, back);

		return;
	}

	if(nNegative == 0)
	{
		if(front)
			CopyGeom(surf, front);

		return;
	}

	DkList<eqlevelvertex_t>	front_vertices;
	DkList<int>				front_indices;

	DkList<eqlevelvertex_t>	back_vertices;
	DkList<int>				back_indices;

	front_vertices.resize( surf->verts.numElem() + 16 );
	front_indices.resize( surf->indices.numElem() + 16 );

	back_vertices.resize(surf->verts.numElem() + 16 );
	back_indices.resize( surf->indices.numElem() + 16 );

#define ADD_REMAPPED_VERTEX(idx, vertex, name)					\
{																\
	if(##name##_vert_remap[ idx ] != -1)						\
	{															\
		##name##_indices.append( ##name##_vert_remap[ idx ] );	\
	}															\
	else														\
	{															\
		int currVerts = ##name##_vertices.numElem();			\
		##name##_vert_remap[idx] = currVerts;					\
		##name##_indices.append( currVerts );					\
		##name##_vertices.append( vertex );						\
	}															\
}

#define ADD_VERTEX(vertex, name)								\
{																\
	int currVerts = ##name##_vertices.numElem();				\
	##name##_indices.append( currVerts );						\
	##name##_vertices.append( vertex );							\
}
	DkList<int> clippedtris_indices;
	clippedtris_indices.resize( surf->indices.numElem() );

	for(int i = 0; i < surf->indices.numElem(); i += 3)
	{
		eqlevelvertexlm_t v[3];
		float d[3];
		int id[3];

		for (int j = 0; j < 3; j++)
		{
			id[j] = surf->indices[i + j];
			v[j] = surf->verts[id[j]];
			d[j] = plane.Distance(v[j].position);
		}

		// fill non-clipped triangles with fastest remap method
		if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0)
		{
			if (front)
			{
				for (int j = 0; j < 3; j++)
				{
					ADD_REMAPPED_VERTEX(id[j], v[j], front)
				}
			}
		}
		else if (d[0] < 0 && d[1] < 0 && d[2] < 0)
		{
			if (back)
			{
				for (int j = 0; j < 3; j++)
				{
					ADD_REMAPPED_VERTEX(id[j], v[j], back)
				}
			}
		}

		else // clip vertices and add
		{
			// needs to cache clipped triangles
			//clippedtris_indices.append(id[0]);
			//clippedtris_indices.append(id[1]);
			//clippedtris_indices.append(id[1]);
			
			int front0 = 0;
			int prev = 2;
			for (int k = 0; k < 3; k++)
			{
				if (d[prev] < 0 && d[k] >= 0)
				{
					front0 = k;
					break;
				}
				prev = k;
			}

			int front1 = (front0 + 1) % 3;
			int front2 = (front0 + 2) % 3;

			if (d[front1] >= 0)
			{
				float x0 = d[front1] / dot(plane.normal, v[front1].position - v[front2].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				if(front)
				{
					
					ADD_REMAPPED_VERTEX(surf->indices[i+front0], surf->verts[surf->indices[i+front0]], front)
					ADD_REMAPPED_VERTEX(surf->indices[i+front1], surf->verts[surf->indices[i+front1]], front)

					// middle vertex is shared
					int idx = front_vertices.append(InterpolateLevelVertex(surf->verts[surf->indices[i+front1]], surf->verts[surf->indices[i+front2]], x0));
					front_indices.append(idx);
					
					ADD_REMAPPED_VERTEX(surf->indices[i+front0], surf->verts[surf->indices[i+front0]], front)
					front_indices.append(idx);
					ADD_VERTEX(InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front2]], x1), front)

				}

				if(back)
				{
					// non-interpolated verts added as well
					ADD_REMAPPED_VERTEX(surf->indices[i+front2], surf->verts[surf->indices[i+front2]], back)
					ADD_VERTEX( InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front2]], x1), back)
					ADD_VERTEX( InterpolateLevelVertex(surf->verts[surf->indices[i+front1]], surf->verts[surf->indices[i+front2]], x0), back)
				}
			} 
			else 
			{
				float x0 = d[front0] / dot(plane.normal, v[front0].position - v[front1].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				if(front)
				{
					// non-interpolated verts added as well
					ADD_REMAPPED_VERTEX(surf->indices[i+front0], surf->verts[surf->indices[i+front0]], back)
					ADD_VERTEX( InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front1]], x0), back)
					ADD_VERTEX( InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front2]], x1), back)
				}

				if(back)
				{
					
					ADD_REMAPPED_VERTEX(surf->indices[i+front1], surf->verts[surf->indices[i+front1]], back)
					ADD_REMAPPED_VERTEX(surf->indices[i+front2], surf->verts[surf->indices[i+front2]], back)

					// middle vertex is shared
					int idx = back_vertices.append(InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front2]], x1));
					back_indices.append(idx);

					ADD_REMAPPED_VERTEX(surf->indices[i+front1], surf->verts[surf->indices[i+front1]], back)
					back_indices.append(idx);
					ADD_VERTEX(InterpolateLevelVertex(surf->verts[surf->indices[i+front0]], surf->verts[surf->indices[i+front1]], x0), back)
					
				}
			}
			
		}
	}

	/*
	// process affected triangles
	DkList<mtriangle_t> mtris;
	DecomposeIndicesToTriangles( clippedtris_indices.ptr(), clippedtris_indices.numElem(), mtris );

	if(mtris.numElem() > 0)
	{
		mtriangle_t* pCurrent = &mtris[0];
		eqlevelvertex_t* pVerts = PPAllocStructArray(eqlevelvertex_t, 4); 

		// try to generate quads and clip them
		while( true )
		{
			int nVerts = 3;
			pVerts[0] = surf->verts[pCurrent->indices[0]];
			pVerts[1] = surf->verts[pCurrent->indices[1]];
			pVerts[2] = surf->verts[pCurrent->indices[2]];

			for(int j = 0; j < 3; j++)
			{
				if(pCurrent->edge_connections[j] != NULL)
				{
					pCurrent = pCurrent->edge_connections[j];
					int nOpposite = pCurrent->GetOppositeVertexIndex( j );

					int nVerts = 4;
					pVerts[3] = surf->verts[pCurrent->indices[nOpposite]];


				}
			}

			int nNewVerts = ClipVertsAgainstPlane( plane, nVerts, &pVerts);

		}

		PPFree( pVerts );
	}
	*/
	
	// remappers done now
	front_vert_remap.Free();
	back_vert_remap.Free();

	if(front_indices.numElem() > 0 && front)
	{
		front->indices.setNum(0, false);
		front->verts.setNum(0, false);

		front->verts.resize(front_vertices.numElem());
		front->indices.resize(front_indices.numElem());

		front->verts.append(front_vertices);
		front->indices.append(front_indices);
	}

	if(back_indices.numElem() > 0 && back)
	{
		back->indices.setNum(0, false);
		back->verts.setNum(0, false);

		back->verts.resize(back_vertices.numElem());
		back->indices.resize(back_indices.numElem());

		back->verts.append(back_vertices);
		back->indices.append(back_indices);
	}
}

decal_geom_data_t* ClipGeomToVolume(decal_geom_data_t* geomdata, Volume& decal_vol)
{
	decal_geom_data_t* pOutFace = geomdata;

	for (int i = 0; i < 6; i++)
	{
		//int i = clipn.GetInt();

		decal_geom_data_t* pTemp = new decal_geom_data_t;

		Plane pl = decal_vol.GetPlane(i);

		pl.offset *= -1;
		pl.normal *= -1;

		SplitGeomByPlane(pOutFace, pl, NULL, pTemp );

		// free old clipped ( but keep original)
		if(pOutFace != geomdata )
			delete pOutFace;

		// set it and do far
		pOutFace = pTemp;
	}

	// if we totally clipped all
	if(pOutFace->indices.numElem() < 3)
	{
		// free old clipped ( but keep original)
		if( pOutFace != geomdata )
			delete pOutFace;

		return NULL;
	}
	
	// done
	return pOutFace;
}

void CopyLevelGeomToDecal(eqlevelsurf_t* pSurfGrp, eqlevelvertexlm_t* pVerts, int* indices, decal_geom_data_t* pDst, const Volume& decal_volume, const decalmakeinfo_t& info)
{
	pDst->indices.resize(pDst->indices.numElem() + pSurfGrp->numindices);
	pDst->verts.resize(pDst->verts.numElem() + pSurfGrp->numvertices);

	int* vert_remap = new int[pSurfGrp->numvertices];

	for(int i = 0; i < pSurfGrp->numvertices; i++)
		vert_remap[i] = -1;

#define ADD_RMP_VERTEX(idx, vertex)					\
{													\
	if(vert_remap[ idx ] != -1)						\
	{												\
		pDst->indices.append( vert_remap[ idx ] );	\
	}												\
	else											\
	{												\
		int currVerts = pDst->verts.numElem();		\
		vert_remap[idx] = currVerts;				\
		pDst->indices.append( currVerts );			\
		pDst->verts.append( vertex );				\
	}												\
}

	for(int i = 0; i < pSurfGrp->numindices; i+=3)
	{
		eqlevelvertexlm_t v[3];
		int id[3];

		for(int j = 0; j < 3; j++)
		{
			id[j] = indices[pSurfGrp->firstindex + i + j];
			v[j] = pVerts[id[j]];
		}

		if( decal_volume.IsTriangleInside(v[0].position, v[1].position, v[2].position) )
		{
			if(info.flags & MAKEDECAL_FLAG_CLIPPING)
			{
				Vector3D normal;
				ComputeTriangleNormal(v[0].position, v[1].position, v[2].position, normal);

				if(info.flags & MAKEDECAL_FLAG_TEX_NORMAL)
				{
					if(dot(info.normal, normal) > 0)
						continue;
				}
				else // do radial
				{
					Vector3D triangle_center = (v[0].position + v[1].position + v[2].position) / 3.0f;

					if(dot(triangle_center-info.origin, normal) > 0)
						continue;
				}
			}

			for(int j = 0; j < 3; j++)
			{
				ADD_RMP_VERTEX(id[j] - pSurfGrp->firstvertex, v[j]);
			}
		}
	}

	delete [] vert_remap;
}

// the triangles of some plane
struct tris_plane_t
{
	Plane plane;
	DkList<mtriangle_t*>	assigned;
	DkList<medge_t*>		edges;
};

void OptimizeDecal(decal_geom_data_t* pDecal)
{
	return;

	/*

	// make triangle list first
	DkList<mtriangle_t> tris;
	DecomposeIndicesToTrianglesWithVerts((ubyte*)pDecal->verts.ptr(), sizeof(eqlevelvertex_t), offsetOf(eqlevelvertex_t, position) , pDecal->indices.ptr(), pDecal->indices.numElem(), tris);

	DkList<tris_plane_t*> triplanes;

	tris_plane_t* pCur = NULL;

	// join the triangles that are in same planes
	for(int i = 0; i < tris.numElem(); i++)
	{
		Plane pl(	pDecal->verts[tris[i].indices[0]].position,
					pDecal->verts[tris[i].indices[1]].position,
					pDecal->verts[tris[i].indices[2]].position);

		if(!pCur)
		{
			for(int j = 0; j < triplanes.numElem(); j++)
			{
				if( triplanes[j]->plane.CompareEpsilon(pl, 0.01, 0.025) )
				{
					pCur = triplanes[j];
					break;
				}
			}

			if(!pCur)
			{
				pCur = new tris_plane_t;
				triplanes.append(pCur);

				pCur->plane = pl;
			}
		}

		if( !pCur->plane.CompareEpsilon(pl, 0.01, 0.025) )
		{
			pCur = NULL;
			i--;
			continue;
		}

		pCur->assigned.append(&tris[i]);
	}

	// check for another useless optimization ;)
	if(tris.numElem() == triplanes.numElem())
	{
		for(int i = 0; i < triplanes.numElem(); i++)
			delete triplanes[i];

		return;
	}

	// now we've need to build convex points.
	// first, collect unique point list for each triplane
	for(int i = 0; i < triplanes.numElem(); i++)
	{
		for(int j = 0; j < triplanes[i]->assigned.numElem(); j++)
		{
			for(int k = 0; k < 3; k++)
			{
				if(!triplanes[i]->assigned[j]->edge_connections[k])
					triplanes[i]->edges.append( &triplanes[i]->assigned[j]->edges[k] );
			}
		}
	}

	/*
	// sort edges
	for(int i = 0; i < triplanes.numElem(); i++)
	{
		triplanes[i]->edges.sort( medge_sort );

		// try to remove double angles
		for(int j = 0; j < triplanes[i]->edges.numElem(); j++)
		{
			Vector3D vj[2] = {
				pDecal->verts[ triplanes[i]->edges[j]->idx[0] ].position, 
				pDecal->verts[ triplanes[i]->edges[j]->idx[1] ].position};

			for(int k = j+1; k < triplanes[i]->edges.numElem(); k++)
			{
				Vector3D vk[2] = {
					pDecal->verts[ triplanes[i]->edges[k]->idx[0] ].position, 
					pDecal->verts[ triplanes[i]->edges[k]->idx[1] ].position};

				float fEdgeDot = dot(vj[0]-vj[1], vk[0]-vk[1]);

				// if edge has identifical angles
				if( fEdgeDot > 0.99 )
				{
					int replace_n = -1;
					int replace_idx = -1;

					// get nearest vertex positions and replace edges
					if( compare_epsilon(vj[0], vk[0], 0.001f))
					{
						replace_n = 0;
						replace_idx = 1;
					}
					else if( compare_epsilon(vj[0], vk[1], 0.001f) )
					{
						replace_n = 0;
						replace_idx = 1;
					}
					else if( compare_epsilon(vj[1], vk[0], 0.001f))
					{
						replace_n = 1;
					}
					else if( compare_epsilon(vj[1], vk[1], 0.001f) )
					{
						replace_n = 1;
					}

					// else do nothing
					if(replace_n != -1)
					{
						medge_t edge;
						//edge.idx[0] = 
					}
				}
			}
		}
	}*/
	

/*
	for(int i = 0; i < triplanes.numElem(); i++)
		delete triplanes[i];
		*/
}

void MakeDecalTexCoord(decal_geom_data_t* pGeomData, const decalmakeinfo_t &info)
{
	Vector3D scaledSize = info.size*2.0f;

	if(info.flags & MAKEDECAL_FLAG_TEX_NORMAL)
	{
		int texSizeW = 1;
		int texSizeH = 1;

		ITexture* pTex = info.material->GetBaseTexture();

		if(pTex)
		{
			texSizeW = pTex->GetWidth();
			texSizeH = pTex->GetHeight();
		}

		Vector3D uaxis;
		Vector3D vaxis;

		Vector3D axisAngles = VectorAngles(info.normal);
		AngleVectors(axisAngles, NULL, &uaxis, &vaxis);

		vaxis *= -1;

		//VectorVectors(m_surftex.Plane.normal, m_surftex.UAxis.normal, m_surftex.VAxis.normal);

		Matrix3x3 texMatrix(uaxis, vaxis, info.normal);

		Matrix3x3 rotationMat = rotateZXY3( 0.0f, 0.0f, DEG2RAD(info.texRotation));
		texMatrix = rotationMat*texMatrix;

		uaxis = texMatrix.rows[0];
		vaxis = texMatrix.rows[1];

		//debugoverlay->Line3D(info.origin, info.origin + info.normal*10, ColorRGBA(0,0,1,1), ColorRGBA(0,1,0,1), 100500);

		for(int i = 0; i < pGeomData->verts.numElem(); i++)
		{
			float one_over_w = 1.0f / fabs(dot(scaledSize,uaxis*uaxis) * info.texScale.x);
			float one_over_h = 1.0f / fabs(dot(scaledSize,vaxis*vaxis) * info.texScale.y);

			float U, V;
		
			U = dot(uaxis, info.origin-pGeomData->verts[i].position) * one_over_w+0.5f;
			V = dot(vaxis, info.origin-pGeomData->verts[i].position) * one_over_h-0.5f;

			pGeomData->verts[i].texcoord.x = U;
			pGeomData->verts[i].texcoord.y = -V;
		}
	}
	else
	{
		// generate new texture coordinates
		for(int i = 0; i < pGeomData->verts.numElem(); i++)
		{
			pGeomData->verts[i].tangent = normalize(pGeomData->verts[i].tangent);
			pGeomData->verts[i].binormal = normalize(pGeomData->verts[i].binormal);
			pGeomData->verts[i].normal = normalize(pGeomData->verts[i].normal);

			float one_over_w = 1.0f / fabs(dot(scaledSize,pGeomData->verts[i].tangent));
			float one_over_h = 1.0f / fabs(dot(scaledSize,pGeomData->verts[i].binormal));

			pGeomData->verts[i].texcoord.x = abs(dot(info.origin-pGeomData->verts[i].position,pGeomData->verts[i].tangent*sign(pGeomData->verts[i].tangent))*one_over_w+0.5f);
			pGeomData->verts[i].texcoord.y = abs(dot(info.origin-pGeomData->verts[i].position,pGeomData->verts[i].binormal*sign(pGeomData->verts[i].binormal))*one_over_h+0.5f);
		}
	}

	// it needs TBN refreshing
	for(int i = 0; i < pGeomData->indices.numElem(); i+=3)
	{
		int idxs[3] = {pGeomData->indices[i], pGeomData->indices[i+1], pGeomData->indices[i+2]};

		Vector3D t,b,n;

		ComputeTriangleTBN(	pGeomData->verts[idxs[0]].position, 
							pGeomData->verts[idxs[1]].position,
							pGeomData->verts[idxs[2]].position,
							pGeomData->verts[idxs[0]].texcoord,
							pGeomData->verts[idxs[1]].texcoord,
							pGeomData->verts[idxs[2]].texcoord,
							n,
							t,
							b);

		pGeomData->verts[idxs[0]].tangent = t;
		pGeomData->verts[idxs[0]].binormal = b;
		pGeomData->verts[idxs[1]].tangent = t;
		pGeomData->verts[idxs[1]].binormal = b;
		pGeomData->verts[idxs[2]].tangent = t;
		pGeomData->verts[idxs[2]].binormal = b;
	}
}

// makes dynamic temporary decal
tempdecal_t* CEqLevel::MakeTempDecal( const decalmakeinfo_t& info )
{
	if(r_notempdecals.GetBool())
		return NULL;

	BoundingBox decal_bbox(info.origin-info.size, info.origin+info.size);
	decal_bbox.Fix();

	Volume decal_vol;
	decal_vol.LoadBoundingBox(decal_bbox.minPoint, decal_bbox.maxPoint);

	int room_ids[32];
	int nRooms = GetRoomsForSphere(info.origin, length(info.size), room_ids);

	if(nRooms == 0)
	{
		Msg("DECAL Outside the world!\n");
		return NULL;
	}

	int nSurf = -1;
	int nVolume = -1;

	if(nRooms > 1)
	{
		nSurf = -2;
		nVolume = -2;
	}

	// first is collecting the all intersecting geometry
	decal_geom_data_t* pInitialGeometry = new decal_geom_data_t;
	for(int i = 0; i < nRooms; i++)
	{
		CEqRoom* pRoom = &m_pRooms[room_ids[i]];

		for(int j = 0; j < pRoom->room_info.numVolumes; j++)
		{
			Vector3D min = pRoom->m_pRoomVolumes[j].m_bounding.minPoint;
			Vector3D max = pRoom->m_pRoomVolumes[j].m_bounding.minPoint;
			
			/*
			// check bounds
			if(!decal_vol.IsBoxInside( min.x, max.x, min.y, max.y, min.z, max.z ))
				continue;
				*/

			for(int k = 0; k < pRoom->m_pRoomVolumes[j].m_numDrawSurfaceGroups; k++)
			{
				eqlevelsurf_t* pSurf = &pRoom->m_pRoomVolumes[j].m_pDrawSurfaceGroups[k];

				IMatVar* pMatVarNoDecals = m_pMaterials[pSurf->material_id]->GetMaterialVar("nodecals", "0");

				if(pMatVarNoDecals->GetInt() == 1)
					continue;

				BoundingBox surf_bbox(pSurf->mins, pSurf->maxs);

				if(!decal_vol.IsBoxInside(	surf_bbox.minPoint.x, surf_bbox.maxPoint.x, 
										surf_bbox.minPoint.y, surf_bbox.maxPoint.y, 
										surf_bbox.minPoint.z, surf_bbox.maxPoint.z ))
										continue;
								
										

				if(nSurf != -1)
				{
					nSurf = -2;
					nVolume = -2;
				}
				else
				{
					nSurf = k;
					nVolume = j;
				}

				// add geometry to the initial geometry
				CopyLevelGeomToDecal(pSurf, m_pVertices, m_pIndices, pInitialGeometry, decal_vol, info);
			}
		}
	}

	// we need to clip our geometry by all planes
	decal_geom_data_t* pDecalGeom = ClipGeomToVolume( pInitialGeometry, decal_vol );

	delete pInitialGeometry;

	tempdecal_t* pDecal = NULL;

	if(pDecalGeom)
	{
		//OptimizeDecal( pDecalGeom );
		
		MakeDecalTexCoord( pDecalGeom, info );
		
		// allocate
		pDecal = new tempdecal_t;
		pDecal->flags = 0;
		pDecal->numIndices = 0;
		pDecal->numVerts = 0;
		pDecal->material = info.material;

		pDecal->numVerts = pDecalGeom->verts.numElem();
		pDecal->numIndices = pDecalGeom->indices.numElem();

		pDecal->verts = PPAllocStructArray(eqlevelvertex_t, pDecal->numVerts);
		pDecal->indices = PPAllocStructArray(uint16, pDecal->numIndices);

		// copy geometry
		memcpy(pDecal->verts, pDecalGeom->verts.ptr(), sizeof(eqlevelvertex_t)*pDecal->numVerts);

		for(int i = 0; i < pDecal->numIndices; i++)
			pDecal->indices[i] = pDecalGeom->indices[i];
		
		if(pDecalGeom != pInitialGeometry)
			delete pDecalGeom;
	}

	return pDecal;
}

// makes static decal (and updates buffers, slow on creation)
staticdecal_t* CEqLevel::MakeStaticDecal( const decalmakeinfo_t &info )
{
	BoundingBox decal_bbox(info.origin-info.size, info.origin+info.size);
	decal_bbox.Fix();

	Volume decal_vol;
	decal_vol.LoadBoundingBox(decal_bbox.minPoint, decal_bbox.maxPoint);

	int room_ids[32];
	int nRooms = GetRoomsForSphere(info.origin, length(info.size), room_ids);

	if(nRooms == 0)
		return NULL;

	int nSurf = -1;
	int nVolume = -1;

	if(nRooms > 1)
	{
		nSurf = -2;
		nVolume = -2;
	}


	// first is collecting the all intersecting geometry
	decal_geom_data_t* pInitialGeometry = new decal_geom_data_t;
	for(int i = 0; i < nRooms; i++)
	{
		CEqRoom* pRoom = &m_pRooms[room_ids[i]];

		for(int j = 0; j < pRoom->room_info.numVolumes; j++)
		{
			Vector3D min = pRoom->m_pRoomVolumes[j].m_bounding.minPoint;
			Vector3D max = pRoom->m_pRoomVolumes[j].m_bounding.minPoint;
			
			// check bounds
			//if(!decal_vol.IsBoxInside( min.x, max.x, min.y, max.y, min.z, max.z ))
			//	continue;

			for(int k = 0; k < pRoom->m_pRoomVolumes[j].m_numDrawSurfaceGroups; k++)
			{
				eqlevelsurf_t* pSurf = &pRoom->m_pRoomVolumes[j].m_pDrawSurfaceGroups[k];

				IMatVar* pMatVarNoDecals = m_pMaterials[pSurf->material_id]->GetMaterialVar("nodecals", "0");

				if(pMatVarNoDecals->GetInt() == 1)
					continue;

				BoundingBox surf_bbox(pSurf->mins, pSurf->maxs);

				
				if(!decal_vol.IsBoxInside(	surf_bbox.minPoint.x, surf_bbox.maxPoint.x, 
											surf_bbox.minPoint.y, surf_bbox.maxPoint.y, 
											surf_bbox.minPoint.z, surf_bbox.maxPoint.z ))
										continue;
								
										

				if(nSurf != -1)
				{
					nSurf = -2;
					nVolume = -2;
				}
				else
				{
					nSurf = k;
					nVolume = j;
				}

				// add geometry to the initial geometry
				CopyLevelGeomToDecal(pSurf, m_pVertices, m_pIndices, pInitialGeometry, decal_vol, info);
			}
		}
	}

	// we need to clip our geometry by all planes
	decal_geom_data_t* pDecalGeom = ClipGeomToVolume( pInitialGeometry, decal_vol );

	delete pInitialGeometry;

	staticdecal_t* pDecal = NULL;

	if(pDecalGeom)
	{
		//OptimizeDecal( pDecalGeom );

		MakeDecalTexCoord( pDecalGeom, info );

		CWorldDecal* pRenderableDecal = new CWorldDecal(); 
		
		// allocate
		pDecal = &pRenderableDecal->decal;
		pDecal->flags = 0;
		pDecal->numIndices = 0;
		pDecal->numVerts = 0;
		pDecal->material = info.material;
		pDecal->box = decal_bbox;

		// static decals are unlimited
		pDecal->numVerts = pDecalGeom->verts.numElem();
		pDecal->numIndices = pDecalGeom->indices.numElem();

		pDecal->pVerts = PPAllocStructArray(eqlevelvertex_t, pDecal->numVerts);
		pDecal->pIndices = PPAllocStructArray(int, pDecal->numIndices);

		// copy geometry
		memcpy(pDecal->pVerts, pDecalGeom->verts.ptr(), sizeof(eqlevelvertex_t)*pDecal->numVerts);
		memcpy(pDecal->pIndices, pDecalGeom->indices.ptr(), sizeof(int)*pDecal->numIndices);

		pRenderableDecal->Init();

		m_StaticDecals.append(pRenderableDecal);

		if(pDecalGeom != pInitialGeometry)
			delete pDecalGeom;
	}

	return pDecal;
}

void CEqLevel::FreeStaticDecal( staticdecal_t* pDecal )
{
	for(int i = 0; i < m_StaticDecals.numElem(); i++)
	{
		if(&m_StaticDecals[i]->decal == pDecal)
		{
			delete m_StaticDecals[i];

			m_StaticDecals.removeIndex(i);
			break;
		}
	}
}

void CEqLevel::UpdateStaticDecals()
{
	// make render buffer for them!
	if(m_pDecalVertexBuffer_Static)
		g_pShaderAPI->DestroyVertexBuffer(m_pDecalVertexBuffer_Static);

	if(m_pDecalIndexBuffer_Static)
		g_pShaderAPI->DestroyIndexBuffer(m_pDecalIndexBuffer_Static);

	m_pDecalVertexBuffer_Static = NULL;
	m_pDecalIndexBuffer_Static = NULL;

	if(m_StaticDecals.numElem() == NULL)
		return;

	DkList<eqlevelvertex_t> verts;
	DkList<int>				indices;

	// make index and vertex buffers
	for(int i = 0; i < m_StaticDecals.numElem(); i++)
	{
		int firstIndex = indices.numElem();
		int firstVertex = verts.numElem();

		indices.resize(indices.numElem() + m_StaticDecals[i]->decal.numIndices);

		for(int j = 0; j < m_StaticDecals[i]->decal.numIndices; j++)
			indices.append( firstVertex + m_StaticDecals[i]->decal.pIndices[j] );

		verts.resize(verts.numElem() + m_StaticDecals[i]->decal.numVerts);

		for(int j = 0; j < m_StaticDecals[i]->decal.numVerts; j++)
			verts.append( m_StaticDecals[i]->decal.pVerts[j] );

		m_StaticDecals[i]->decal.firstVertex = firstVertex;
		m_StaticDecals[i]->decal.firstIndex = firstIndex;
	}

	m_pDecalVertexBuffer_Static = g_pShaderAPI->CreateVertexBuffer( BUFFER_STATIC, verts.numElem(), sizeof(eqlevelvertex_t), verts.ptr());
	m_pDecalIndexBuffer_Static = g_pShaderAPI->CreateIndexBuffer( indices.numElem(), sizeof(int), BUFFER_STATIC, indices.ptr());
}

// returns 1 or 2 rooms for point (if inside a split of portal). Returns count of rooms (-1 is outside)
int	CEqLevel::GetRoomsForPoint(const Vector3D &point, int room_ids[2])
{
	return GetRoomsForSphere(point, 0.0f, room_ids);
}

// bbox checking for rooms (can return more than 2 rooms)
int CEqLevel::GetRoomsForSphere(const Vector3D &point, float radius, int* room_ids)
{
	// check portal insideness
	for(int i = 0; i < g_pLevel->m_numPortals; i++)
	{
		CEqAreaPortal* pPortal = &g_pLevel->m_pPortals[i];

		if( pPortal->m_bounding.Contains( point ) )
		{
			// check for between the portal planes
			ClassifyPlane_e cp_side[2];

			cp_side[0] = pPortal->m_planes[0].ClassifyPointEpsilon(point, 0.05f+radius);
			cp_side[1] = pPortal->m_planes[1].ClassifyPointEpsilon(point, 0.05f+radius);
		
			if((cp_side[0] == CP_BACK && cp_side[1] == CP_BACK) || (cp_side[0] == CP_ONPLANE && cp_side[1] == CP_ONPLANE))
			{
				room_ids[0] = pPortal->portal_info.rooms[0];
				room_ids[1] = pPortal->portal_info.rooms[1];

				return 2;
			}
		}
	}

	if(radius > 50.0f)
	{
		int nRooms = 0;

		for(int i = 0; i < g_pLevel->m_numRooms; i++)
		{
			if(nRooms > 31)
				break;

			if(g_pLevel->m_pRooms[i].IsPointInsideRoom( point, radius ))
			{
				room_ids[nRooms] = i;
				nRooms++;
			}
		}

		return nRooms;
	}
	else
	{
		// find room that we inside
		for(int i = 0; i < g_pLevel->m_numRooms; i++)
		{
			if(g_pLevel->m_pRooms[i].IsPointInsideRoom( point, radius ))
			{
				room_ids[0] = i;
				return 1;
			}
		}
	}

	return 0;
}

// bbox checking for rooms (can return more than 2 rooms)
int CEqLevel::GetRoomsForBBox(const Vector3D &point, int* room_ids)
{
	return 0;
}

// returns portal between rooms
int	CEqLevel::GetFirstPortalLinkedToRoom( int startRoom, int endRoom, Vector3D& orient_to_pos, int maxDepth)
{
	// fancy checkks
	if(startRoom == -1)
		return -1;

	if(endRoom == -1)
		return -1;

	if(startRoom == endRoom)
		return -1;

	// start from the end room
	CEqRoom* pRoom = &m_pRooms[startRoom];
	CEqRoom* pCheckRoom = &m_pRooms[endRoom];

	// not linked
	if(pRoom->room_info.numPortals == 0)
		return -1;

	// get nearest portal in this room
	int nCheckPortal = -1;
	float fBestLength = MAX_COORD_UNITS;

	for(int i = 0; i < pRoom->room_info.numPortals; i++)
	{
		int portal_id = pRoom->room_info.portals[i];
		float fLen = length(m_pPortals[portal_id].m_bounding.GetCenter() - orient_to_pos);

		if(fLen < fBestLength)
		{
			fBestLength = fLen;
			nCheckPortal = i;
		}
	}
	
	int portal_id;

	// start from nearest portal
	if(nCheckPortal != -1)
	{
		int portal_id = pRoom->room_info.portals[nCheckPortal];

		CEqAreaPortal* pPortal = &g_pLevel->m_pPortals[portal_id];
			
		int nRoom = pPortal->portal_info.rooms[0];
		if(nRoom == startRoom)
			nRoom = pPortal->portal_info.rooms[1];

		if( CheckPortalLink_r(NULL, &m_pRooms[nRoom], pCheckRoom, 0, maxDepth) )
			return portal_id;
	}

	for(int i = 0; i < pRoom->room_info.numPortals; i++)
	{
		if(nCheckPortal == i)
			continue;

		portal_id = pRoom->room_info.portals[i];

		CEqAreaPortal* pPortal = &g_pLevel->m_pPortals[portal_id];

		int nRoom = pPortal->portal_info.rooms[0];
		if(nRoom == startRoom)
			nRoom = pPortal->portal_info.rooms[1];

		if( CheckPortalLink_r(NULL, &m_pRooms[nRoom], pCheckRoom, 0, maxDepth) )
			return portal_id;
	}

	return -1;
}

bool CEqLevel::CheckPortalLink( int startroom, int targetroom, int maxDepth )
{
	return CheckPortalLink_r(NULL, &m_pRooms[startroom], &m_pRooms[targetroom], 0, maxDepth);
}

bool CEqLevel::CheckPortalLink_r( CEqRoom* pPrevious, CEqRoom* pCurRoom, CEqRoom* pCheckRoom, int nDepth, int maxDepth )
{
	int nCurDepth = nDepth+1;

	if(nCurDepth == maxDepth)
	{
		return false;
	}

	for(int i = 0; i < pCurRoom->room_info.numPortals; i++)
	{
		int portal_id = pCurRoom->room_info.portals[i];

		CEqAreaPortal* pPortal = &g_pLevel->m_pPortals[portal_id];
			
		int nRoom = pPortal->portal_info.rooms[0];
		CEqRoom* pRoom = &m_pRooms[nRoom];

		if(pRoom == pPrevious)
			nRoom = pPortal->portal_info.rooms[1];

		pRoom = &m_pRooms[nRoom];

		if(pRoom == pCheckRoom)
			return true;

		if(CheckPortalLink_r( pCurRoom, pRoom, pCheckRoom, nCurDepth, maxDepth))
			return true;
	}

	return false;
}

// returns center of the portal
Vector3D CEqLevel::GetPortalPosition( int nPortal )
{
	if(nPortal == -1)
		return vec3_zero;

	return m_pPortals[nPortal].m_bounding.GetCenter();
}

// makes new render area list
renderAreaList_t* CEqLevel::CreateRenderAreaList()
{
	renderAreaList_t* list = new renderAreaList_t;
	list->area_list = new renderArea_t[m_numRooms];

	list->updated = false;

	// init
	for(int i = 0; i < m_numRooms; i++)
	{
		list->area_list[i].room = i;
		list->area_list[i].doRender = false;
		list->area_list[i].planeSets.resize(4);
	}

	m_RenderAreas.append(list);

	return list;
}

int	CEqLevel::GetRoomCount()
{
	return m_numRooms;
}

// releases area list
void CEqLevel::DestroyRenderAreaList(renderAreaList_t* pList)
{
	for(int i = 0; i < m_RenderAreas.numElem(); i++)
	{
		if(m_RenderAreas[i] == pList)
		{
			m_RenderAreas.removeIndex(i);

			delete [] pList->area_list;
			delete pList;

			return;
		}
	}
}

// returns list of translucent renderable objects
CBaseRenderableObject** CEqLevel::GetTranslucentObjectRenderables()
{
	return m_StaticTransparents.ptr();
}

int	CEqLevel::GetTranslucentObjectCount()
{
	return m_StaticTransparents.numElem();
}

// returns list of translucent renderable objects
CBaseRenderableObject** CEqLevel::GetStaticDecalRenderables()
{
	return (CBaseRenderableObject**)m_StaticDecals.ptr();
}

int	CEqLevel::GetStaticDecalCount()
{
	return m_StaticDecals.numElem();
}

IVertexFormat* CEqLevel::GetDecalVertexFormat()
{
	return m_pDecalVertexFormat;
}

// water info for physics objects and rendering
IWaterInfo* CEqLevel::GetNearestWaterInfo(const Vector3D& position)
{
	float fNearestDist = MAX_COORD_UNITS;
	IWaterInfo* pNearestWater = NULL;

	for(int i = 0; i < m_nWaterInfoCount; i++)
	{
		float dist = length(m_pWaterInfos[i].m_bounding.GetCenter() - position);
		if(dist < fNearestDist)
		{
			pNearestWater = &m_pWaterInfos[i];
			fNearestDist = dist;
		}
	}

	return pNearestWater;
}

// TODO: more complex checks, including GetBestVisibleWaterInfo( viewpos, frustum )

//-------------------------------------------------
// volume class
//-------------------------------------------------

CEqRoomVolume::CEqRoomVolume()
{
	m_pDrawSurfaceGroups = NULL;
	m_numDrawSurfaceGroups = 0;

	m_pPlanes = NULL;
}

void CEqRoomVolume::LoadRenderData(int room_id, eqroomvolume_t* volume, eqworldlump_t* pSurfaceLumps, eqworldlump_t* pPlanes)
{
	// copy draw surfaces
	m_numDrawSurfaceGroups = pSurfaceLumps->data_size / sizeof(eqlevelsurf_t);
	m_pDrawSurfaceGroups = (eqlevelsurf_t*)PPAlloc(pSurfaceLumps->data_size);
	memcpy( m_pDrawSurfaceGroups, ((ubyte*)pSurfaceLumps) + sizeof(eqworldlump_t), pSurfaceLumps->data_size );

	eqdrawsurfaceunion_t* pUnion = NULL;

	// add transparent surfaces
	for(int i = 0; i < m_numDrawSurfaceGroups; i++)
	{
		if(!pUnion)
		{
			pUnion = new eqdrawsurfaceunion_t;
			pUnion->first_index = m_pDrawSurfaceGroups[i].firstindex;
			pUnion->first_vertex = m_pDrawSurfaceGroups[i].firstvertex;
			pUnion->numindices = m_pDrawSurfaceGroups[i].numindices;
			pUnion->numverts = m_pDrawSurfaceGroups[i].numvertices;
			pUnion->material_id = m_pDrawSurfaceGroups[i].material_id;
			pUnion->flags = m_pDrawSurfaceGroups[i].flags;
			pUnion->bbox.Reset();

			pUnion->bbox.AddVertex(m_pDrawSurfaceGroups[i].mins);
			pUnion->bbox.AddVertex(m_pDrawSurfaceGroups[i].maxs);

			m_drawSurfaceUnions.append(pUnion);
		}

		if(pUnion->material_id != m_pDrawSurfaceGroups[i].material_id)
		{
			// make new
			pUnion = NULL;
		}
		else
		{
			pUnion->numindices += m_pDrawSurfaceGroups[i].numindices;
			pUnion->numverts += m_pDrawSurfaceGroups[i].numvertices;
			pUnion->bbox.AddVertex( m_pDrawSurfaceGroups[i].mins );
			pUnion->bbox.AddVertex( m_pDrawSurfaceGroups[i].maxs );
		}

		if( m_pDrawSurfaceGroups[i].flags & EQSURF_FLAG_TRANSLUCENT )
		{
			CWorldTransparentRenderable* pRenderable = new CWorldTransparentRenderable();
			pRenderable->m_nRoomID = room_id;
			pRenderable->info = m_pDrawSurfaceGroups[i];

			g_pLevel->m_StaticTransparents.append( pRenderable );
		}
	}

	volume_info = *volume;

	m_bounding.minPoint = volume_info.mins;
	m_bounding.maxPoint = volume_info.maxs;

	m_pPlanes = (Plane*)PPAlloc(volume_info.numPlanes * sizeof(Plane));

	Plane* planeData = (Plane*)(((ubyte*)pPlanes) + sizeof(eqworldlump_t));
	memcpy(m_pPlanes, planeData + volume_info.firstPlane, volume_info.numPlanes * sizeof(Plane));
}

void CEqRoomVolume::FreeData()
{
	PPFree(m_pDrawSurfaceGroups);
	PPFree(m_pPlanes);

	for(int i = 0; i < m_drawSurfaceUnions.numElem(); i++)
	{
		delete m_drawSurfaceUnions[i];
	}

	m_drawSurfaceUnions.clear();

	m_pDrawSurfaceGroups = NULL;
}

bool CEqRoomVolume::IsPointInsideVolume(const Vector3D &point, float radius)
{
	BoundingBox bbox = m_bounding;
	bbox.minPoint -= radius;
	bbox.maxPoint += radius;

	if(!bbox.Contains(point))
		return false;

	for(int i = 0; i < volume_info.numPlanes; i++)
	{
		float fDist = m_pPlanes[i].Distance(point);

		if(fDist > (0.1f+radius))
			continue;

		return true;
	}

	return false;
}

//-------------------------------------------------
// room class
//-------------------------------------------------

CEqRoom::CEqRoom()
{
	m_pRoomVolumes = NULL;
	m_numRoomVolumes = 0;

	memset(m_pPortalSides, 0, sizeof(m_pPortalSides));
}

void CEqRoom::Init(eqroom_t* info)
{
	room_info = *info;

	m_numRoomVolumes = room_info.numVolumes;

	m_bounding.minPoint = room_info.mins;
	m_bounding.maxPoint = room_info.maxs;
}

void CEqRoom::FreeData()
{
	if(m_pRoomVolumes)
	{
		for(int i = 0; i < m_numRoomVolumes; i++)
			m_pRoomVolumes[i].FreeData();

		delete [] m_pRoomVolumes;
	}

	m_numRoomVolumes = 0;
}

bool CEqRoom::IsPointInsideRoom(const Vector3D& position, float radius)
{
	BoundingBox bbox = m_bounding;
	bbox.minPoint -= radius;
	bbox.maxPoint += radius;

	if(!bbox.Contains(position))
		return false;

	for(int i = 0; i < m_numRoomVolumes; i++)
	{
		if(m_pRoomVolumes[i].IsPointInsideVolume( position, radius ))
			return true;
	}

	return false;
}

// portals
CEqAreaPortal::CEqAreaPortal()
{
	m_vertices[0] = NULL;
	m_vertices[1] = NULL;
}

void CEqAreaPortal::Init(eqareaportal_t *portal, eqworldlump_t* pVerts, eqworldlump_t* pPlanes)
{
	portal_info = *portal;
	m_bounding.minPoint = portal_info.mins;
	m_bounding.maxPoint = portal_info.maxs;

	Plane* planeData = (Plane*)(((ubyte*)pPlanes) + sizeof(eqworldlump_t));

	Vector3D* verts = (Vector3D*)(((ubyte*)pVerts) + sizeof(eqworldlump_t));

	m_planes[0] = planeData[portal_info.portalPlanes[0]];
	m_planes[1] = planeData[portal_info.portalPlanes[1]];

	m_vertices[0] = (Vector3D*)malloc(portal_info.numVerts[0] * sizeof(Vector3D));
	m_vertices[1] = (Vector3D*)malloc(portal_info.numVerts[1] * sizeof(Vector3D));

	memcpy(m_vertices[0], verts + portal_info.firstVertex[0], portal_info.numVerts[0] * sizeof(Vector3D));
	memcpy(m_vertices[1], verts + portal_info.firstVertex[1], portal_info.numVerts[1] * sizeof(Vector3D));
}

void CEqAreaPortal::FreeData()
{
	if(m_vertices[0])
		free(m_vertices[0]);

	if(m_vertices[1])
		free(m_vertices[1]);

	m_vertices[0] = NULL;
	m_vertices[1] = NULL;
}

//----------------------------------------------------------------------------------------
// Water volumes
//----------------------------------------------------------------------------------------

CEqWaterVolume::CEqWaterVolume()
{
	m_pPlanes = NULL;
}

void CEqWaterVolume::LoadData(int water_id, eqroomvolume_t* volume, eqworldlump_t* pPlanes)
{
	volume_info = *volume;

	m_bounding.minPoint = volume_info.mins;
	m_bounding.maxPoint = volume_info.maxs;

	m_pPlanes = (Plane*)PPAlloc(volume_info.numPlanes * sizeof(Plane));

	Plane* planeData = (Plane*)(((ubyte*)pPlanes) + sizeof(eqworldlump_t));
	memcpy(m_pPlanes, planeData + volume_info.firstPlane, volume_info.numPlanes * sizeof(Plane));
}

void CEqWaterVolume::FreeData()
{
	PPFree(m_pPlanes);
}

bool CEqWaterVolume::IsPointInsideVolume(const Vector3D &point, float radius)
{
	BoundingBox bbox = m_bounding;
	bbox.minPoint -= radius;
	bbox.maxPoint += radius;

	if(!bbox.Contains(point))
		return false;

	for(int i = 0; i < volume_info.numPlanes; i++)
	{
		float fDist = m_pPlanes[i].Distance(point);

		if(fDist > (0.1f+radius))
			continue;

		return true;
	}

	return false;
}

//-------------------------------------------------
// water info class
//-------------------------------------------------

CEqWaterInfo::CEqWaterInfo()
{

}

void CEqWaterInfo::Init(eqwaterinfo_t* info)
{
	water_info = *info;

	m_numWaterVolumes = water_info.numVolumes;

	m_bounding.minPoint = water_info.mins;
	m_bounding.maxPoint = water_info.maxs;
}

void CEqWaterInfo::FreeData()
{
	if(m_pWaterVolumes)
	{
		for(int i = 0; i < m_numWaterVolumes; i++)
			m_pWaterVolumes[i].FreeData();

		delete [] m_pWaterVolumes;
	}

	m_numWaterVolumes = 0;
}

bool CEqWaterInfo::IsPointInsideWater(const Vector3D& position, float radius)
{
	BoundingBox bbox = m_bounding;
	bbox.minPoint -= radius;
	bbox.maxPoint += radius;

	if(!bbox.Contains(position))
		return false;

	for(int i = 0; i < m_numWaterVolumes; i++)
	{
		if(m_pWaterVolumes[i].IsPointInsideVolume( position, radius ))
			return true;
	}

	return false;
}

float CEqWaterInfo::GetWaterHeightLevel()
{
	return water_info.waterHeight;
}

BoundingBox CEqWaterInfo::GetBounds()
{
	return m_bounding;
}