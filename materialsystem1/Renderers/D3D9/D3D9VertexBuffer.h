//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexBuffer.h"

class CD3D9VertexBuffer : public IVertexBuffer
{
public:
	
	friend class				ShaderAPID3D9;

								CD3D9VertexBuffer();
								~CD3D9VertexBuffer();

	int							GetSizeInBytes() const;
	int							GetVertexCount() const;
	int							GetStrideSize() const;

	// updates buffer without map/unmap operations which are slower
	void						Update(void* data, int size, int offset, bool discard /*= true*/);

	// locks vertex buffer and gives to programmer buffer data
	bool						Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void						Unlock();

	void						ReleaseForRestoration();
	void						Restore();

	// sets vertex buffer flags
	void						SetFlags( int flags ) { m_flags = flags; }
	int							GetFlags() const { return m_flags; }

protected:
	int							m_flags;

	LPDIRECT3DVERTEXBUFFER9		m_pVertexBuffer;
	long						m_nSize;
	long						m_nInitialSize;

	int							m_nNumVertices;
	int							m_nStrideSize;
	DWORD						m_nUsage;

	bool						m_bIsLocked;

	struct VBRestoreData
	{
		void*	data;
		int		size;
	};
	VBRestoreData*				m_restore{ nullptr };
};