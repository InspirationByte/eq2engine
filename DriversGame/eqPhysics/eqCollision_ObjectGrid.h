//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object grid
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQCOLLISION_OBJECTGRID_H
#define EQCOLLISION_OBJECTGRID_H

#include "math/Vector.h"
#include "utils/DkList.h"
#include "ppmem.h"

class CEqCollisionObject;
class CEqRigidBody;
class CEqPhysics;

struct collgridcell_t
{
	PPMEM_MANAGED_OBJECT_TAG("physGridCell")

	short	x,y;
	float	cellBoundUsed;	// unsigned z of usage by static objects

	DkList<CEqCollisionObject*>		m_gridObjects;
	DkList<CEqCollisionObject*>		m_dynamicObjs;
};

class CEqCollisionBroadphaseGrid
{
public:
	CEqCollisionBroadphaseGrid();
	~CEqCollisionBroadphaseGrid();

	void				Init(CEqPhysics* physics, int gridsize, const Vector3D& worldmins, const Vector3D& worldmaxs);
	void				Destroy();

	bool				IsInit() const;

	collgridcell_t*		GetPreallocatedCellAtPos(const Vector3D& origin);

	collgridcell_t*		GetCellAtPos(const Vector3D& origin) const;
	collgridcell_t*		GetCellAt(int x, int y) const;

	bool				GetPointAt(const Vector3D& origin, int& x, int& y) const;

	void				AddStaticObjectToGrid( CEqCollisionObject* m_collisionObject );
	void				RemoveStaticObjectFromGrid( CEqCollisionObject* m_collisionObject );

	bool				GetCellBounds(int x, int y, Vector3D& mins, Vector3D& maxs) const;

	void				FindBoxRange(const Vector3D& mins, const Vector3D& maxs, IVector2D& cr_min, IVector2D& cr_max, float extTolerance) const;

	void				DebugRender();

	// TODO: query line, box, sphere

protected:

	collgridcell_t*		GetAllocCellAt(int x, int y);
	void				FreeCellAt( int x, int y );

	collgridcell_t**	m_gridMap;

	CEqPhysics*			m_physics;

	int					m_gridSize;
	float				m_invGridSize;

	int					m_gridWide;
	int					m_gridTall;
};

#endif // EQCOLLISION_OBJECTGRID_H