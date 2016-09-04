//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include <malloc.h>

#include "IndexBufferGL.h"
#include "ShaderAPIGL.h"
#include "DebugInterface.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

CIndexBufferGL::CIndexBufferGL()
{
	m_nIndices = 0;
	m_nIndexSize = 0;

	m_bIsLocked = false;
	m_nGL_IB_Index = 0;
}

int8 CIndexBufferGL::GetIndexSize()
{
	return m_nIndexSize;
}

int CIndexBufferGL::GetIndicesCount()
{
	return m_nIndices;
}

// updates buffer without map/unmap operations which are slower
void CIndexBufferGL::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	bool dynamic = (m_usage == GL_DYNAMIC_DRAW);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer can't be updated while locked!");
		return;
	}

	if(offset+size > m_nIndices && !dynamic)
	{
		ASSERT(!"Update() with bigger size cannot be used on static vertex buffer!");
		return;
	}

	ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

	pGLRHI->GL_CRITICAL();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset*m_nIndexSize, size*m_nIndexSize, data);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if(dynamic && discard && offset == 0)
		m_nIndices = size;
}

// locks index buffer and gives to programmer buffer data
bool CIndexBufferGL::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	bool dynamic = (m_usage == GL_DYNAMIC_DRAW);

	if(m_bIsLocked)
	{
		ASSERT(!"Index buffer already locked! (You must unlock it first!)");
		return false;
	}

	if(sizeToLock > m_nIndices && !dynamic)
	{
		MsgError("Static index buffer is not resizable, must be less or equal %d (%d)\n", m_nIndices, sizeToLock);
		ASSERT(!"Static index buffer is not resizable. Debug it!\n");
		return false;
	}

	// discard if dynamic
	bool discard = dynamic;

	if(readOnly)
		discard = false;

	// don't lock at other offset
	// TODO: in other APIs
	if( discard )
		lockOfs = 0;

	m_lockDiscard = discard;

	// allocate memory for lock data
	m_lockSize = sizeToLock;
	m_lockOffs = lockOfs;
	m_lockReadOnly = readOnly;

	ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

#ifdef USE_GLES2
	// map buffer
	pGLRHI->GL_CRITICAL();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);

	GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | (discard ? GL_MAP_INVALIDATE_RANGE_BIT : 0);
	m_lockPtr = (ubyte*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, m_nIndices*m_nIndexSize, mapFlags );
	(*outdata) = m_lockPtr + m_lockOffs*m_nIndexSize;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	pGLRHI->GL_END_CRITICAL();

	if(m_lockPtr == NULL)
		ASSERTMSG(false, "Failed to map index buffer!");
#else
	int nLockByteCount = m_nIndexSize*sizeToLock;

	m_lockPtr = (ubyte*)malloc(nLockByteCount);
	(*outdata) = m_lockPtr;

	// read data into the buffer if we're not discarding
	if( !discard )
	{
		pGLRHI->GL_CRITICAL();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);

		// lock whole buffer
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_nIndices*m_nIndexSize, m_lockPtr);

		// give user buffer with offset
		(*outdata) = m_lockPtr + m_lockOffs*m_nIndexSize;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
#endif // USE_GLES2

	m_bIsLocked = true;

	if( dynamic && discard )
		m_nIndices = sizeToLock;

	return true;
}

// unlocks buffer
void CIndexBufferGL::Unlock()
{
	if(m_bIsLocked)
	{
		if( !m_lockReadOnly )
		{
			ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

			pGLRHI->GL_CRITICAL();

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);

#ifdef USE_GLES2
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
#else
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_lockSize*m_nIndexSize, m_lockPtr);
#endif // USE_GLES2

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

#ifndef USE_GLES2 // don't do dis...
		free(m_lockPtr);
#endif // USE_GLES2
		m_lockPtr = NULL;
	}
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}
