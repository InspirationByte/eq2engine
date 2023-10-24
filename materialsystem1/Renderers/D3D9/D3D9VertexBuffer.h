//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexBuffer.h"

class CD3D9VertexBuffer : public IVertexBuffer
{
	friend class ShaderAPID3D9;
public:
	~CD3D9VertexBuffer();

	int						GetSizeInBytes() const { return m_bufElemCapacity * m_bufElemSize; }
	int						GetVertexCount() const { return m_bufElemCapacity; }
	int						GetStrideSize() const { return m_bufElemSize; }

	void					Update(void* data, int size, int offset = 0);

	bool					Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void					Unlock();

	void					SetFlags( int flags ) { m_flags = flags; }
	int						GetFlags() const { return m_flags; }

	void					ReleaseForRestoration();
	void					Restore();

protected:
	IDirect3DVertexBuffer9*	m_rhiBuffer{ nullptr };
	DWORD					m_bufUsage{ 0 };

	int						m_bufElemCapacity{ 0 };
	int						m_bufElemSize{ 0 };

	bool					m_lockFlags{ 0 };

	int						m_flags{ 0 };
	struct RestoreData
	{
		~RestoreData();
		void*	data;
		int		size;
	};
	RestoreData*			m_restore{ nullptr };
};