//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format OpenGL declaration
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9VertexFormat.h"

CVertexFormatD3DX9::CVertexFormatD3DX9(const char* name, const VertexFormatDesc_t* desc, int numAttribs)
{
	m_name = name;
	memset(m_streamStride, 0, sizeof(m_streamStride));

	m_vertexDesc.setNum(numAttribs);
	for(int i = 0; i < numAttribs; i++)
		m_vertexDesc[i] = desc[i];
}

CVertexFormatD3DX9::~CVertexFormatD3DX9()
{
	if (m_pVertexDecl)
		m_pVertexDecl->Release();
}

int CVertexFormatD3DX9::GetVertexSize(int nStream) const
{
	return m_streamStride[nStream];
}

void CVertexFormatD3DX9::GetFormatDesc(const VertexFormatDesc_t** desc, int& numAttribs) const
{
	*desc = m_vertexDesc.ptr();
	numAttribs = m_vertexDesc.numElem();
}

//----------------------

void CVertexFormatD3DX9::GenVertexElement(D3DVERTEXELEMENT9* elems)
{
	memset(m_streamStride, 0, sizeof(m_streamStride));

	int numRealAttribs = 0;

	int index[8] = {0};

	// Fill the vertex element array
	for (int i = 0; i < m_vertexDesc.numElem(); i++)
	{
		const VertexFormatDesc_t& fmtdesc = m_vertexDesc[i];
		D3DVERTEXELEMENT9& elem = elems[numRealAttribs];

		int stream = fmtdesc.streamId;
		int size = fmtdesc.elemCount;

		// if not unused
		if(fmtdesc.attribType != VERTEXATTRIB_UNUSED)
		{
			elem.Stream = stream;
			elem.Offset = m_streamStride[stream];
			elem.Type = d3ddecltypes[fmtdesc.attribFormat][size - 1];
			elem.Method = D3DDECLMETHOD_DEFAULT;
			elem.Usage = d3dvertexusage[fmtdesc.attribType];
			elem.UsageIndex = index[fmtdesc.attribType]++;

			numRealAttribs++;
		}

		m_streamStride[stream] += size * s_attributeSize[fmtdesc.attribFormat];
	}

	// Terminating element
	memset(elems + numRealAttribs, 0, sizeof(D3DVERTEXELEMENT9));

	elems[numRealAttribs].Stream = 0xFF;
	elems[numRealAttribs].Type = D3DDECLTYPE_UNUSED;
}