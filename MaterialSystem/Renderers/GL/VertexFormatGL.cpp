//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "VertexFormatGL.h"

CVertexFormatGL::CVertexFormatGL(VertexFormatDesc_t* desc, int numAttribs) 
	: m_vertexDesc(NULL), m_numAttribs(numAttribs)
{
	memset(m_hGeneric,0,sizeof(m_hGeneric));
	memset(m_streamStride, 0, sizeof(m_streamStride));
	
	if(m_numAttribs)
	{
		m_vertexDesc = new VertexFormatDesc_t[m_numAttribs];

		for(int i = 0; i < m_numAttribs; i++)
			m_vertexDesc[i] = desc[i];
		
		int nGeneric = 0;

		for (int i = 0; i < m_numAttribs; i++)
		{
			int stream = m_vertexDesc[i].m_nStream;

			switch (m_vertexDesc[i].m_nType)
			{
				case VERTEXTYPE_TANGENT:
				case VERTEXTYPE_BINORMAL:
				case VERTEXTYPE_VERTEX:
				case VERTEXTYPE_NORMAL:
				case VERTEXTYPE_TEXCOORD:
				case VERTEXTYPE_COLOR:
				case VERTEXTYPE_NONE:
					m_hGeneric[nGeneric].m_nStream = stream;
					m_hGeneric[nGeneric].m_nSize   = m_vertexDesc[i].m_nSize;
					m_hGeneric[nGeneric].m_nOffset = m_streamStride[stream];
					m_hGeneric[nGeneric].m_nFormat = m_vertexDesc[i].m_nFormat;
					nGeneric++;
					break;
			}

			m_streamStride[stream] += m_vertexDesc[i].m_nSize * attributeFormatSize[m_vertexDesc[i].m_nFormat];
		}
	}
}

CVertexFormatGL::~CVertexFormatGL()
{
	delete [] m_vertexDesc;
}

int CVertexFormatGL::GetVertexSize(int nStream)
{
	return m_streamStride[nStream];
}

void CVertexFormatGL::GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs)
{
	*desc = m_vertexDesc;
	numAttribs = m_numAttribs;
}