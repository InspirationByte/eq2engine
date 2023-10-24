//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IIndexBuffer.h"

#define MAX_IB_SWITCHING 8

class CIndexBufferGL : public IIndexBuffer
{
	friend class	ShaderAPIGL;
public:
	int				GetSizeInBytes() const { return m_bufElemSize * m_bufElemCapacity; }
	int				GetIndexSize() const { return m_bufElemSize; }
	int				GetIndicesCount() const { return m_bufElemCapacity; }

	void			Update(void* data, int size, int offset = 0);

	bool			Lock(int lockOfs, int sizeToLock, void** outdata, int flags);
	void			Unlock();

	uint			GetCurrentBuffer() const { return m_rhiBuffer[m_bufferIdx]; }

protected:
	void			IncrementBuffer();

	uint			m_rhiBuffer[MAX_IB_SWITCHING]{ 0 };
	int				m_bufferIdx{ 0 };

	EBufferAccessType	m_access;

	int				m_bufElemCapacity{ 0 };
	int				m_bufElemSize{ 0 };

	ubyte*			m_lockPtr{ nullptr };
	int				m_lockOffs{ 0 };
	int				m_lockSize{ 0 };
	int				m_lockFlags{ 0 };
};
