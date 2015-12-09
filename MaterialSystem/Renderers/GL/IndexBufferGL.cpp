//**************Copyright (C) Two Dark Interactive Software 2009**************
//
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Index Buffer OpenGL declaration
//
//****************************************************************************

#include <malloc.h>

#include "IndexBufferGL.h"
#include "ShaderAPIGL.h"
#include "DebugInterface.h"
#include "eqGLCaps.h"

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

	int nLockByteCount = m_nIndexSize*sizeToLock;

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
	m_lockPtr = (ubyte*)malloc(nLockByteCount);
	(*outdata) = m_lockPtr;
	m_lockSize = sizeToLock;
	m_lockOffs = lockOfs;
	m_lockReadOnly = readOnly;

	// read data into the buffer if we're not discarding
	if( !discard )
	{
		ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

		pGLRHI->ThreadingSharingRequest();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);

		// lock whole buffer
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_nIndices*m_nIndexSize, m_lockPtr);

		// give user buffer with offset
		(*outdata) = m_lockPtr + m_lockOffs*m_nIndexSize;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		pGLRHI->ThreadingSharingRelease();
	}

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

			pGLRHI->ThreadingSharingRequest();

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nGL_IB_Index);

			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_lockSize*m_nIndexSize, m_lockPtr);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			pGLRHI->ThreadingSharingRelease();
		}

		free(m_lockPtr);
		m_lockPtr = NULL;
	}
	else
		ASSERT(!"Vertex buffer is not locked!");

	m_bIsLocked = false;
}