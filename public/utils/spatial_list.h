//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Object spatial list
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct SpatialListUtil
{
	static int				Hash(const IVector2D& cell, int maxSize);
	static int				SizeInCells(float units);
	static IVector2D		GetCell(const Vector3D& pos);
	static Vector3D			GetPosition(const IVector2D& cell);
};

template<typename T>
struct SpatialObjectAccessor
{
	static Vector3D GetPosition(T* obj);
};

class SpatialList
{
public:
	using WalkCellFunc = EqFunction<bool(ushort idx)>;


	SpatialList(PPSourceLine sl, const int poolSize);

	void Clear();
	void Add(const IVector2D cell, ushort id);
	void QueryCell(const IVector2D& cell, const WalkCellFunc& func) const;

	template<typename T>
	void Add(T* obj, ushort id)
	{
		const IVector2D cell = SpatialListUtil::GetCell(SpatialObjectAccessor<T>::GetPosition(obj));
		Add(cell, id);
	}

private:
	struct Item
	{
		ushort id;
		ushort next;
		TVec2D<short> pos;
	};

	Array<Item>		m_pool;
	int				m_poolHead{ 0 };

	Array<ushort>	m_buckets;
};
