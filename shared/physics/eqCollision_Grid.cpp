//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object 2D spatial grid
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "eqCollision_Grid.h"
#include "eqCollision_Object.h"

#include "render/IDebugOverlay.h"

DECLARE_CVAR(ph_debugGridX, "-1", nullptr, 0);
DECLARE_CVAR(ph_debugGridY, "-1", nullptr, 0);

static constexpr float BROADPHASE_BBOX_EXPAND = 16.0f;

static void ReleaseGridCellObjs(eqPhysGridCell& cell)
{
	CEqCollisionObject* collObj = cell.dynamicObjList.getFirst();
	while (collObj)
	{
		CEqCollisionObject* unlinkObj = collObj;

		collObj->SetCell(-1);
		collObj = collObj->next;

		unlinkObj->next = nullptr;
		unlinkObj->prev = nullptr;
	}
}

CEqCollisionBroadphaseGrid::CEqCollisionBroadphaseGrid(int cellSize, const BoundingBox& worldBox)
{
	m_worldBox = worldBox;
	m_worldBox.Expand(BROADPHASE_BBOX_EXPAND);
	m_cellSize = cellSize;
	m_invCellSize = 1.0f / cellSize;

	const Vector3D worldSize = m_worldBox.GetSize();
	m_gridWide = ceil(worldSize.x * m_invCellSize);
	m_gridTall = ceil(worldSize.z * m_invCellSize);

	m_gridMap.assureSizeEmplace(m_gridWide * m_gridTall, nullptr);
}

CEqCollisionBroadphaseGrid::~CEqCollisionBroadphaseGrid()
{
	for (eqPhysGridCell* cell : m_gridMap)
	{
		if (!cell)
			continue;
		ReleaseGridCellObjs(*cell);
		for (CEqCollisionObject* collObj : cell->gridObjects)
			collObj->SetCell(-1);
	}
	m_gridPool.clear();
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
	const Vector2D gridPos = m_worldBox.minPoint.xz();
	const Vector2D cellPos((origin.xz() - gridPos) * m_invCellSize);

	xzCell = cellPos;
	if (cellPos.x < 0 || cellPos.x >= m_gridWide || cellPos.y < 0 || cellPos.y >= m_gridTall)
		return false;

	return true;
}

eqPhysGridCell*	CEqCollisionBroadphaseGrid::GetAllocCellAtPos(const Vector3D& origin)
{
	IVector2D cellPos;
	if (!GetPointAt(origin, cellPos))
		return nullptr;

	return GetAllocCellAt(cellPos);
}

eqPhysGridCell* CEqCollisionBroadphaseGrid::GetAllocCellAt(const IVector2D& xzCell)
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if (xzCell.x < 0 || xzCell.x >= gridWide || xzCell.y < 0 || xzCell.y >= gridTall)
		return nullptr;

	const int cellIdx = xzCell.y * gridWide + xzCell.x;
	eqPhysGridCell* cell = m_gridMap[cellIdx];
	if (!cell)
	{
		cell = new (m_gridPool.allocate()) eqPhysGridCell();
		m_gridMap[cellIdx] = cell;
	}
	return cell;
}

eqPhysGridCell* CEqCollisionBroadphaseGrid::GetCellAtPos(const Vector3D& origin) const
{
	IVector2D cellPos;
	if (!GetPointAt(origin, cellPos))
		return nullptr;

	return GetCellAt(cellPos);
}

eqPhysGridCell* CEqCollisionBroadphaseGrid::GetCellAt(const IVector2D& xzCell) const
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if (xzCell.x < 0 || xzCell.x >= gridWide || xzCell.y < 0 || xzCell.y >= gridTall)
		return nullptr;

	const int cellIdx = xzCell.y * m_gridWide + xzCell.x;
	return m_gridMap[cellIdx];
}

void CEqCollisionBroadphaseGrid::FreeCellAt(const IVector2D& xzCell)
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if (xzCell.x < 0 || xzCell.x >= gridWide || xzCell.y < 0 || xzCell.y >= gridTall)
		return;

	const int cellIdx = xzCell.y * gridWide + xzCell.x;
	eqPhysGridCell* cell = m_gridMap[cellIdx];
	if (!cell)
		return;

	ReleaseGridCellObjs(*cell);
	ASSERT_MSG(cell->gridObjects.isEmpty(), "Cell deallocated, but in use (%d)\n", cell->gridObjects.numElem());

	m_gridMap[cellIdx] = nullptr;
	cell->~eqPhysGridCell();
	m_gridPool.deallocate(cell);
}

void CEqCollisionBroadphaseGrid::GetCellBoundsXZ(const IVector2D& xzCell, Vector2D& mins, Vector2D& maxs) const
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;
	const int gridSize = m_cellSize;
	const Vector2D gridPos = m_worldBox.minPoint.xz();

	mins = Vector2D(xzCell.x * gridSize, xzCell.y * gridSize) + gridPos;
	maxs = Vector2D((xzCell.x + 1) * gridSize, (xzCell.y + 1) * gridSize) + gridPos;
}

