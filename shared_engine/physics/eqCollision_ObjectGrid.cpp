//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics object 2D spatial grid
//////////////////////////////////////////////////////////////////////////////////

#include "eqCollision_ObjectGrid.h"
#include "eqCollision_Object.h"
#include "eqBulletIndexedMesh.h"

#include "core/DebugInterface.h"
#include "core/ConVar.h"

#include "render/IDebugOverlay.h"

CEqCollisionBroadphaseGrid::CEqCollisionBroadphaseGrid()
{
	m_gridMap = NULL;

	m_gridWide = 0;
	m_gridTall = 0;
	m_gridSize = 0;
}

CEqCollisionBroadphaseGrid::~CEqCollisionBroadphaseGrid()
{
	Destroy();
}

void CEqCollisionBroadphaseGrid::Init(CEqPhysics* physics, int gridsize, const Vector3D& worldmins, const Vector3D& worldmaxs)
{
	m_physics = physics;

	m_gridSize = gridsize;

	Vector3D size = worldmaxs-worldmins;

	m_invGridSize = (1.0f / (float)m_gridSize);

	// compute grid size
	m_gridWide = ceilf(size.x * m_invGridSize);
	m_gridTall = ceilf(size.z * m_invGridSize);

	m_gridMap = PPNew collgridcell_t*[m_gridWide*m_gridTall];

	for(int y = 0; y < m_gridWide; y++)
	{
		for(int x = 0; x < m_gridTall; x++)
		{
			int idx = y*m_gridWide+x;

			m_gridMap[idx] = NULL;
		}
	}

	float grid_size = (float(sizeof(collgridcell_t*)*m_gridWide*m_gridTall) / 1024.0f) / 1024.0f;

	DevMsg(DEVMSG_CORE, "CELL PTR MAP INIT = %d x %d (cellsize = %d). SIZE=%.2f MB\n", m_gridWide, m_gridTall, m_gridSize, grid_size);
}

void CEqCollisionBroadphaseGrid::Destroy()
{
	if(m_gridMap)
	{
		for(int y = 0; y < m_gridWide; y++)
		{
			for(int x = 0; x < m_gridTall; x++)
			{
				int idx = y*m_gridWide+x;

				collgridcell_t* cell = m_gridMap[idx];

				if(cell)
				{
					for(int i = 0; i < cell->m_dynamicObjs.numElem(); i++)
						cell->m_dynamicObjs[i]->SetCell(NULL);

					for(int i = 0; i < m_gridMap[idx]->m_gridObjects.numElem(); i++)
						cell->m_gridObjects[i]->SetCell(NULL);
				}

				delete cell;
			}
		}
		delete [] m_gridMap;
	}

	m_gridMap = NULL;

	m_gridWide = 0;
	m_gridTall = 0;
	m_gridSize = 0;
}

bool CEqCollisionBroadphaseGrid::IsInit() const
{
	return m_gridMap != NULL;
}

bool CEqCollisionBroadphaseGrid::GetPointAt(const Vector3D& origin, int& x, int& y) const
{
	float halfGridNeg = m_gridSize*-0.5f;

	Vector2D center(m_gridWide*halfGridNeg, m_gridTall*halfGridNeg);
	Vector2D xz_pos((origin.xz() - center) * m_invGridSize);

	x = floor(xz_pos.x);
	y = floor(xz_pos.y);

	if(x < 0 || x >= m_gridWide)
		return false;

	if(y < 0 || y >= m_gridTall)
		return false;

	return true;
}

collgridcell_t*	CEqCollisionBroadphaseGrid::GetPreallocatedCellAtPos(const Vector3D& origin)
{
	float halfGridNeg = m_gridSize*-0.5f;

	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	const Vector2D center(gridWide*halfGridNeg, gridTall*halfGridNeg);
	const IVector2D xz_pos((origin.xz() - center) * m_invGridSize);

	if(xz_pos.x < 0 || xz_pos.x >= gridWide)
		return NULL;

	if(xz_pos.y < 0 || xz_pos.y >= gridTall)
		return NULL;

	return GetAllocCellAt( xz_pos.x, xz_pos.y );
}

