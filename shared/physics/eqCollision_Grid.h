//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object grid
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqCollisionObject;
class CEqPhysicsWorld;

struct eqPhysGridCell
{
	using StaticCollObjList = Array<CEqCollisionObject*>;
	using DynCollObjList = LinkedListImpl<CEqCollisionObject>;

	StaticCollObjList	gridObjects{ PP_SL };
	DynCollObjList		dynamicObjList;
	float				cellBoundUsed = 0.0f;	// unsigned z of usage by static objects
};

class CEqCollisionBroadphaseGrid
{
public:
	CEqCollisionBroadphaseGrid(int cellSize, const BoundingBox& worldBox);
	~CEqCollisionBroadphaseGrid();

	eqPhysGridCell*		GetAllocCellAtPos(const Vector3D& origin);
	eqPhysGridCell*		GetAllocCellAt(const IVector2D& xzCell);

	eqPhysGridCell*		GetCellAtPos(const Vector3D& origin) const;
	eqPhysGridCell*		GetCellAt(const IVector2D& xzCell) const;

	bool				GetPointAt(const Vector3D& origin, IVector2D& xzCell) const;
	bool				GetPointAt(const Vector3D& origin, Vector2D& xzCell) const;

	void				AddStaticObjectToGrid( CEqCollisionObject* collisionObject );
	void				RemoveStaticObjectFromGrid( CEqCollisionObject* collisionObject );

	void				GetCellBoundsXZ(const IVector2D& xzCell, Vector2D& mins, Vector2D& maxs) const;
	bool				GetCellBounds(const IVector2D& xzCell, Vector3D& mins, Vector3D& maxs) const;

	IAARectangle		FindBoxRange(const BoundingBox& bbox, float extTolerance) const;

	void				DebugRender();

	// TODO: query line, box, sphere

protected:

	void				FreeCellAt(const IVector2D& xzCell);

	MemoryPool<eqPhysGridCell>	m_gridPool{ PP_SL };
	Array<eqPhysGridCell*>		m_gridMap{ PP_SL };		// TODO: LUT

	BoundingBox			m_worldBox;
	float				m_cellSize;
	float				m_invCellSize;

	int					m_gridWide;
	int					m_gridTall;
};
