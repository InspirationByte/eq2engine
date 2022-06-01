//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "IndexBufferGL.h"
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

CIndexBufferGL::CIndexBufferGL()
{
	m_nIndices = 0;
	m_nIndexSize = 0;

	m_bIsLocked = false;
	memset(m_nGL_IB_Index, 0, sizeof(m_nGL_IB_Index));
	m_bufferIdx = 0;
}

int8 CIndexBufferGL::GetIndexSize()
{
	return m_nIndexSize;
}

int CIndexBufferGL::GetIndicesCount()
{
	return m_nIndices;
}

void CIndexBufferGL::IncrementBuffer()
{
	if (m_access != BUFFER_DYNAMIC)
		return;

	const int numBuffers = MAX_IB_SWITCHING;

	int bufferIdx = m_bufferIdx;

	bufferIdx++;
	if (bufferIdx >= numBuffers)
		bufferIdx = 0;

	m_bufferIdx = bufferIdx;
}

// updates buffer without map/unmap operations which are slower
void CIndexBufferGL::Update(void* data, int size, int offset, bool discard /*= true*/)
{
	bool dynamic = (m_access == BUFFER_DYNAMIC);

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

	CIndexBufferGL* currIB = (CIndexBufferGL*)g_shaderApi.m_pCurrentIndexBuffer;

	if(offset > 0)
		IncrementBuffer();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GetCurrentBuffer());
	GLCheckError("indexbuffer update bind");

	if (offset > 0) // streaming
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset*m_nIndexSize, size*m_nIndexSize, data);
	else // orphaning
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size*m_nIndexSize, data, glBufferUsages[m_access]);

	GLCheckError("indexbuffer update");

	// index buffer should be restored
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currIB ? currIB->GetCurrentBuffer() : 0);

	if(dynamic && discard && offset == 0)
		m_nIndices = size;
}

// locks index buffer and gives to programmer buffer data
bool CIndexBufferGL::Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
{
	bool dynamic = (m_access == BUFFER_DYNAMIC);

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

	IncrementBuffer();

	// don't lock at other offset
	// TODO: in other APIs
	if( discard )
		lockOfs = 0;

	m_lockDiscard = discard;

	// allocate memory for lock data
	m_lockSize = sizeToLock;
	m_lockOffs = lockOfs;
	m_lockReadOnly = readOnly;

	CIndexBufferGL* currIB = (CIndexBufferGL*)g_shaderApi.m_pCurrentIndexBuffer;

#ifdef USE_GLES2
	// map buffer
	if(currIB != this)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GetCurrentBuffer());

	GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;// | (discard ? GL_MAP_INVALIDATE_BUFFER_BIT : 0);
	GLCheckError("indexbuffer map");
	m_lockPtr = (ubyte*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, m_lockOffs*m_nIndexSize, m_lockSize*m_nIndexSize, mapFlags );
	(*outdata) = m_lockPtr;// + m_lockOffs*m_nIndexSize;

	// index buffer should be restored
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currIB ? currIB->GetCurrentBuffer() : 0);

	if(m_lockPtr == nullptr)
		ASSERT_FAIL("Failed to map index buffer!");
#else
	int nLockByteCount = m_nIndexSize*sizeToLock;

	m_lockPtr = (ubyte*)PPAlloc(nLockByteCount);
	(*outdata) = m_lockPtr;

	// read data into the buffer if we're not discarding
	if( !discard )
	{
		if(currIB != this)
		{
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GetCurrentBuffer());
			GLCheckError("indexbuffer get data bind");
		}

		// lock whole buffer
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, m_lockOffs*m_nIndexSize, m_lockSize*m_nIndexSize, m_lockPtr);
		GLCheckError("indexbuffer get data");

		// give user buffer with offset
		(*outdata) = m_lockPtr;// + m_lockOffs*m_nIndexSize;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currIB ? currIB->GetCurrentBuffer() : 0);
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
			CIndexBufferGL* currIB = (CIndexBufferGL*)g_shaderApi.m_pCurrentIndexBuffer;

			if(currIB != this)
			{
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GetCurrentBuffer());
				GLCheckError("indexbuffer unmap bind");
			}

#ifdef USE_GLES2
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
#else
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, m_lockOffs*m_nIndexSize, m_lockSize*m_nIndexSize, m_lockPtr);
#endif // USE_GLES2
			GLCheckError("indexbuffer unmap");

			// index buffer should be restored
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currIB ? currIB->GetCurrentBuffer() : 0);
		}

#ifndef USE_GLES2 // don't do dis...
		PPFree(m_lockPtr);
#endif // USE_GLES2
		m_lockPtr = nullptr;
	}
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}
