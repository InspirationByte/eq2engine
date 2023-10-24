//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLVertexBuffer.h"
#include "ShaderAPIGL.h"

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
void CVertexBufferGL::Update(void* data, int size, int offset)
{
	const bool dynamic = (m_access == BUFFER_DYNAMIC);

	if (m_lockFlags)
	{
		ASSERT(!"Buffer can't be updated while locked!");
		return;
	}

	if (!dynamic && offset + size > m_bufElemCapacity)
	{
		ASSERT(!"Update() with bigger size cannot be used on static buffer!");
		return;
	}

	if (offset > 0)
		IncrementBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
	GLCheckError("buffer update bind");

	const int lockByteCount = size * m_bufElemSize;
	const int lockByteOffset = offset * m_bufElemSize;

	if (lockByteOffset > 0) // streaming
		glBufferSubData(GL_ARRAY_BUFFER, lockByteOffset, lockByteCount, data);
	else // orphaning
		glBufferData(GL_ARRAY_BUFFER, lockByteCount, data, g_gl_bufferUsages[m_access]);

	GLCheckError("buffer update");

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (dynamic)
		m_bufElemCapacity = offset + size;
}

// locks vertex buffer and gives to programmer buffer data
bool CVertexBufferGL::Lock(int lockOfs, int sizeToLock, void** outdata, int flags)
{
	ASSERT_MSG(flags != 0, "Lock flags are invalid - specify flags from BUFFER_FLAGS");

	const bool dynamic = (m_access == BUFFER_DYNAMIC);
	const bool write = (flags & BUFFER_FLAG_WRITE);
	const bool readBack = (flags & BUFFER_FLAG_READ);
	const int lockByteCount = sizeToLock * m_bufElemSize;
	const int lockByteOffset = lockOfs * m_bufElemSize;

	if(m_lockFlags)
	{
		ASSERT_FAIL("Buffer already locked! (You must unlock it first!)");
		return false;
	}

	// validate lock
	if (lockOfs < 0 || (!dynamic && write || readBack) && lockOfs + sizeToLock > m_bufElemCapacity)
	{
		ASSERT_FAIL("locking outside buffer size range");
		return false;
	}

	if (!readBack)
		IncrementBuffer();

#ifdef USE_GLES2
	// map buffer
	glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
	GLCheckError("buffer lock bind");

	const GLbitfield mapFlags =
		GL_MAP_UNSYNCHRONIZED_BIT
		| (readBack ? GL_MAP_READ_BIT : 0)
		| (write ? GL_MAP_WRITE_BIT : 0);
	//	| (discard ? GL_MAP_INVALIDATE_BUFFER_BIT : 0);

	m_lockPtr = (ubyte*)glMapBufferRange(GL_ARRAY_BUFFER, lockByteOffset, lockByteCount, mapFlags );
	GLCheckError("buffer map");
	(*outdata) = m_lockPtr;

	// index buffer should be restored
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
	m_lockPtr = (ubyte*)PPAlloc(lockByteCount);
	(*outdata) = m_lockPtr;

	// read data into the buffer if we're not discarding
	if(readBack)
	{
        glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
		GLCheckError("buffer lock bind");

		// lock whole buffer
		glGetBufferSubData(GL_ARRAY_BUFFER, lockByteOffset, lockByteCount, m_lockPtr);
		GLCheckError("buffer get data");

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
#endif // USE_GLES2

	if (m_lockPtr)
	{
		m_lockSize = sizeToLock;
		m_lockOffs = lockOfs;
		m_lockFlags = flags;
	}
	else
	{
		ASSERT_FAIL("Failed to map index buffer!");
		m_lockFlags = 0;
	}

	if( dynamic && write )
		m_bufElemCapacity = lockOfs + sizeToLock;

	return m_lockFlags != 0;
}

// unlocks buffer
void CVertexBufferGL::Unlock()
{
	if (!m_lockFlags)
	{
		ASSERT_FAIL("Buffer is not locked!");
		return;
	}

#ifdef USE_GLES2
	glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
	GLCheckError("buffer unlock bind");

	glUnmapBuffer(GL_ARRAY_BUFFER);
	GLCheckError("buffer unlock unmap");

	// index buffer should be restored
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
	const bool lockWrite = m_lockFlags & BUFFER_FLAG_WRITE;
	if (lockWrite)
	{
		glBindBuffer(GL_ARRAY_BUFFER, GetCurrentBuffer());
		GLCheckError("buffer unlock bind");

		const int lockByteCount = m_lockSize * m_bufElemSize;
		const int lockByteOffset = m_lockOffs * m_bufElemSize;

		glBufferSubData(GL_ARRAY_BUFFER, lockByteOffset, lockByteCount, m_lockPtr);
		GLCheckError("buffer unlock sub data");

		// index buffer should be restored
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	PPFree(m_lockPtr);
#endif

	m_lockSize = 0;
	m_lockOffs = 0;
	m_lockFlags = 0;
	m_lockPtr = nullptr;
}
