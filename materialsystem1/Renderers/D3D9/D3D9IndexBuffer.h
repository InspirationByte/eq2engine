//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IIndexBuffer.h"

class CD3D9IndexBuffer : public IIndexBuffer
{
public:
	
	friend class			ShaderAPID3D9;

							CD3D9IndexBuffer();
							~CD3D9IndexBuffer();

	int						GetSizeInBytes() const;
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

	int						m_nInitialSize;

	DWORD					m_nUsage;

	bool					m_bIsLocked;
	bool					m_bLockFail;

	struct IBRestoreData
	{
		void*	data;
		int		size;
	};
	IBRestoreData*			m_restore{ nullptr };
};