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

CVertexFormatD3DX9::CVertexFormatD3DX9()
{
	memset(m_nVertexSize,0,sizeof(m_nVertexSize));
	m_pVertexDecl = NULL;
}

int8 CVertexFormatD3DX9::GetVertexSizePerStream(int16 nStream)
{
	return m_nVertexSize[nStream];
}