bool CEqCollisionBroadphaseGrid::GetCellBounds(const IVector2D& xzCell, Vector3D& mins, Vector3D& maxs) const
{
	eqPhysGridCell* cell = GetCellAt(xzCell);
	if(!cell)
		return false;

	Vector2D min2D, max2D;
	GetCellBoundsXZ(xzCell, min2D, max2D);

	const float cellHeight = cell->cellBoundUsed;
	mins = Vector3D(min2D.x, -cellHeight, min2D.y);
	maxs = Vector3D(max2D.x, cellHeight, max2D.y);

	return true;
}

IAARectangle CEqCollisionBroadphaseGrid::FindBoxRange(const BoundingBox& bbox, float extTolerance) const
{
	const float invGridSize = m_invCellSize;
	const Vector2D gridPos = m_worldBox.minPoint.xz();

	const Vector2D xz_pos1((bbox.minPoint.xz() - gridPos) * invGridSize);
	const Vector2D xz_pos2((bbox.maxPoint.xz() - gridPos) * invGridSize);

	IAARectangle gridRange;
	if(extTolerance > F_EPS )
	{
		const float EXT_TOLERANCE = extTolerance;	// the percentage of cell size
		const float EXT_TOLERANCE_REC = 1.0f - EXT_TOLERANCE;

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
	return gridRange;
}

void CEqCollisionBroadphaseGrid::AddStaticObjectToGrid( CEqCollisionObject* collisionObject )
{
	if(collisionObject == nullptr)
		return;

	IVector2D xzCell;
	if (!GetPointAt(collisionObject->GetPosition(), xzCell))
	{
		ASSERT_FAIL("Object outside cell world");
		return;
	}

	collisionObject->SetCell(xzCell);
	collisionObject->UpdateBoundingBoxTransform();

	const BoundingBox bbox = collisionObject->m_aabb_transformed;
	const IAARectangle gridRange = FindBoxRange(bbox, 0.0f );

	ASSERT_MSG(gridRange.leftTop.x >= 0 && gridRange.leftTop.y >= 0 && gridRange.rightBottom.x <= m_gridWide && gridRange.rightBottom.y <= m_gridTall,
		"FindBoxRange: outside of grid bounds, box is [%.2f %.2f %.2f] [%.2f %.2f %.2f]",
		bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z,
		bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z);

	for(int y = gridRange.leftTop.y; y <= gridRange.rightBottom.y; y++)
	{
		for(int x = gridRange.leftTop.x; x <= gridRange.rightBottom.x; x++)
		{
			eqPhysGridCell* ncell = GetAllocCellAt( IVector2D(x, y) );
			if (!ncell)
				continue;

			ncell->gridObjects.append( collisionObject );
			ncell->cellBoundUsed = max(ncell->cellBoundUsed, bbox.maxPoint.y);
		}
	}

	collisionObject->m_cellRange = gridRange;
}

void CEqCollisionBroadphaseGrid::RemoveStaticObjectFromGrid( CEqCollisionObject* collisionObject )
{
	if(collisionObject == nullptr)
		return;

	collisionObject->SetCell(-1);

	const IAARectangle gridRange = collisionObject->m_cellRange;
	for(int y = gridRange.leftTop.y; y <= gridRange.rightBottom.y; y++)
	{
		for(int x = gridRange.leftTop.x; x <= gridRange.rightBottom.x; x++)
		{
			eqPhysGridCell* ncell = GetCellAt(IVector2D(x, y));
			if (!ncell)
				continue;

			ncell->gridObjects.fastRemove(collisionObject);
			if( ncell->gridObjects.isEmpty() )
				FreeCellAt(IVector2D(x, y));
		}
	}
}

void CEqCollisionBroadphaseGrid::DebugRender()
{
#ifdef ENABLE_DEBUG_DRAWING
	for (int i = 0; i < m_gridMap.numElem(); ++i)
	{
		const IVector2D cellPos(i % m_gridWide, i / m_gridWide);
		Vector3D mins, maxs;
		if (!GetCellBounds(cellPos, mins, maxs))
			continue;

		DbgBox().Mins(mins).Maxs(maxs).Color(ColorRGBA(1, 0, 1, 0.25f));

		if (ph_debugGridX.GetInt() != cellPos.x || ph_debugGridY.GetInt() != cellPos.y)
			continue;

		eqPhysGridCell* cell = GetCellAt(cellPos);
		if (!cell)
			continue;

		CEqCollisionObject* collObj = cell->dynamicObjList.getFirst();
		while (collObj)
		{
			const ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 1.0f);
			DbgBox().Box(collObj->m_aabb_transformed).Color(bodyCol);

			collObj = collObj->next;
		}
	}
#endif // ENABLE_DEBUG_DRAWING
}
