//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLVertexFormat.h"

CVertexFormatGL::CVertexFormatGL(const char* name, const VertexFormatDesc* desc, int numAttribs)
{
	m_name = name;
	memset(m_genericAttribs,0,sizeof(m_genericAttribs));
	memset(m_streamStride, 0, sizeof(m_streamStride));
	
	if(numAttribs)
	{
		m_vertexDesc.setNum(numAttribs);

		for(int i = 0; i < numAttribs; i++)
			m_vertexDesc[i] = desc[i];
		
		int nGeneric = 0;
		for (int i = 0; i < numAttribs; i++)
		{
			const VertexFormatDesc& fmtdesc = m_vertexDesc[i];
			const ER_VertexAttribType attribType = static_cast<ER_VertexAttribType>(fmtdesc.attribType & VERTEXATTRIB_MASK);
			const int stream = fmtdesc.streamId;

			switch (attribType)
			{
				case VERTEXATTRIB_TANGENT:
				case VERTEXATTRIB_BINORMAL:
				case VERTEXATTRIB_POSITION:
				case VERTEXATTRIB_NORMAL:
				case VERTEXATTRIB_TEXCOORD:
				case VERTEXATTRIB_COLOR:
					m_genericAttribs[nGeneric].streamId	= stream;
					m_genericAttribs[nGeneric].sizeInBytes = fmtdesc.elemCount;
					m_genericAttribs[nGeneric].offsetInBytes = m_streamStride[stream];
					m_genericAttribs[nGeneric].attribFormat = fmtdesc.attribFormat;
					nGeneric++;
					break;
				case VERTEXATTRIB_UNUSED:
					break;
			}

			m_streamStride[stream] += fmtdesc.elemCount * s_attributeSize[fmtdesc.attribFormat];
		}
	}
}

int CVertexFormatGL::GetVertexSize(int nStream) const
{
	return m_streamStride[nStream];
}

ArrayCRef<VertexFormatDesc> CVertexFormatGL::GetFormatDesc() const
{
	return m_vertexDesc;
}