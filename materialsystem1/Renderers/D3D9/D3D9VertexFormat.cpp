//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format OpenGL declaration
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9VertexFormat.h"

CD3D9VertexFormat::CD3D9VertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> vertexLayout)
{
	m_name = name;
	m_nameHash = StringToHash(name);

	m_vertexDesc.setNum(vertexLayout.numElem());
	for(int i = 0; i < vertexLayout.numElem(); i++)
		m_vertexDesc[i] = vertexLayout[i];
}

CD3D9VertexFormat::~CD3D9VertexFormat()
{
	if (m_pVertexDecl)
		m_pVertexDecl->Release();
}

int CD3D9VertexFormat::GetVertexSize(int nStream) const
{
	return m_vertexDesc[nStream].stride;
}

ArrayCRef<VertexLayoutDesc> CD3D9VertexFormat::GetFormatDesc() const
{
	return m_vertexDesc;
}

//----------------------

void CD3D9VertexFormat::GenVertexElement(D3DVERTEXELEMENT9* elems)
{
	int numRealAttribs = 0;
	int index[VERTEXATTRIB_COUNT] = {0};

	// Fill the vertex element array
	for (int stream = 0; stream < m_vertexDesc.numElem(); ++stream)
	{
		const VertexLayoutDesc& layoutDesc = m_vertexDesc[stream];
		for (const VertexLayoutDesc::AttribDesc& attrib : layoutDesc.attributes)
		{
			if (attrib.type == VERTEXATTRIB_UNKNOWN || attrib.format == ATTRIBUTEFORMAT_NONE)
				continue;

			D3DVERTEXELEMENT9& elem = elems[numRealAttribs];
			elem.Stream = stream;
			elem.Offset = attrib.offset;
			elem.Type = g_d3d9_decltypes[attrib.format][attrib.count - 1];
			elem.Method = D3DDECLMETHOD_DEFAULT;
			elem.Usage = g_d3d9_vertexUsage[attrib.type];
			elem.UsageIndex = index[attrib.type]++;

			++numRealAttribs;
		}
	}

	// Terminating element
	memset(elems + numRealAttribs, 0, sizeof(D3DVERTEXELEMENT9));

	elems[numRealAttribs].Stream = 0xFF;
	elems[numRealAttribs].Type = D3DDECLTYPE_UNUSED;
}