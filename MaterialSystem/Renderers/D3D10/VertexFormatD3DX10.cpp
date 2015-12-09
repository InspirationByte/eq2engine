//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format OpenGL declaration
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include <d3dx10.h>

#include "VertexFormatD3DX10.h"

CVertexFormatD3DX10::CVertexFormatD3DX10()
{
	memset(m_nVertexSize,0,sizeof(m_nVertexSize));
	m_pVertexDecl = NULL;
}

int8 CVertexFormatD3DX10::GetVertexSizePerStream(int16 nStream)
{
	return m_nVertexSize[nStream];
}