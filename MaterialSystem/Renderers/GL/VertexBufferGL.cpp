//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include <malloc.h>

#include "ShaderAPIGL.h"
#include "VertexBufferGL.h"
#include "DebugInterface.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

CVertexBufferGL::CVertexBufferGL()
{
	m_nGL_VB_Index = 0;
	m_numVerts = 0;
	m_strideSize = 0;

	m_bIsLocked = false;
	m_lockDiscard = false;
	m_lockPtr = NULL;

	m_boundStream = -1;
}

// returns size in bytes
long CVertexBufferGL::GetSizeInBytes()
{
	return m_strideSize*m_numVerts;
}

// returns vertex count
int CVertexBufferGL::GetVertexCount()
{
	return m_numVerts;
}

// retuns stride size
int CVertexBufferGL::GetStrideSize()
{
	return m_strideSize;
}

// updates buffer without map/unmap operations which are slower
void CVertexBufferGL::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	bool dynamic = (m_usage == GL_DYNAMIC_DRAW);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer can't be updated while locked!");
		return;
	}

	if(offset+size > m_numVerts && !dynamic)
	{
		ASSERT(!"Update() with bigger size cannot be used on static vertex buffer!");
		return;
	}

	ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

	pGLRHI->ThreadingSharingRequest();

	glBindBuffer(GL_ARRAY_BUFFER, m_nGL_VB_Index);
	glBufferSubData(GL_ARRAY_BUFFER, offset*m_strideSize, size*m_strideSize, data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	pGLRHI->ThreadingSharingRelease();

	if(dynamic && discard && offset == 0)
		m_numVerts = size;


}

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferGL::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	bool dynamic = (m_usage == GL_DYNAMIC_DRAW);

	if(m_bIsLocked)
	{
		ASSERT(!"Vertex buffer already locked! (You must unlock it first!)");
		return false;
	}

	// discard if dynamic
	bool discard = dynamic;

	// don't lock at other offset
	// TODO: in other APIs
	if( discard )
		lockOfs = 0;

	if(lockOfs+sizeToLock > m_numVerts && !dynamic)
	{
		ASSERT(!"Static vertex buffer is not resizable. Debug it!\n");
		return false;
	}

	if(readOnly)
		discard = false;

	m_lockDiscard = discard;

	// allocate memory for lock data
	m_lockSize = sizeToLock;
	m_lockOffs = lockOfs;
	m_lockReadOnly = readOnly;

	ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

#ifdef USE_GLES2
	// map buffer
	pGLRHI->ThreadingSharingRequest();
	glBindBuffer(GL_ARRAY_BUFFER, m_nGL_VB_Index);

	GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | (discard ? GL_MAP_INVALIDATE_RANGE_BIT : 0);
	m_lockPtr = (ubyte*)glMapBufferRange(GL_ARRAY_BUFFER, 0, m_numVerts*m_strideSize, mapFlags );
	(*outdata) = m_lockPtr + m_lockOffs*m_strideSize;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	pGLRHI->ThreadingSharingRelease();

	if(m_lockPtr == NULL)
		ASSERTMSG(false, "Failed to map vertex buffer!");
#else

	int nLockByteCount = m_strideSize*sizeToLock;

	m_lockPtr = (ubyte*)malloc(nLockByteCount);
	(*outdata) = m_lockPtr;

	// read data into the buffer if we're not discarding
	if( !discard )
	{
		pGLRHI->ThreadingSharingRequest();
		glBindBuffer(GL_ARRAY_BUFFER, m_nGL_VB_Index);

		// lock whole buffer
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, m_numVerts*m_strideSize, m_lockPtr);

		// give user buffer with offset
		(*outdata) = m_lockPtr + m_lockOffs*m_strideSize;

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		pGLRHI->ThreadingSharingRelease();
	}
#endif // USE_GLES2

	m_bIsLocked = true;

	if( dynamic && discard )
		m_numVerts = sizeToLock;

	return true;
}

// unlocks buffer
void CVertexBufferGL::Unlock()
{
	if(m_bIsLocked)
	{
		if( !m_lockReadOnly )
		{
			ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;
			pGLRHI->ThreadingSharingRequest();

			//if( m_boundStream == -1 )
			glBindBuffer(GL_ARRAY_BUFFER, m_nGL_VB_Index);

#ifdef USE_GLES2
			glUnmapBuffer(GL_ARRAY_BUFFER);
#else
			glBufferSubData(GL_ARRAY_BUFFER, 0, m_lockSize*m_strideSize, m_lockPtr);
#endif // USE_GLES2

			// check if bound
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			pGLRHI->ThreadingSharingRelease();
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
