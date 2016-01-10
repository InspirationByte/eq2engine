//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef INDEXBUFFERGL_H
#define INDEXBUFFERGL_H

#include "Platform.h"
#include "IIndexBuffer.h"

class CIndexBufferGL : public IIndexBuffer
{
public:
	
	friend class	ShaderAPIGL;

					CIndexBufferGL();

	// returns index size
	int8			GetIndexSize();

	// returns index count
	int				GetIndicesCount();

	// locks index buffer and gives to programmer buffer data
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void			Unlock();

protected:
	uint			m_nGL_IB_Index;
	bool			m_bIsLocked;
	int				m_usage;

	uint			m_nIndices;
	int8			m_nIndexSize;

	ubyte*			m_lockPtr;
	int				m_lockOffs;
	int				m_lockSize;
	bool			m_lockDiscard;
	bool			m_lockReadOnly;
};

#endif // INDEXBUFFERGL_H