collgridcell_t* CEqCollisionBroadphaseGrid::GetCellAtPos(const Vector3D& origin) const
{
	const float halfGridNeg = m_gridSize*-0.5f;

	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	const Vector2D center(gridWide*halfGridNeg, gridTall*halfGridNeg);
	const IVector2D xz_pos((origin.xz() - center) * m_invGridSize);

	if(xz_pos.x < 0 || xz_pos.x >= gridWide)
		return NULL;

	if(xz_pos.y < 0 || xz_pos.y >= gridTall)
		return NULL;

	return m_gridMap[xz_pos.y*gridWide + xz_pos.x];
}

collgridcell_t* CEqCollisionBroadphaseGrid::GetCellAt(int x, int y) const
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return NULL;

	if(y < 0 || y >= gridTall)
		return NULL;

	return m_gridMap[y*gridWide + x];
}

bool CEqCollisionBroadphaseGrid::GetCellBounds(int x, int y, Vector3D& mins, Vector3D& maxs) const
{
	collgridcell_t* cell = GetCellAt(x,y);

	if(!cell)
		return false;

	const float cellHeight = cell->cellBoundUsed;
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;
	const int gridSize = m_gridSize;

	Vector3D center(gridWide*gridSize*-0.5f, 0, gridTall*gridSize*-0.5f);

	mins = Vector3D(x*gridSize, -cellHeight, y*gridSize) + center;
	maxs = Vector3D((x+1)*gridSize, cellHeight, (y+1)*gridSize) + center;

	return true;
}

collgridcell_t*	CEqCollisionBroadphaseGrid::GetAllocCellAt(int x, int y)
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return nullptr;

	if(y < 0 || y >= gridTall)
		return nullptr;

	if(!m_gridMap)
		return nullptr;

	int cellIdx = y * gridWide + x;

	if(!m_gridMap[cellIdx])
	{
		collgridcell_t* newCell = PPNew collgridcell_t();

		newCell->x = x;
		newCell->y = y;
		newCell->cellBoundUsed = 0;

		m_gridMap[cellIdx] = newCell;
	}

	return m_gridMap[cellIdx];
}

void CEqCollisionBroadphaseGrid::FreeCellAt( int x, int y )
{
	const int gridWide = m_gridWide;
	const int gridTall = m_gridTall;

	if(x < 0 || x >= gridWide)
		return;

	if(y < 0 || y >= gridTall)
		return;

	if(m_gridMap == NULL)
		return;

	int cellIdx = y * gridWide + x;

	collgridcell_t* cell = m_gridMap[cellIdx];

	if(cell)
	{
		Array<CEqCollisionObject*>& dynamicObjs = cell->m_dynamicObjs;
		int count = dynamicObjs.numElem();

		for(int i = 0; i < count; i++)
		{
			CEqCollisionObject* pObj = dynamicObjs[i];
			pObj->SetCell(NULL);
		}

		if(cell->m_gridObjects.numElem())
			MsgWarning( "Cell deallocated, but in use (%d)\n", cell->m_gridObjects.numElem());

		delete cell;

		m_gridMap[cellIdx] = nullptr;
	}
}

void CEqCollisionBroadphaseGrid::FindBoxRange(const BoundingBox& bbox, IVector2D& cr_min, IVector2D& cr_max, float extTolerance) const
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

		cr_min.x = (dx1 < EXT_TOLERANCE) ? (floor(xz_pos1.x)-1) : floor(xz_pos1.x);
		cr_min.y = (dy1 < EXT_TOLERANCE) ? (floor(xz_pos1.y)-1) : floor(xz_pos1.y);

		cr_max.x = (dx2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.x)+1) : floor(xz_pos2.x);
		cr_max.y = (dy2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.y)+1) : floor(xz_pos2.y);
	}
	else
	{
		cr_min.x = floor(xz_pos1.x);
		cr_min.y = floor(xz_pos1.y);
		cr_max.x = floor(xz_pos2.x);
		cr_max.y = floor(xz_pos2.y);
	}
}

