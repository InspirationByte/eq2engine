//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLVertexFormat.h"

CVertexFormatGL::CVertexFormatGL(const char* name, ArrayCRef<VertexLayoutDesc> vertexLayout)
{
	m_name = name;
	m_nameHash = StringToHash(name);

	memset(m_genericAttribs,0,sizeof(m_genericAttribs));
	
	m_vertexDesc.setNum(vertexLayout.numElem());

	for(int i = 0; i < vertexLayout.numElem(); i++)
		m_vertexDesc[i] = vertexLayout[i];
		
	int numGenericAttrib = 0;
	for (int stream = 0; stream < vertexLayout.numElem(); ++stream)
	{
		const VertexLayoutDesc& layoutDesc = m_vertexDesc[stream];
		for (const VertexLayoutDesc::AttribDesc& attrib : layoutDesc.attributes) 
		{
			if (attrib.type == VERTEXATTRIB_UNKNOWN || attrib.format == ATTRIBUTEFORMAT_NONE)
				continue;
			m_genericAttribs[numGenericAttrib].streamId = stream;
			m_genericAttribs[numGenericAttrib].count = attrib.count;
			m_genericAttribs[numGenericAttrib].offsetInBytes = attrib.offset;
			m_genericAttribs[numGenericAttrib].attribFormat = attrib.format;
			++numGenericAttrib;
		}
	}
}

int CVertexFormatGL::GetVertexSize(int nStream) const
{
	return m_vertexDesc[nStream].stride;
}

ArrayCRef<VertexLayoutDesc> CVertexFormatGL::GetFormatDesc() const
{
	return m_vertexDesc;
}