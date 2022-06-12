//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IIndexBuffer.h"

struct iborestoredata_t
{
	void*		data;
	int			size;
};

class CIndexBufferD3DX9 : public IIndexBuffer
{
public:
	
	friend class			ShaderAPID3DX9;

							CIndexBufferD3DX9();
							~CIndexBufferD3DX9();

	long					GetSizeInBytes() const;
	int						GetIndexSize() const;
	int						GetIndicesCount() const;

	// updates buffer without map/unmap operations which are slower
	void					Update(void* data, int size, int offset, bool discard /*= true*/);

	// locks index buffer and gives to programmer buffer data
	bool					Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void					Unlock();

	void					ReleaseForRestoration();
	void					Restore();

protected:
	LPDIRECT3DINDEXBUFFER9	m_pIndexBuffer;

	int						m_nIndices;
	int						m_nIndexSize;

	long					m_nInitialSize;

	DWORD					m_nUsage;

	bool					m_bIsLocked;
	bool					m_bLockFail;

	iborestoredata_t*		m_pRestore;
};