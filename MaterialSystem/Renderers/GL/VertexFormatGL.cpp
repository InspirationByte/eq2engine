//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "VertexFormatGL.h"

CVertexFormatGL::CVertexFormatGL()
{
	memset(m_hGeneric,0,sizeof(m_hGeneric));
	memset(m_hTexCoord,0,sizeof(m_hTexCoord));
	memset(&m_hVertex,0,sizeof(m_hVertex));
	memset(&m_hNormal,0,sizeof(m_hNormal));
	memset(&m_hColor,0,sizeof(m_hColor));

	memset(m_nVertexSize,0,sizeof(m_nVertexSize));

	m_nMaxGeneric = 0;
	m_nMaxTexCoord = 0;
}

int8 CVertexFormatGL::GetVertexSizePerStream(int16 nStream)
{
	return m_nVertexSize[nStream];
}