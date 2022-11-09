//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Game object spatial list
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "spatial_list.h"

constexpr const int		s_spatialObjectGridCell = 16;
constexpr const float	s_spatialOneByObjectGridCell = 1.0f / static_cast<float>(s_spatialObjectGridCell);
constexpr const int		s_spatialObjectGridSize = 2048 * s_spatialObjectGridCell;


int SpatialListUtil::Hash(const IVector2D& cell)
{
	return cell.y << 16 | cell.x;
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
