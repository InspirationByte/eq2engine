//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IIndexBuffer.h"

class CD3D9IndexBuffer : public IIndexBuffer
{
	friend class ShaderAPID3D9;
public:
	~CD3D9IndexBuffer();

	int						GetSizeInBytes() const { return m_bufElemCapacity * m_bufElemSize; }
	int						GetIndicesCount() const { return m_bufElemCapacity; }
	int						GetIndexSize() const { return m_bufElemSize; }

	void					Update(void* data, int size, int offset = 0);

	bool					Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void					Unlock();

	void					ReleaseForRestoration();
	void					Restore();

protected:
	IDirect3DIndexBuffer9*	m_rhiBuffer{ 0 };
	DWORD					m_bufUsage{ 0 };

	int						m_bufInitialSize{ 0 };

	int						m_bufElemCapacity{ 0 };
	int						m_bufElemSize{ 0 };

	int						m_lockFlags{ 0 };

	struct RestoreData
	{
		~RestoreData();
		void*	data;
		int		size;
	};
	RestoreData*			m_restore{ nullptr };
};