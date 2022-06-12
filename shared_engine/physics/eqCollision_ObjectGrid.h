//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object grid
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqCollisionObject;
class CEqPhysics;

struct collgridcell_t
{
	short	x,y;
	float	cellBoundUsed;	// unsigned z of usage by static objects

	Array<CEqCollisionObject*>		m_gridObjects{ PP_SL };
	Array<CEqCollisionObject*>		m_dynamicObjs{ PP_SL };
};

class CEqCollisionBroadphaseGrid
{
public:
	CEqCollisionBroadphaseGrid(CEqPhysics* physics, int gridsize, const Vector3D& worldmins, const Vector3D& worldmaxs);
	~CEqCollisionBroadphaseGrid();

	collgridcell_t*		GetPreallocatedCellAtPos(const Vector3D& origin);

	collgridcell_t*		GetCellAtPos(const Vector3D& origin) const;
	collgridcell_t*		GetCellAt(int x, int y) const;

	bool				GetPointAt(const Vector3D& origin, int& x, int& y) const;

	void				AddStaticObjectToGrid( CEqCollisionObject* m_collisionObject );
	void				RemoveStaticObjectFromGrid( CEqCollisionObject* m_collisionObject );

	bool				GetCellBounds(int x, int y, Vector3D& mins, Vector3D& maxs) const;

	void				FindBoxRange(const BoundingBox& bbox, IVector2D& cr_min, IVector2D& cr_max, float extTolerance) const;

	void				DebugRender();

	// TODO: query line, box, sphere

protected:

	collgridcell_t*		GetAllocCellAt(int x, int y);
	void				FreeCellAt( int x, int y );

	collgridcell_t**	m_gridMap{ nullptr };

	CEqPhysics*			m_physics{ nullptr };

	int					m_gridSize;
	float				m_invGridSize;

	int					m_gridWide;
	int					m_gridTall;
};
