//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Game object spatial list
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "spatial_list.h"

constexpr const int		s_spatialObjectGridCell = 16;
constexpr const float	s_spatialOneByObjectGridCell = 1.0f / static_cast<float>(s_spatialObjectGridCell);
constexpr const int		s_spatialObjectGridSize = 2048 * s_spatialObjectGridCell;


int SpatialListUtil::Hash(const IVector2D& cell, int maxSize)
{
	return ((cell.x * 73856093) ^ (cell.y * 19349663)) & (maxSize - 1);
}

int SpatialListUtil::SizeInCells(float units)
{
	return floor(units * s_spatialOneByObjectGridCell + 0.5f);
}

IVector2D SpatialListUtil::GetCell(const Vector3D& pos)
{
	IVector2D cell(
		SizeInCells(static_cast<float>(s_spatialObjectGridSize) + pos.x),
		SizeInCells(static_cast<float>(s_spatialObjectGridSize) + pos.z));
	return cell;
}

Vector3D SpatialListUtil::GetPosition(const IVector2D& cell)
{
	const float gridCell = static_cast<float>(s_spatialObjectGridCell);

	Vector3D pos(
		cell.x * gridCell - s_spatialObjectGridSize,
		0,
		cell.y * gridCell - s_spatialObjectGridSize
	);

	return pos;
}

SpatialList::SpatialList(PPSourceLine sl, const int poolSize)
	: m_pool(sl), m_buckets(sl)
{
	ASSERT(poolSize > 0);

	const int bucketsSize = nextPowerOf2(poolSize);
	m_buckets.setNum(bucketsSize);
	m_pool.setNum(poolSize);

	Clear();
}

void SpatialList::Clear()
{
	for (int i = 0; i < m_buckets.numElem(); ++i)
		m_buckets[i] = 0xffff;
	m_poolHead = 0;
}

void SpatialList::Add(const IVector2D cell, ushort id)
{
	if (m_poolHead >= m_pool.numElem())
		return;

	const int h = SpatialListUtil::Hash(cell, m_buckets.numElem());
	const ushort idx = (ushort)m_poolHead++;
	Item& item = m_pool[idx];
	item.pos = cell;
	item.id = id;
	item.next = m_buckets[h];
	m_buckets[h] = idx;
}

void SpatialList::QueryCell(const IVector2D& cell, const WalkCellFunc& func) const
{
	const int h = SpatialListUtil::Hash(cell, m_buckets.numElem());
	ushort idx = m_buckets[h];
	while (idx != 0xffff)
	{
		const Item& item = m_pool[idx];
		if (item.pos == cell)
		{
			if (!func(item.id))
				return;
		}
		idx = item.next;
	}
}