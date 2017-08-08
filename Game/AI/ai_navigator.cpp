//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation mesh pathfinding
//////////////////////////////////////////////////////////////////////////////////

/*
USING GROUND NODES:

	First we've need a CAINavWorld, that probably will be renamed from CAINavigator.
	Next is an new CAINavigator for single instance of AI character that uses CAINavWorld
	for creating way points. CAINavigator will contain information about path visibility,
	will produce visibility information for any object and compute possible pathways in specified range.
	Just GetPossiblePathsTo(entity, outpaths, maxdist), outpaths are CAINavigationPaths, sorted by walk distance ascendingly. 
	MaxDist is maximum summary walk distance of all generated waypoints.
	GetBestNearestPath finds shortest and straightest path using best of GetPossiblePathsTo

	First pass we've check walkability to nodes. Check way occlusion with doors.
	Next pass we do findNearestPoly and findPath to the nodes. Also do straght test to the target position.
	Append the polyrefs because we connect nodes
	Do findStraightPath and record result

	NOTE:	using too many ground nodes will produce huge memory usage. So need to select from distance
			variations or check some conditions like visibility by enemy AND do that in second pass
	NOTE:	don't use recursive algorhitm
*/

#include "ai_navigator.h"
#include "Math/BoundingBox.h"
#include "Utils/GeomTools.h"
#include "AINode.h"

#include "eqlevel.h"

#define MAX_PATHSLOT	128			// how many paths we can store
#define MAX_PATHPOLY	256			// max number of polygons in a path
#define MAX_PATHVERT	512			// most verts in a path 

// polygon area
enum PolyAreas_e
{
	POLYAREA_GROUND = 0,
	POLYAREA_DOOR,
	POLYAREA_JUMP,
};

enum PolyFlags_e
{
	POLYFLAGS_WALK	= (1 << 0),			// Ability to walk (ground, grass, road)
	POLYFLAGS_DOOR	= (1 << 1),			// Ability to move through doors.
	POLYFLAGS_JUMP	= (1 << 2),			// Ability to jump.

	POLYFLAGS_ALL	= 0xffff			// All abilities.
};

ConVar ai_nav_debugdraw("ai_nav_debugdraw",  "0", "Draw all waypoints and connections", CV_CHEAT);
ConVar ai_nav_debugdraw_routes("ai_nav_debugdraw_routes",  "0", "Draw routes and waypoints", CV_CHEAT);

CAINavigationPath::CAINavigationPath()
{	
//	m_bIsReady = true;
	m_nCurrWaypoint = 0;
}

CAINavigationPath::~CAINavigationPath()
{

}

WayPoint_t& CAINavigationPath::GetStart()
{
	if(!m_WayPoints.numElem())
		return zeroPoint;

	return m_WayPoints[0];
}

WayPoint_t& CAINavigationPath::GetGoal()
{
	if(!m_WayPoints.numElem())
		return zeroPoint;

	return m_WayPoints[m_WayPoints.numElem()-1];
}

int CAINavigationPath::GetNumWayPoints()
{
	return m_WayPoints.numElem();
}

WayPoint_t& CAINavigationPath::GetWaypoint(int id)
{
	if(!m_WayPoints.numElem())
		return zeroPoint;

	return m_WayPoints[id];
}

WayPoint_t& CAINavigationPath::GetCurrentWayPoint()
{
	if(m_WayPoints.numElem() < 1)
		return zeroPoint;

	return m_WayPoints[m_nCurrWaypoint];
}

void CAINavigationPath::AdvanceWayPoint()
{
	//Msg("Way advanced\n");

	m_nCurrWaypoint++;

	if(m_nCurrWaypoint >= m_WayPoints.numElem()-1)
	{
		m_nCurrWaypoint = m_WayPoints.numElem()-1;
		//m_WayPoints.clear();
	}
}

float CAINavigationPath::GetWayLength()
{
	return m_fWayLength;
}

void CAINavigationPath::Clear()
{
	m_WayPoints.clear();
	m_nCurrWaypoint = 0;
}

