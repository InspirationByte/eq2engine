//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef INDEXBUFFERGL_H
#define INDEXBUFFERGL_H

#include "renderers/IIndexBuffer.h"

#define MAX_IB_SWITCHING 8

class CIndexBufferGL : public IIndexBuffer
{
public:

	friend class	ShaderAPIGL;

					CIndexBufferGL();

	long			GetSizeInBytes() const;

	// returns index size
	int				GetIndexSize() const;

	// returns index count
	int				GetIndicesCount() const;

	// updates buffer without map/unmap operations which are slower
	void			Update(void* data, int size, int offset, bool discard /*= true*/);

	// locks index buffer and gives to programmer buffer data
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void			Unlock();

	uint			GetCurrentBuffer() const { return m_nGL_IB_Index[m_bufferIdx]; }

protected:
	void			IncrementBuffer();

	uint			m_nGL_IB_Index[MAX_IB_SWITCHING];
	int				m_bufferIdx;

	int				m_nIndices;
	int				m_nIndexSize;

	ER_BufferAccess	m_access;

	ubyte*			m_lockPtr;
	int				m_lockOffs;
	int				m_lockSize;

	bool			m_lockDiscard;
	bool			m_lockReadOnly;
	bool			m_bIsLocked;
};

#endif // INDEXBUFFERGL_H
