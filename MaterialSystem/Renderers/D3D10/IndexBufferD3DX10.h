//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef INDEXBUFFERD3DX10_H
#define INDEXBUFFERD3DX10_H

#include "IIndexBuffer.h"

class CIndexBufferD3DX10 : public IIndexBuffer
{
public:
	
	friend class			ShaderAPID3DX10;

							CIndexBufferD3DX10();

	int8					GetIndexSize();
	int						GetIndicesCount();

	// locks index buffer and gives to programmer buffer data
	bool					Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void					Unlock();

protected:
	ID3D10Buffer*			m_pIndexBuffer;

	D3D10_USAGE				m_nUsage;

	uint					m_nIndices;
	int8					m_nIndexSize;

	bool					m_bIsLocked;
};

#endif // INDEXBUFFERD3DX9_H