void CAINavigationPath::DebugRender()
{
	if(!ai_nav_debugdraw_routes.GetBool())
		return;

	//if(!m_bIsReady)
	//	return;

	for(int i = 0; i < m_WayPoints.numElem(); i++)
		debugoverlay->Box3D(m_WayPoints[i].position - Vector3D(5), m_WayPoints[i].position + Vector3D(5), ColorRGBA(1,0,0,1));

	for(int i = 0; i < m_WayPoints.numElem()-1; i++)
		debugoverlay->Line3D(m_WayPoints[i].position, m_WayPoints[i+1].position, ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1), 0);
}

CAINavigator::CAINavigator()
{
	m_bHasPathfinding = false;

	m_pMesh = NULL;

	m_pRcCtx = NULL;

	m_pNavMesh = NULL;
	m_pNavQuery = NULL;

	m_flDebugDrawNextTime = 0;
}

CAINavigator::~CAINavigator()
{
	Shutdown();
}

#define NAVMESH_IDENT		MCHAR4('E','N','A','V')
#define NAVMESH_VERSION		1

struct navmesh_hdr_t
{
	int		ident;
	int		version;
	int		size;
};

void CAINavigator::InitNavigationMesh()
{
	if(m_bHasPathfinding)
		return;

	// TODO: check worldinfo for no_ai flag

	// try to load generated navigation mesh

	EqString nav_file_name(varargs("worlds/%s/navmesh.ai", gpGlobals->worldname));

	if(g_fileSystem->FileExist( nav_file_name.GetData() ))
	{
		Msg("Loading AI navigation mesh from '%s'\n", nav_file_name.GetData());

		IFile* pFile = g_fileSystem->Open( nav_file_name.GetData(), "rb", SP_MOD );
		if(pFile)
		{
			navmesh_hdr_t hdr;

			// write header
			pFile->Read(&hdr, 1, sizeof(navmesh_hdr_t));

			if(hdr.ident == NAVMESH_IDENT && hdr.version == NAVMESH_VERSION)
			{
				// use detour allocator
				ubyte* navData = (ubyte*)dtAlloc(hdr.size, DT_ALLOC_PERM);

				// read navigation data
				pFile->Read(navData, 1, hdr.size);

				m_pNavMesh = dtAllocNavMesh();
		
				m_pNavMesh->init(navData, hdr.size, DT_TILE_FREE_DATA);

				m_pNavQuery = dtAllocNavMeshQuery();
				m_pNavQuery->init(m_pNavMesh, 2048);

				m_bHasPathfinding = true;
			}
			else
				MsgWarning("Bad navigation mesh format or version, rebuilding\n");

			g_fileSystem->Close(pFile);
		}
	}

	// post check
	if(m_bHasPathfinding)
		return;

	Msg("Building AI navigation mesh...\n");

	// generate from physics geometry
	eqphysicsvertex_t*	pPhysVertexData;
	int*				pPhysIndexData;

	eqlevelphyssurf_t*	pPhysSurfaces;
	int					numPhysSurfaces;

	int					numPhysIndices;

	numPhysSurfaces = eqlevel->GetPhysicsSurfaces(&pPhysVertexData, &pPhysIndexData, &pPhysSurfaces, numPhysIndices);

	if(!numPhysSurfaces)
		return;

	BoundingBox world_bbox;
	world_bbox.Reset();

	int nTriangles = 0;
	int nVertices = 0;

	DkList<Vector3D>	verts;
	DkList<int>			indices;

	verts.resize(numPhysIndices);
	indices.resize(numPhysIndices);

	//int* back_vert_remap = new int[numPhysIndices];

	for(int i = 0; i < numPhysSurfaces; i++)
	{
		eqlevelphyssurf_t* pSurface = &pPhysSurfaces[i];

		for(int j = 0; j < pSurface->numvertices; j++)
		{
			world_bbox.AddVertex(pPhysVertexData[pSurface->firstvertex+j].position);
			verts.append(pPhysVertexData[pSurface->firstvertex+j].position);
		}

		for(int j = 0; j < pSurface->numindices; j++)
			indices.append(pPhysIndexData[pSurface->firstindex+j]);

		nVertices += pSurface->numvertices;
		nTriangles += pSurface->numindices / 3;
	}

	// (Optional) Mark areas.
	for (int i = 0; i < ents->numElem(); ++i)
	{
		BaseEntity* pEnt = (BaseEntity*)ents->ptr()[i];
		if(!stricmp(pEnt->GetClassname(), "prop_static") && pEnt->GetModel())
		{
			physmodeldata_t* pPhysModel = &pEnt->GetModel()->GetHWData()->m_physmodel;

			int area_count = pPhysModel->numIndices / 3;
			
			int start_idx = verts.numElem();

			Matrix4x4 ent_transform = ComputeWorldMatrix(pEnt->GetAbsOrigin(), pEnt->GetAbsAngles(), Vector3D(1));

			for(int j = 0; j < pPhysModel->numVertices; j++)
			{
				world_bbox.AddVertex(pPhysModel->vertices[j]);
				verts.append((ent_transform * Vector4D(pPhysModel->vertices[j], 1.0f)).xyz());
			}

			for(int j = 0; j < pPhysModel->numIndices; j++)
				indices.append(pPhysModel->indices[j]+start_idx);

			nVertices += pPhysModel->numVertices;
			nTriangles += pPhysModel->numIndices / 3;
		}
	}

	m_pRcCtx = new rcContext(true);

	// cellsize (1.5, 1.0) was the most accurate at finding all the places we could go, but was also slow to generate.
	// Might be suitable for pre-generated meshes. Though it also produces a lot more polygons.

	float cellSize = 12.0f;//0.3;
	float cellHeight = 5.0f ;//0.2;
	float agentHeight = 100.0f;
	float agentRadius = 12.0f;
	float agentMaxClimb = 32.0f;
	float agentMaxSlope = 45.0f;
	float regionMinSize = 4.0f;
	float regionMergeSize = 10.0f;
	float edgeMaxLen = 1024.0f;
	float edgeMaxError = 1.3f;
	float vertsPerPoly = 6.0f;
	float detailSampleDist = 6.0f;
	float detailSampleMaxError = 1.0f;
   
	// Init build configuration
	memset(&m_rCfg, 0, sizeof(m_rCfg));
	m_rCfg.cs = cellSize;
	m_rCfg.ch = cellHeight;
	m_rCfg.walkableSlopeAngle = agentMaxSlope;
	m_rCfg.walkableHeight = (int)ceilf(agentHeight / m_rCfg.ch);
	m_rCfg.walkableClimb = (int)floorf(agentMaxClimb / m_rCfg.ch);
	m_rCfg.walkableRadius = (int)ceilf(agentRadius / m_rCfg.cs);
	m_rCfg.maxEdgeLen = (int)(edgeMaxLen / cellSize);
	m_rCfg.maxSimplificationError = edgeMaxError;
	m_rCfg.minRegionArea = (int)rcSqr(regionMinSize);      // Note: area = size*size
	m_rCfg.mergeRegionArea = (int)rcSqr(regionMergeSize);   // Note: area = size*size
	m_rCfg.maxVertsPerPoly = (int)vertsPerPoly;
	m_rCfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
	m_rCfg.detailSampleMaxError = cellHeight * detailSampleMaxError;

	// Reset build times gathering.
	m_pRcCtx->resetTimers();

	// Start the build process.   
	float fGenTime = Platform_GetCurrentTime();

	rcVcopy(m_rCfg.bmin, world_bbox.GetMinPoint());
	rcVcopy(m_rCfg.bmax, world_bbox.GetMaxPoint());
	rcCalcGridSize(m_rCfg.bmin, m_rCfg.bmax, m_rCfg.cs, &m_rCfg.width, &m_rCfg.height);

	// Allocate voxel heightfield where we rasterize our input data to.
	rcHeightfield* pSolid = rcAllocHeightfield();

	if (!rcCreateHeightfield(m_pRcCtx, *pSolid, m_rCfg.width, m_rCfg.height, m_rCfg.bmin, m_rCfg.bmax, m_rCfg.cs, m_rCfg.ch))
	{
		MsgError("InitNavigationMesh: Could not create solid heightfield.");
		rcFreeHeightField(pSolid);

		if(m_pRcCtx)
			delete m_pRcCtx;

		m_pRcCtx = NULL;

		return;
	}
   
	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	ubyte* pTriAreas = new ubyte[nTriangles];
	memset(pTriAreas, 0, nTriangles*sizeof(ubyte));

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.

	rcMarkWalkableTriangles( m_pRcCtx, m_rCfg.walkableSlopeAngle, (float*)verts.ptr(), nVertices, indices.ptr(), nTriangles, pTriAreas );
	rcRasterizeTriangles(m_pRcCtx, (float*)verts.ptr(), nVertices, indices.ptr(), pTriAreas, nTriangles, *pSolid, m_rCfg.walkableClimb);

	delete [] pTriAreas;

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(m_pRcCtx, m_rCfg.walkableClimb, *pSolid);
	rcFilterLedgeSpans(m_pRcCtx, m_rCfg.walkableHeight, m_rCfg.walkableClimb, *pSolid);
	rcFilterWalkableLowHeightSpans(m_pRcCtx, m_rCfg.walkableHeight, *pSolid);

	// Partition walkable surface to simple regions.

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.

	rcCompactHeightfield* pChf = rcAllocCompactHeightfield();

	if (!rcBuildCompactHeightfield(m_pRcCtx, m_rCfg.walkableHeight, m_rCfg.walkableClimb, *pSolid, *pChf))
	{
		MsgError("InitNavigationMesh: Could not build compact heightfield data.");
		rcFreeHeightField(pSolid);
		rcFreeCompactHeightfield(pChf);

		if(m_pRcCtx)
			delete m_pRcCtx;

		m_pRcCtx = NULL;

		return;
	}
   
	// after we working with compact heightfield
	rcFreeHeightField( pSolid );
      
	// Erode the walkable area by agent radius.
	rcErodeWalkableArea(m_pRcCtx, m_rCfg.walkableRadius, *pChf);

	// rcMarkConvexPolyArea(m_pRcCtx, &pPhysModel->vertices, vols[i].nverts, vols[i].hmin, vols[i].hmax, area_count, *pChf);
   
	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	rcBuildDistanceField(m_pRcCtx, *pChf);

	// Partition the walkable surface into simple regions without holes.
	rcBuildRegions(m_pRcCtx, *pChf, m_rCfg.borderSize, m_rCfg.minRegionArea, m_rCfg.mergeRegionArea);

	// Trace and simplify region contours.
   
	// Create contours.
	rcContourSet* pCset = rcAllocContourSet();
	rcBuildContours(m_pRcCtx, *pChf, m_rCfg.maxSimplificationError, m_rCfg.maxEdgeLen, *pCset);
   
	// Build polygon navmesh from the contours.
	m_pMesh = rcAllocPolyMesh();

	if (!rcBuildPolyMesh(m_pRcCtx, *pCset, m_rCfg.maxVertsPerPoly, *m_pMesh))
	{
		MsgError("InitNavigationMesh: Could not triangulate contours.\n");

		rcFreeCompactHeightfield(pChf);
		rcFreeContourSet(pCset);
		rcFreePolyMesh(m_pMesh);

		if(m_pRcCtx)
			delete m_pRcCtx;

		m_pRcCtx = NULL;

		return;
	}
 
	// Create detail mesh which allows to access approximate height on each polygon.
	m_pDetailMesh = rcAllocPolyMeshDetail();
	rcBuildPolyMeshDetail(m_pRcCtx, *m_pMesh, *pChf, m_rCfg.detailSampleDist, m_rCfg.detailSampleMaxError, *m_pDetailMesh);

	rcFreeCompactHeightfield(pChf);
	rcFreeContourSet(pCset);

	// At this point the navigation mesh data is ready, we can access it from m_pmesh.
	if (m_rCfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		ubyte* navData = NULL;
		int navDataSize = 0;

		// Update poly flags from areas.
		for (int i = 0; i < m_pMesh->npolys; ++i)
		{
			if (m_pMesh->areas[i] == RC_WALKABLE_AREA)
			{
				m_pMesh->areas[i] = POLYAREA_GROUND;
				m_pMesh->flags[i] = POLYFLAGS_WALK;
			}
		}

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));

		params.verts = m_pMesh->verts;
		params.vertCount = m_pMesh->nverts;
		params.polys = m_pMesh->polys;
		params.polyAreas = m_pMesh->areas;
		params.polyFlags = m_pMesh->flags;
		params.polyCount = m_pMesh->npolys;
		params.nvp = m_pMesh->nvp;
		params.detailMeshes = m_pDetailMesh->meshes;
		params.detailVerts = m_pDetailMesh->verts;
		params.detailVertsCount = m_pDetailMesh->nverts;
		params.detailTris = m_pDetailMesh->tris;
		params.detailTriCount = m_pDetailMesh->ntris;

		// no off mesh connections yet
		params.offMeshConVerts = NULL ;
		params.offMeshConRad = NULL ;
		params.offMeshConDir = NULL ;
		params.offMeshConAreas = NULL ;
		params.offMeshConFlags = NULL ;
		params.offMeshConUserID = NULL ;
		params.offMeshConCount = 0 ;

		params.walkableHeight = agentHeight;
		params.walkableRadius = agentRadius;
		params.walkableClimb = agentMaxClimb;
		rcVcopy(params.bmin, m_pMesh->bmin);
		rcVcopy(params.bmax, m_pMesh->bmax);
		params.cs = m_rCfg.cs;
		params.ch = m_rCfg.ch;

		if (params.nvp > DT_VERTS_PER_POLYGON)
			MsgError("Max NVP\n");
		if (params.vertCount >= 0xffff)
			MsgError("-1\n");
		if (!params.vertCount || !params.verts)
			MsgError("no verts\n");
		if (!params.polyCount || !params.polys)
			MsgError("no polys\n");

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			MsgError("Could not build Detour navmesh.\n");

			rcFreePolyMesh(m_pMesh);

			rcFreePolyMeshDetail(m_pDetailMesh);

			m_pDetailMesh = NULL;
			m_pMesh = NULL;

			if(m_pRcCtx)
				delete m_pRcCtx;

			m_pRcCtx = NULL;

			return;
		}

		// save generated navigation mesh
		IFile* pFile = g_fileSystem->Open( nav_file_name.GetData() , "wb", SP_MOD);
		if(pFile)
		{
			navmesh_hdr_t hdr;
			hdr.ident = NAVMESH_IDENT;
			hdr.version = NAVMESH_VERSION;
			hdr.size = navDataSize;

			// write header
			pFile->Write(&hdr, 1, sizeof(navmesh_hdr_t));

			// write navigation data
			pFile->Write(navData, 1, navDataSize);

			g_fileSystem->Close(pFile);
		}

		m_pNavMesh = dtAllocNavMesh();
		
		m_pNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);

		m_pNavQuery = dtAllocNavMeshQuery();
		m_pNavQuery->init(m_pNavMesh, 2048);
	}
   
	m_pRcCtx->stopTimer(RC_TIMER_TOTAL);

	m_bHasPathfinding = true;

	Msg("Navigation mesh generated (%g seconds)\n", Platform_GetCurrentTime() - fGenTime);
}

