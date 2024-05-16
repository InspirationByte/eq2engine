//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object 2D spatial grid
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "eqCollision_ObjectGrid.h"
#include "eqCollision_Object.h"

#include "render/IDebugOverlay.h"

DECLARE_CVAR(ph_debugGridX, "-1", nullptr, 0);
DECLARE_CVAR(ph_debugGridY, "-1", nullptr, 0);

CEqCollisionBroadphaseGrid::CEqCollisionBroadphaseGrid(CEqPhysics* physics, int gridsize, const Vector3D& worldmins, const Vector3D& worldmaxs)
{
	m_physics = physics;
	m_gridSize = gridsize;

	Vector3D size = worldmaxs - worldmins;

	m_invGridSize = (1.0f / (float)m_gridSize);

	// compute grid size
	m_gridWide = ceilf(size.x * m_invGridSize);
	m_gridTall = ceilf(size.z * m_invGridSize);
}

CEqCollisionBroadphaseGrid::~CEqCollisionBroadphaseGrid()
{
	for (int y = 0; y < m_gridWide; y++)
	{
		for (int x = 0; x < m_gridTall; x++)
		{
			const int idx = y * m_gridWide + x;
			auto it = m_gridMap.find(idx);
			if(it.atEnd())
				continue;

			eqPhysGridCell& cell = *it;

			for (CEqCollisionObject* collObj: cell.dynamicObjs)
				collObj->SetCell(nullptr);

			for (CEqCollisionObject* collObj: cell.gridObjects)
				collObj->SetCell(nullptr);
		}
	}
	m_gridMap.clear(true);
}

bool CEqCollisionBroadphaseGrid::GetPointAt(const Vector3D& origin, IVector2D& xzCell) const
{
	Vector2D cell;
	const bool ok = GetPointAt(origin, cell);

	xzCell.x = floor(cell.x);
	xzCell.y = floor(cell.y);

	return ok;
}

bool CEqCollisionBroadphaseGrid::GetPointAt(const Vector3D& origin, Vector2D& xzCell) const
{
	const float halfGridNeg = m_gridSize * -0.5f;

	Vector2D center(m_gridWide * halfGridNeg, m_gridTall * halfGridNeg);
	Vector2D xz_pos((origin.xz() - center) * m_invGridSize);

	xzCell = xz_pos;

	if (xz_pos.x < 0 || xz_pos.x >= m_gridWide)
		return false;

	if (xz_pos.y < 0 || xz_pos.y >= m_gridTall)
		return false;

	return true;
}

eqPhysGridCell*	CEqCollisionBroadphaseGrid::GetPreallocatedCellAtPos(const Vector3D& origin)
{
	float halfGridNeg = m_gridSize*-0.5f;

	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	const Vector2D center(gridWide*halfGridNeg, gridTall*halfGridNeg);
	const IVector2D xz_pos((origin.xz() - center) * m_invGridSize);

	if(xz_pos.x < 0 || xz_pos.x >= gridWide)
		return nullptr;

	if(xz_pos.y < 0 || xz_pos.y >= gridTall)
		return nullptr;

	return GetAllocCellAt( xz_pos.x, xz_pos.y );
}

eqPhysGridCell* CEqCollisionBroadphaseGrid::GetCellAtPos(const Vector3D& origin) const
{
	const float halfGridNeg = m_gridSize*-0.5f;

	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	const Vector2D center(gridWide*halfGridNeg, gridTall*halfGridNeg);
	const IVector2D xz_pos((origin.xz() - center) * m_invGridSize);

	if(xz_pos.x < 0 || xz_pos.x >= gridWide)
		return nullptr;

	if(xz_pos.y < 0 || xz_pos.y >= gridTall)
		return nullptr;

	auto it = m_gridMap.find(xz_pos.y*gridWide + xz_pos.x);

	return it.atEnd() ? static_cast<eqPhysGridCell*>(nullptr) : &(*it);
}

eqPhysGridCell* CEqCollisionBroadphaseGrid::GetCellAt(int x, int y) const
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return nullptr;

	if(y < 0 || y >= gridTall)
		return nullptr;

	auto it = m_gridMap.find(y*gridWide + x);

	return it.atEnd() ? static_cast<eqPhysGridCell*>(nullptr) : &(*it);
}

