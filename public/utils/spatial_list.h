//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Object spatial list
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct SpatialListUtil
{
	static int				Hash(const IVector2D& cell);
	static int				SizeInCells(float units);
	static IVector2D		GetCell(const Vector3D& pos);
	static Vector3D			GetPosition(const IVector2D& cell);
};

template<typename T>
struct SpatialObjectAccessor
{
	static Vector3D GetPosition(T* obj);
};

template<typename T>
struct SpatialList
{
	SpatialList(PPSourceLine sl) 
		: indexGrid(sl), objects(sl) 
	{
	}

	void Clear(bool deallocate = false)
	{
		indexGrid.clear(deallocate);
		objects.clear(deallocate);
	}

	void Add(T* obj)
	{
		const IVector2D cell = SpatialListUtil::GetCell(SpatialObjectAccessor<T>::GetPosition(obj));

		Array<int>& objIndexList = indexGrid[SpatialListUtil::Hash(cell)];
		objIndexList.append(objects.append(obj));
	}

	Map<int, Array<int>>	indexGrid;
	Array<T*>				objects;
};
