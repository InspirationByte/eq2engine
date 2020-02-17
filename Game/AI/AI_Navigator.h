//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation mesh pathfinding
//////////////////////////////////////////////////////////////////////////////////

#ifndef AI_NAVMESH_PATHFINDING
#define AI_NAVMESH_PATHFINDING

#include "BaseEngineHeader.h"

// recast
#include "Recast.h"

// detour
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"

struct WayPoint_t
{
	WayPoint_t()
	{
		position = vec3_zero;
		plane = Plane(0,0,1,1);
	}

	Vector3D	position;
	Plane		plane;
};


//-------------------------------------------------
// CAINavigationPath - Navigation path
// used by entities inside
//-------------------------------------------------
class CAINavigationPath
{
public:
	friend class CAINavigator;

	CAINavigationPath();
	~CAINavigationPath();

	WayPoint_t&			GetStart();
	WayPoint_t&			GetGoal();

	void				Clear();

	int					GetNumWayPoints();
	WayPoint_t&			GetWaypoint(int id);

	WayPoint_t&			GetCurrentWayPoint();
	void				AdvanceWayPoint();

	float				GetWayLength();

	void				DebugRender();

protected:

	WayPoint_t			zeroPoint;
	DkList<WayPoint_t>	m_WayPoints;

	float				m_fWayLength;

	int					m_nCurrWaypoint;

	//bool				m_bIsReady;
};

class CAINodeGround;
class CAINodeAir;

//-------------------------------------------------
// CAINavmeshNavigator - Navigation mesh and points
//-------------------------------------------------
class CAINavigator
{
public:

	CAINavigator();
	~CAINavigator();

	void					InitNavigationMesh();
	void					Shutdown();

	void					FindPath(CAINavigationPath* pPath, Vector3D &start, Vector3D &target);

	void					DebugRender();

	void					AddGroundNode( CAINodeGround* pGroundNode );

protected:

	bool					m_bHasPathfinding;

	rcConfig				m_rCfg;
	rcContext*				m_pRcCtx;

	rcPolyMeshDetail*		m_pDetailMesh;
	rcPolyMesh*				m_pMesh;
	dtNavMesh*				m_pNavMesh;
	dtNavMeshQuery*			m_pNavQuery;

	float					m_flDebugDrawNextTime;

	DkList<CAINodeGround*>	m_pGroundNodes;
};

extern CAINavigator*		g_pAINavigator;

#endif // AI_NAVMESH_PATHFINDING