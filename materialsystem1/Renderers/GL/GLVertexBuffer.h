//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexBuffer.h"

#define MAX_VB_SWITCHING 8

class CVertexBufferGL : public IVertexBuffer
{
public:
	friend class	ShaderAPIGL;

					CVertexBufferGL();

	// returns size in bytes
	int				GetSizeInBytes() const;

	// returns vertex count
	int				GetVertexCount() const;

	// retuns stride size
	int				GetStrideSize() const;

	// updates buffer without map/unmap operations which are slower
	void			Update(void* data, int size, int offset, bool discard = true);

	// locks vertex buffer and gives to programmer buffer data
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void			Unlock();

	// sets vertex buffer flags
	void			SetFlags( int flags ) {m_flags = flags;}
	int				GetFlags() const {return m_flags;}

	uint			GetCurrentBuffer() const { return m_nGL_VB_Index[m_bufferIdx]; }
	 
protected:
	void			IncrementBuffer();

	uint			m_nGL_VB_Index[MAX_VB_SWITCHING];
	int				m_bufferIdx;

	int				m_flags;
	int				m_numVerts;
	int				m_strideSize;
	ER_BufferAccess	m_access;

	ubyte*			m_lockPtr;
	int				m_lockOffs;
	int				m_lockSize;

	bool			m_lockDiscard;
	bool			m_lockReadOnly;
	bool			m_bIsLocked;
};