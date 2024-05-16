//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object grid
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqCollisionObject;
class CEqPhysics;

struct eqPhysGridCell
{
	Array<CEqCollisionObject*>	gridObjects{ PP_SL };
	Array<CEqCollisionObject*>	dynamicObjs{ PP_SL };
	float						cellBoundUsed = 0.0f;	// unsigned z of usage by static objects
};

class CEqCollisionBroadphaseGrid
{
public:
	CEqCollisionBroadphaseGrid(CEqPhysics* physics, int gridsize, const Vector3D& worldmins, const Vector3D& worldmaxs);
	~CEqCollisionBroadphaseGrid();

	eqPhysGridCell*		GetPreallocatedCellAtPos(const Vector3D& origin);

	eqPhysGridCell*		GetCellAtPos(const Vector3D& origin) const;
	eqPhysGridCell*		GetCellAt(int x, int y) const;

	bool				GetPointAt(const Vector3D& origin, IVector2D& xzCell) const;
	bool				GetPointAt(const Vector3D& origin, Vector2D& xzCell) const;

	void				AddStaticObjectToGrid( CEqCollisionObject* collisionObject );
	void				RemoveStaticObjectFromGrid( CEqCollisionObject* collisionObject );

	void				GetCellBoundsXZ(int x, int y, Vector2D& mins, Vector2D& maxs) const;
	bool				GetCellBounds(int x, int y, Vector3D& mins, Vector3D& maxs) const;

	void				FindBoxRange(const BoundingBox& bbox, IAARectangle& gridRange, float extTolerance) const;

	void				DebugRender();

	// TODO: query line, box, sphere

protected:

	eqPhysGridCell*		GetAllocCellAt(int x, int y);
	void				FreeCellAt( int x, int y );

	Map<int, eqPhysGridCell> m_gridMap{ PP_SL };
	CEqPhysics*			m_physics{ nullptr };

	int					m_gridSize;
	float				m_invGridSize;

	int					m_gridWide;
	int					m_gridTall;
};
