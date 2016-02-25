//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration//////////////////////////////////////////////////////////////////////////////////

#ifndef INDEXBUFFERD3DX9_H
#define INDEXBUFFERD3DX9_H

#include "Platform.h"
#include "IIndexBuffer.h"

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

	int8					GetIndexSize();
	int						GetIndicesCount();

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

	uint					m_nIndices;
	int8					m_nIndexSize;

	long					m_nInitialSize;

	DWORD					m_nUsage;

	bool					m_bIsLocked;
	bool					m_bLockFail;

	iborestoredata_t*		m_pRestore;
};

#endif // INDEXBUFFERD3DX9_H