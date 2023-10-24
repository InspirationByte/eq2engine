//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexBuffer.h"

#define MAX_VB_SWITCHING 8

class CVertexBufferGL : public IVertexBuffer
{
	friend class ShaderAPIGL;

public:
	int				GetSizeInBytes() const { return m_bufElemSize * m_bufElemCapacity; }
	int				GetVertexCount() const { return m_bufElemCapacity; }

	int				GetStrideSize() const { return m_bufElemSize; }

	void			Update(void* data, int size, int offset = 0);

	bool			Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void			Unlock();

	void			SetFlags( int flags ) { m_flags = flags; }
	int				GetFlags() const { return m_flags; }

	uint			GetCurrentBuffer() const { return m_rhiBuffer[m_bufferIdx]; }
	 
protected:
	void			IncrementBuffer();

	uint			m_rhiBuffer[MAX_VB_SWITCHING]{ 0 };
	int				m_bufferIdx{ 0 };

	EBufferAccessType	m_access;

	int				m_bufElemCapacity{ 0 };
	int				m_bufElemSize{ 0 };

	ubyte*			m_lockPtr{ nullptr };
	int				m_lockOffs{ 0 };
	int				m_lockSize{ 0 };
	int				m_lockFlags{ 0 };

	int				m_flags{ 0 };
};