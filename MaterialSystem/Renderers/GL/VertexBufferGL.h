//**************Copyright (C) Two Dark Interactive Software 2009**************
//
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer OpenGL declaration
//
//****************************************************************************

#ifndef VERTEXBUFFERGL_H
#define VERTEXBUFFERGL_H

#include "Platform.h"
#include "IVertexBuffer.h"

class CVertexBufferGL : public IVertexBuffer
{
public:
	friend class	ShaderAPIGL;

					CVertexBufferGL();

	// returns size in bytes
	long			GetSizeInBytes();

	// returns vertex count
	int				GetVertexCount();

	// retuns stride size
	int				GetStrideSize();

	// locks vertex buffer and gives to programmer buffer data
	bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly);

	// unlocks buffer
	void			Unlock();

	// sets vertex buffer flags
	void			SetFlags( int flags ) {m_flags = flags;}
	int				GetFlags() {return m_flags;}

protected:
	int				m_flags;

	uint			m_nGL_VB_Index;
	bool			m_bIsLocked;

	int				m_numVerts;
	int				m_strideSize;
	int				m_usage;

	int				m_boundStream;

	ubyte*			m_lockPtr;
	int				m_lockOffs;
	int				m_lockSize;
	bool			m_lockDiscard;
	bool			m_lockReadOnly;
};

#endif // VERTEXBUFFERGL_H