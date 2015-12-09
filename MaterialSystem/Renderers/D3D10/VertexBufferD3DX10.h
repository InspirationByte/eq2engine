//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer Direct3D 9 declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXBUFFERD3DX9_H
#define VERTEXBUFFERD3DX9_H

#include "Platform.h"
#include "IVertexBuffer.h"

class CVertexBufferD3DX10 : public IVertexBuffer
{
public:
	
	friend class				ShaderAPID3DX10;

								CVertexBufferD3DX10();
	long						GetSizeInBytes();
	int							GetVertexCount();
	int							GetStrideSize();

	// locks vertex buffer and gives to programmer buffer data
	bool						Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void						Unlock();

	// sets vertex buffer flags
	void						SetFlags( int flags ) {m_flags = flags;}
	int							GetFlags() {return m_flags;}

protected:
	int							m_flags;

protected:
	ID3D10Buffer*				m_pVertexBuffer;
	long						m_nSize;
	int							m_nNumVertices;
	int							m_nStrideSize;
	D3D10_USAGE					m_nUsage;

	bool						m_bIsLocked;
};

#endif // VERTEXBUFFERD3DX9_H