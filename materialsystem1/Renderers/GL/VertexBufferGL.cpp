//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "VertexBufferGL.h"

#include "ShaderAPIGL.h"
#include "shaderapigl_def.h"

#include "core/DebugInterface.h"

#ifdef USE_GLES2
#include <glad_es3.h>
#else
#include <glad.h>
#endif

#include <malloc.h>


extern ShaderAPIGL g_shaderApi;

extern bool GLCheckError(const char* op, ...);

CVertexBufferGL::CVertexBufferGL()
{
	m_numVerts = 0;
	m_strideSize = 0;

	m_bIsLocked = false;
	m_lockDiscard = false;
	m_lockPtr = NULL;
	m_flags = 0;

	memset(m_nGL_VB_Index, 0, sizeof(m_nGL_VB_Index));
	m_bufferIdx = 0;
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

void CVertexBufferGL::IncrementBuffer()
{
	if (m_access != BUFFER_DYNAMIC)
		return;

	const int numBuffers = MAX_VB_SWITCHING;

	int bufferIdx = m_bufferIdx;

	bufferIdx++;
	if (bufferIdx >= numBuffers)
		bufferIdx = 0;

	m_bufferIdx = bufferIdx;
}

// updates buffer without map/unmap operations which are slower
void CVertexBufferGL::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	bool dynamic = (m_access == BUFFER_DYNAMIC);

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

	if (offset > 0)
		IncrementBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
	GLCheckError("vertexbuffer update bind");

	if (offset > 0) // streaming
		glBufferSubData(GL_ARRAY_BUFFER, offset*m_strideSize, size*m_strideSize, data);
	else // orphaning
		glBufferData(GL_ARRAY_BUFFER, size*m_strideSize, data, glBufferUsages[m_access]);

	GLCheckError("vertexbuffer update");

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if(dynamic && discard && offset == 0)
		m_numVerts = size;
}

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferGL::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	bool dynamic = (m_access == BUFFER_DYNAMIC);

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

	IncrementBuffer();

	m_lockDiscard = discard;

	// allocate memory for lock data
	m_lockSize = sizeToLock;
	m_lockOffs = lockOfs;
	m_lockReadOnly = readOnly;

#ifdef USE_GLES2
	// map buffer
	glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());

	GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;// | (discard ? GL_MAP_INVALIDATE_BUFFER_BIT : 0);
	GLCheckError("vertexbuffer map");
	m_lockPtr = (ubyte*)glMapBufferRange(GL_ARRAY_BUFFER, m_lockOffs*m_strideSize, m_lockSize*m_strideSize, mapFlags );
	(*outdata) = m_lockPtr;// + m_lockOffs*m_strideSize;

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if(m_lockPtr == NULL)
		ASSERT_MSG(false, "Failed to map vertex buffer!");
#else

	int nLockByteCount = m_strideSize*sizeToLock;

	m_lockPtr = (ubyte*)malloc(nLockByteCount);
	(*outdata) = m_lockPtr;

	// read data into the buffer if we're not discarding
	if( !discard )
	{
		glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
		GLCheckError("vertexbuffer get data bind");

		// lock whole buffer
		glGetBufferSubData(GL_ARRAY_BUFFER, m_lockOffs*m_strideSize, nLockByteCount, m_lockPtr);
		GLCheckError("vertexbuffer get data");

		// give user buffer with offset
		(*outdata) = m_lockPtr;// + m_lockOffs*m_strideSize;

		glBindBuffer(GL_ARRAY_BUFFER, 0);
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
			glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
			GLCheckError("vertexbuffer unmap bind");

#ifdef USE_GLES2
			glUnmapBuffer(GL_ARRAY_BUFFER);
#else
			glBufferSubData(GL_ARRAY_BUFFER, m_lockOffs*m_strideSize, m_lockSize*m_strideSize, m_lockPtr);
#endif // USE_GLES2
			GLCheckError("vertexbuffer unmap");

			glBindBuffer(GL_ARRAY_BUFFER, 0);
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
