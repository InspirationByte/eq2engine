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

CD3D9VertexFormat::CD3D9VertexFormat(const char* name, const VertexFormatDesc* desc, int numAttribs)
{
	m_name = name;
	memset(m_streamStride, 0, sizeof(m_streamStride));

	m_vertexDesc.setNum(numAttribs);
	for(int i = 0; i < numAttribs; i++)
		m_vertexDesc[i] = desc[i];
}

CD3D9VertexFormat::~CD3D9VertexFormat()
{
	if (m_pVertexDecl)
		m_pVertexDecl->Release();
}

int CD3D9VertexFormat::GetVertexSize(int nStream) const
{
	return m_streamStride[nStream];
}

ArrayCRef<VertexFormatDesc> CD3D9VertexFormat::GetFormatDesc() const
{
	return m_vertexDesc;
}

//----------------------

void CD3D9VertexFormat::GenVertexElement(D3DVERTEXELEMENT9* elems)
{
	memset(m_streamStride, 0, sizeof(m_streamStride));

	int numRealAttribs = 0;

	int index[VERTEXATTRIB_COUNT] = {0};

	// Fill the vertex element array
	for (int i = 0; i < m_vertexDesc.numElem(); i++)
	{
		const VertexFormatDesc& fmtdesc = m_vertexDesc[i];
		D3DVERTEXELEMENT9& elem = elems[numRealAttribs];

		const int stream = fmtdesc.streamId;
		const int size = fmtdesc.elemCount;

		// if not unused
		if(fmtdesc.attribType != VERTEXATTRIB_UNUSED)
		{
			const EVertAttribType attribType = static_cast<EVertAttribType>(fmtdesc.attribType & VERTEXATTRIB_MASK);

			ASSERT_MSG(size - 1 < elementsOf(g_d3d9_decltypes[fmtdesc.attribFormat]), "VertexFormat size - incorrectly set up");

			elem.Stream = stream;
			elem.Offset = m_streamStride[stream];
			elem.Type = g_d3d9_decltypes[fmtdesc.attribFormat][size - 1];
			elem.Method = D3DDECLMETHOD_DEFAULT;
			elem.Usage = g_d3d9_vertexUsage[attribType];
			elem.UsageIndex = index[attribType]++;

			numRealAttribs++;
		}

		m_streamStride[stream] += size * s_attributeSize[fmtdesc.attribFormat];
	}

	// Terminating element
	memset(elems + numRealAttribs, 0, sizeof(D3DVERTEXELEMENT9));

	elems[numRealAttribs].Stream = 0xFF;
	elems[numRealAttribs].Type = D3DDECLTYPE_UNUSED;
}