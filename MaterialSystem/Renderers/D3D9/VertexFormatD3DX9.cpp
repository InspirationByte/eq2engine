//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format OpenGL declaration
//////////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>
#include <d3dx9.h>

#include "VertexFormatD3DX9.h"
#include "d3dx9_def.h"

CVertexFormatD3DX9::CVertexFormatD3DX9(VertexFormatDesc_t* desc, int numAttribs) 
	: m_pVertexDecl(NULL), m_vertexDesc(NULL), m_numAttribs(numAttribs)
{
	m_vertexDesc = new VertexFormatDesc_t[m_numAttribs];
	memset(m_streamStride, 0, sizeof(m_streamStride));

	for(int i = 0; i < m_numAttribs; i++)
		m_vertexDesc[i] = desc[i];
}

CVertexFormatD3DX9::~CVertexFormatD3DX9()
{
	delete [] m_vertexDesc;
}

int CVertexFormatD3DX9::GetVertexSize(int nStream)
{
	return m_streamStride[nStream];
}

void CVertexFormatD3DX9::GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs)
{
	*desc = m_vertexDesc;
	numAttribs = m_numAttribs;
}

//----------------------

void CVertexFormatD3DX9::GenVertexElement(D3DVERTEXELEMENT9* elems)
{
	memset(m_streamStride, 0, sizeof(m_streamStride));

	int numRealAttribs = 0;

	int index[8] = {0};

	// Fill the vertex element array
	for (int i = 0; i < m_numAttribs; i++)
	{
		VertexFormatDesc_t& fmtdesc = m_vertexDesc[i];
		D3DVERTEXELEMENT9& elem = elems[numRealAttribs];

		int stream = fmtdesc.m_nStream;
		int size = fmtdesc.m_nSize;

		// if not unused
		if(fmtdesc.m_nType != VERTEXTYPE_NONE)
		{
			elem.Stream = stream;
			elem.Offset = m_streamStride[stream];
			elem.Type = d3ddecltypes[fmtdesc.m_nFormat][size - 1];
			elem.Method = D3DDECLMETHOD_DEFAULT;
			elem.Usage = d3dvertexusage[fmtdesc.m_nType];
			elem.UsageIndex = index[fmtdesc.m_nType]++;

			numRealAttribs++;
		}

		m_streamStride[stream] += size * attributeFormatSize[fmtdesc.m_nFormat];
	}

	// Terminating element
	memset(elems + numRealAttribs, 0, sizeof(D3DVERTEXELEMENT9));

	elems[numRealAttribs].Stream = 0xFF;
	elems[numRealAttribs].Type = D3DDECLTYPE_UNUSED;
}