void CEqCollisionBroadphaseGrid::GetCellBoundsXZ(int x, int y, Vector2D& mins, Vector2D& maxs) const
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;
	const int gridSize = m_gridSize;

	Vector2D center(gridWide * gridSize * -0.5f, gridTall * gridSize * -0.5f);

	mins = Vector2D(x * gridSize, y * gridSize) + center;
	maxs = Vector2D((x + 1) * gridSize, (y + 1) * gridSize) + center;
}

bool CEqCollisionBroadphaseGrid::GetCellBounds(int x, int y, Vector3D& mins, Vector3D& maxs) const
{
	eqPhysGridCell* cell = GetCellAt(x,y);

	if(!cell)
		return false;

	Vector2D min2D, max2D;
	GetCellBoundsXZ(x, y, min2D, max2D);

	const float cellHeight = cell->cellBoundUsed;
	mins = Vector3D(min2D.x, -cellHeight, min2D.y);
	maxs = Vector3D(max2D.x, cellHeight, max2D.y);

	return true;
}

eqPhysGridCell*	CEqCollisionBroadphaseGrid::GetAllocCellAt(int x, int y)
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return nullptr;

	if(y < 0 || y >= gridTall)
		return nullptr;

	const int cellIdx = y * gridWide + x;
	auto it = m_gridMap.find(cellIdx);

	if(it.atEnd())
		it = m_gridMap.insert(cellIdx);

	return &(*it);
}

void CEqCollisionBroadphaseGrid::FreeCellAt( int x, int y )
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return;

	if(y < 0 || y >= gridTall)
		return;

	const int cellIdx = y * gridWide + x;
	auto it = m_gridMap.find(cellIdx);

	if(it.atEnd())
		return;

	eqPhysGridCell& cell = *it;

	Array<CEqCollisionObject*>& dynamicObjs = cell.dynamicObjs;
	int count = dynamicObjs.numElem();

	for(int i = 0; i < count; i++)
	{
		CEqCollisionObject* pObj = dynamicObjs[i];
		pObj->SetCell(nullptr);
	}

	if(cell.gridObjects.numElem())
		MsgWarning( "Cell deallocated, but in use (%d)\n", cell.gridObjects.numElem());

	m_gridMap.remove(it);
}

void CEqCollisionBroadphaseGrid::FindBoxRange(const BoundingBox& bbox, IAARectangle& gridRange, float extTolerance) const
{
	const float invGridSize = m_invGridSize;
	const float halfGridNeg = m_gridSize*-0.5f;

	const Vector2D center(m_gridWide*halfGridNeg, m_gridTall*halfGridNeg);

	const Vector2D xz_pos1((bbox.minPoint.xz() - center) * invGridSize);
	const Vector2D xz_pos2((bbox.maxPoint.xz() - center) * invGridSize);

	if(extTolerance > 0 )
	{
		const float EXT_TOLERANCE		= extTolerance;	// the percentage of cell size
		const float EXT_TOLERANCE_REC	= 1.0f - EXT_TOLERANCE;

		const float dx1 = xz_pos1.x - floor(xz_pos1.x);
		const float dy1 = xz_pos1.y - floor(xz_pos1.y);

		const float dx2 = xz_pos2.x - floor(xz_pos2.x);
		const float dy2 = xz_pos2.y - floor(xz_pos2.y);

		gridRange.leftTop.x = (dx1 < EXT_TOLERANCE) ? (floor(xz_pos1.x)-1) : floor(xz_pos1.x);
		gridRange.leftTop.y = (dy1 < EXT_TOLERANCE) ? (floor(xz_pos1.y)-1) : floor(xz_pos1.y);

		gridRange.rightBottom.x = (dx2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.x)+1) : floor(xz_pos2.x);
		gridRange.rightBottom.y = (dy2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.y)+1) : floor(xz_pos2.y);
	}
	else
	{
		gridRange.leftTop.x = floor(xz_pos1.x);
		gridRange.leftTop.y = floor(xz_pos1.y);
		gridRange.rightBottom.x = floor(xz_pos2.x);
		gridRange.rightBottom.y = floor(xz_pos2.y);
	}
}