void CEqCollisionBroadphaseGrid::AddStaticObjectToGrid( CEqCollisionObject* collisionObject )
{
	if(m_gridMap == NULL)
		return;

	if(collisionObject == NULL)
		return;

	int cx,cy;
	if(!GetPointAt(collisionObject->GetPosition(), cx, cy))
		return;

	// we're adding object to all nodes occupied by bbox
	collgridcell_t* cell = GetAllocCellAt( cx,cy );

	if(cell)
	{
		// set this cell.
		collisionObject->SetCell( cell );

		// now check in a bounding box extents
		// and add this object tho another cells of grid
		collisionObject->UpdateBoundingBoxTransform();

		BoundingBox& bbox = collisionObject->m_aabb_transformed;
		float boxSizeY = bbox.maxPoint.y;

		IVector2D crMin, crMax;
		FindBoxRange(bbox, crMin, crMax, 0.0f );

		// in this range do...
		for(int y = crMin.y; y < crMax.y+1; y++)
		{
			for(int x = crMin.x; x < crMax.x+1; x++)
			{
				collgridcell_t* ncell = GetAllocCellAt( x, y );

				if(ncell)
				{
					ncell->m_gridObjects.append( collisionObject );

					// change height bounds
					if(boxSizeY > ncell->cellBoundUsed)
						ncell->cellBoundUsed = boxSizeY;
				}
			}
		}

		collisionObject->m_cellRange.x = crMin.x;
		collisionObject->m_cellRange.y = crMin.y;
		collisionObject->m_cellRange.z = crMax.x;
		collisionObject->m_cellRange.w = crMax.y;
	}
	else
	{
		MsgError("NO CELL FOUND! CAN'T PLANT\n");
	}
}

void CEqCollisionBroadphaseGrid::RemoveStaticObjectFromGrid( CEqCollisionObject* collisionObject )
{
	if(m_gridMap == NULL)
		return;

	if(collisionObject == NULL)
		return;

	// now check in a bounding box extents
	// and add this object tho another cells of grid

	int cr_x1 = collisionObject->m_cellRange.x;
	int cr_y1 = collisionObject->m_cellRange.y;
	int cr_x2 = collisionObject->m_cellRange.z;
	int cr_y2 = collisionObject->m_cellRange.w;

	collisionObject->SetCell(nullptr);

	//Msg("Removing OBJECT %p [%d %d] [%d %d]\n", collisionObject, cr_x1, cr_y1, cr_x2, cr_y2);

	// in this range do...
	for(int y = cr_y1; y < cr_y2+1; y++)
	{
		for(int x = cr_x1; x < cr_x2+1; x++)
		{
			collgridcell_t* ncell = GetCellAt( x, y );

			if(ncell)
			{
				if(!ncell->m_gridObjects.fastRemove( collisionObject ))
					MsgError("Not found in [%d %d]\n", x, y);

				// remove cell if no users
				if( ncell->m_gridObjects.numElem() <= 0 )
					FreeCellAt(x,y);
			}
		}
	}
}

ConVar ph_grid_debug_display_x("ph_grid_debug_display_x", "-1");
ConVar ph_grid_debug_display_y("ph_grid_debug_display_y", "-1");

void CEqCollisionBroadphaseGrid::DebugRender()
{
	Vector3D mins, maxs;

	for(int y = 0; y < m_gridWide; y++)
	{
		for(int x = 0; x < m_gridTall; x++)
		{
			if(!GetCellBounds(x,y, mins, maxs))
				continue;

			debugoverlay->Box3D(mins, maxs, ColorRGBA(1,0,1,0.25f));

			if (ph_grid_debug_display_x.GetInt() == x && ph_grid_debug_display_y.GetInt() == y)
			{
				collgridcell_t* cell = GetCellAt(x, y);

				if (cell)
				{
					for (int i = 0; i < cell->m_dynamicObjs.numElem(); i++)
					{
						CEqCollisionObject* obj = cell->m_dynamicObjs[i];

						ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 1.0f);

						debugoverlay->Box3D(obj->m_aabb_transformed.minPoint, obj->m_aabb_transformed.maxPoint, bodyCol, 0.0f);
					}
				}
			} // debug display
		}
	}


}
