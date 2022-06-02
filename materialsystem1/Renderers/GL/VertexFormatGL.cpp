//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "VertexFormatGL.h"

CVertexFormatGL::CVertexFormatGL(const char* name, VertexFormatDesc_t* desc, int numAttribs)
	: m_vertexDesc(nullptr), m_numAttribs(numAttribs)
{
	m_name = name;
	memset(m_genericAttribs,0,sizeof(m_genericAttribs));
	memset(m_streamStride, 0, sizeof(m_streamStride));
	
	if(m_numAttribs)
	{
		m_vertexDesc = PPNew VertexFormatDesc_t[m_numAttribs];

		for(int i = 0; i < m_numAttribs; i++)
			m_vertexDesc[i] = desc[i];
		
		int nGeneric = 0;

		for (int i = 0; i < m_numAttribs; i++)
		{
			int stream = m_vertexDesc[i].streamId;

			switch (m_vertexDesc[i].attribType)
			{
				case VERTEXATTRIB_TANGENT:
				case VERTEXATTRIB_BINORMAL:
				case VERTEXATTRIB_POSITION:
				case VERTEXATTRIB_NORMAL:
				case VERTEXATTRIB_TEXCOORD:
				case VERTEXATTRIB_COLOR:
					m_genericAttribs[nGeneric].streamId			= stream;
					m_genericAttribs[nGeneric].sizeInBytes		= m_vertexDesc[i].elemCount;
					m_genericAttribs[nGeneric].offsetInBytes	= m_streamStride[stream];
					m_genericAttribs[nGeneric].attribFormat		= m_vertexDesc[i].attribFormat;
					nGeneric++;
					break;
				case VERTEXATTRIB_UNUSED:
					break;
			}

			m_streamStride[stream] += m_vertexDesc[i].elemCount * s_attributeSize[m_vertexDesc[i].attribFormat];
		}
	}
}

CVertexFormatGL::~CVertexFormatGL()
{
	delete [] m_vertexDesc;
}

int CVertexFormatGL::GetVertexSize(int nStream) const
{
	return m_streamStride[nStream];
}

void CVertexFormatGL::GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) const
{
	*desc = m_vertexDesc;
	numAttribs = m_numAttribs;
}