void CEqCollisionBroadphaseGrid::AddStaticObjectToGrid( CEqCollisionObject* collisionObject )
{
	if(collisionObject == nullptr)
		return;

	IVector2D cxz;
	if(!GetPointAt(collisionObject->GetPosition(), cxz))
		return;

	// we're adding object to all nodes occupied by bbox
	eqPhysGridCell* cell = GetAllocCellAt( cxz.x, cxz.y );

	if(cell)
	{
		// set this cell.
		collisionObject->SetCell( cell );

		// now check in a bounding box extents
		// and add this object tho another cells of grid
		collisionObject->UpdateBoundingBoxTransform();

		const BoundingBox bbox = collisionObject->m_aabb_transformed;
		const float boxSizeY = bbox.maxPoint.y;

		IAARectangle gridRange;
		FindBoxRange(bbox, gridRange, 0.0f );

		ASSERT_MSG(gridRange.leftTop.x >=0 && gridRange.leftTop.y >= 0 && gridRange.rightBottom.x < m_gridWide && gridRange.rightBottom.y < m_gridTall,
			"FindBoxRange: outside of grid bounds, box is [%.2f %.2f %.2f] [%.2f %.2f %.2f]",
			bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z,
			bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z);

		// in this range do...
		for(int y = gridRange.leftTop.y; y <= gridRange.rightBottom.y; y++)
		{
			for(int x = gridRange.leftTop.x; x <= gridRange.rightBottom.x; x++)
			{
				eqPhysGridCell* ncell = GetAllocCellAt( x, y );

				if (!ncell)
					continue;

				ncell->gridObjects.append( collisionObject );

				// change height bounds
				if(boxSizeY > ncell->cellBoundUsed)
					ncell->cellBoundUsed = boxSizeY;
			}
		}

		collisionObject->m_cellRange = gridRange;
	}
	else
	{
		MsgError("NO CELL FOUND! CAN'T PLANT\n");
	}
}

void CEqCollisionBroadphaseGrid::RemoveStaticObjectFromGrid( CEqCollisionObject* collisionObject )
{
	if(collisionObject == nullptr)
		return;

	// now check in a bounding box extents
	// and add this object tho another cells of grid

	IAARectangle gridRange = collisionObject->m_cellRange;

	collisionObject->SetCell(nullptr);

	//Msg("Removing OBJECT %p [%d %d] [%d %d]\n", collisionObject, gridRange.leftTop.x, gridRange.leftTop.y, gridRange.rightBottom.x, gridRange.rightBottom.y);

	// in this range do...
	for(int y = gridRange.leftTop.y; y <= gridRange.rightBottom.y; y++)
	{
		for(int x = gridRange.leftTop.x; x <= gridRange.rightBottom.x; x++)
		{
			eqPhysGridCell* ncell = GetCellAt( x, y );

			if (!ncell)
				continue;

			if(!ncell->gridObjects.fastRemove( collisionObject ))
				MsgError("Not found in [%d %d]\n", x, y);

			// remove cell if no users
			if( ncell->gridObjects.numElem() <= 0 )
				FreeCellAt(x,y);
		}
	}
}

void CEqCollisionBroadphaseGrid::DebugRender()
{
#ifdef ENABLE_DEBUG_DRAWING
	for(int y = 0; y < m_gridWide; y++)
	{
		for(int x = 0; x < m_gridTall; x++)
		{
			Vector3D mins, maxs;
			if(!GetCellBounds(x,y, mins, maxs))
				continue;

			debugoverlay->Box3D(mins, maxs, ColorRGBA(1,0,1,0.25f));

			if (ph_debugGridX.GetInt() != x || ph_debugGridY.GetInt() != y)
				continue;

			eqPhysGridCell* cell = GetCellAt(x, y);
			if (!cell)
				continue;

			for (int i = 0; i < cell->dynamicObjs.numElem(); i++)
			{
				CEqCollisionObject* obj = cell->dynamicObjs[i];

				ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 1.0f);

				debugoverlay->Box3D(obj->m_aabb_transformed.minPoint, obj->m_aabb_transformed.maxPoint, bodyCol, 0.0f);
			}
		}
	}
#endif // ENABLE_DEBUG_DRAWING
}