ConVar ai_nav_searchbox_size("ai_nav_searchbox_size", "128", "Search box size", CV_ARCHIVE);

void CAINavigator::FindPath(CAINavigationPath* pPath, Vector3D &start, Vector3D &target)
{
	if(!m_bHasPathfinding)
		return;

	// init path
	pPath->m_WayPoints.clear();
	pPath->m_nCurrWaypoint = 0;

	// first needs to check ground nodes for door occluders and other shit things



	dtStatus status;
	Vector3D search_box_size(ai_nav_searchbox_size.GetFloat());

	dtPolyRef StartPoly;
	Vector3D StartNearest;

	dtPolyRef EndPoly;
	Vector3D EndNearest;

	dtPolyRef PolyPath[MAX_PATHPOLY];
	int nPathCount = 0;

	Vector3D StraightPath[MAX_PATHVERT];
	int nVertCount = 0;

	// setup the filter
	dtQueryFilter Filter;
	Filter.setIncludeFlags(0xFFFF) ;
	Filter.setExcludeFlags(0) ;
	Filter.setAreaCost(POLYAREA_GROUND, 1.0f) ;
	Filter.setAreaCost(POLYAREA_JUMP, 0.5f) ;

	// find the start polygon
	status = m_pNavQuery->findNearestPoly(start, search_box_size, &Filter, &StartPoly, StartNearest);
	if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) 
		return;

	// find the end polygon
	status = m_pNavQuery->findNearestPoly(target, search_box_size, &Filter, &EndPoly, EndNearest);
	if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
		return;

	status = m_pNavQuery->findPath(StartPoly, EndPoly, StartNearest, EndNearest, &Filter, PolyPath, &nPathCount, MAX_PATHPOLY) ;
	if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
		return;

	if(!nPathCount)
		return;

	// straighten the path
	status = m_pNavQuery->findStraightPath(StartNearest, EndNearest, PolyPath, nPathCount, (float*)StraightPath, NULL, NULL, &nVertCount, MAX_PATHVERT);
	
	if((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
		return; // couldn't create a path

	if(!nVertCount) 
		return;

	// At this point we have our path.  Copy it to the path store
	// also calculate path normals for advancing the waypoints
	for(int i = 1; i < nVertCount ; i++)
	{
		WayPoint_t way;
		way.position = StraightPath[i-1];

		Vector3D normal = normalize( StraightPath[i] - StraightPath[i-1]);

		way.plane = Plane(normal, -dot(normal, StraightPath[i-1]));

		pPath->m_WayPoints.append(way);
	}

	if(pPath->m_WayPoints.numElem() > 0)
	{
		WayPoint_t lastway;
		lastway.position = StraightPath[nVertCount-1];
		lastway.plane = pPath->m_WayPoints[pPath->m_WayPoints.numElem()-1].plane;

		pPath->m_WayPoints.append(lastway);
	}

	if(pPath->m_WayPoints.numElem())
		pPath->zeroPoint = pPath->m_WayPoints[pPath->m_WayPoints.numElem()-1];
}

void CAINavigator::Shutdown()
{
	m_flDebugDrawNextTime = 0;

	m_pGroundNodes.clear();

	if(m_bHasPathfinding)
	{
		if(m_pMesh)
			rcFreePolyMesh(m_pMesh);

		if(m_pDetailMesh)
			rcFreePolyMeshDetail(m_pDetailMesh);

		m_pDetailMesh = NULL;
		m_pMesh = NULL;

		dtFreeNavMesh(m_pNavMesh);
		m_pNavMesh = NULL;

		dtFreeNavMeshQuery(m_pNavQuery);
		m_pNavQuery = NULL;

		if(m_pRcCtx)
			delete m_pRcCtx;

		m_pRcCtx = NULL;

		m_bHasPathfinding = false;
	}
}

void CAINavigator::DebugRender()
{
	if(!ai_nav_debugdraw.GetBool() || !m_bHasPathfinding)
		return;

	// render graph first
	for(int i = 0; i < m_pGroundNodes.numElem(); i++)
	{
		for(int j = 0; j < m_pGroundNodes[i]->m_pLinkedNodes.numElem(); j++)
		{
			debugoverlay->Line3D(m_pGroundNodes[i]->GetAbsOrigin(), m_pGroundNodes[i]->m_pLinkedNodes[j]->GetAbsOrigin(),
									ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), 0.0f);
		}
	}

	if(!m_pMesh)
		return;

	if(m_flDebugDrawNextTime > gpGlobals->realtime)
		return;

	m_flDebugDrawNextTime = gpGlobals->realtime + 5.0f;

	int nvp = m_pMesh->nvp; 
	float cs = m_pMesh->cs;
	float ch = m_pMesh->ch;
	float* orig = m_pMesh->bmin;

	for (int i = 0; i < m_pMesh->npolys; ++i) // go through all polygons
	{
		if (m_pMesh->areas[i] == POLYAREA_GROUND)
		{
			const unsigned short* p = &m_pMesh->polys[i*nvp*2];

			unsigned short vi[3];
			for (int j = 2; j < nvp; ++j) // go through all verts in the polygon
			{
				if (p[j] == RC_MESH_NULL_IDX)
					break;

				vi[0] = p[0];
				vi[1] = p[j-1];
				vi[2] = p[j];

				Vector3D poly[3];

				for (int k = 0; k < 3; ++k) // create a 3-vert triangle for each 3 verts in the polygon.
				{
					unsigned short* v = &m_pMesh->verts[vi[k]*3];
					float x = orig[0] + v[0]*cs;
					float y = orig[1] + (v[1]+1)*ch;
					float z = orig[2] + v[2]*cs;

					poly[k] = Vector3D(x,y,z);
				}

				if (m_pMesh->areas[i] == POLYAREA_GROUND)
					debugoverlay->Polygon3D(poly[0],poly[1],poly[2], ColorRGBA(0,0.5,0.5,0.5f),5.0f) ;
				else
					debugoverlay->Polygon3D(poly[0],poly[1],poly[2], ColorRGBA(0.5,0,0,0.5f),5.0f) ;
			}
		}
	}
}

void CAINavigator::AddGroundNode( CAINodeGround* pGroundNode )
{
	m_pGroundNodes.append( pGroundNode );
}

static CAINavigator s_Navmesh;
CAINavigator*		g_pAINavigator = &s_Navmesh;