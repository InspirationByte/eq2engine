//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 vertex layout
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>

#include "core/core_common.h"
#include "D3D11VertexFormat.h"

CVertexFormatD3DX10::~CVertexFormatD3DX10()
{
	if (m_vertexDecl)
		m_vertexDecl->Release();
}

const char* CVertexFormatD3DX10::GetName() const
{
	return m_name;
}

int	CVertexFormatD3DX10::GetVertexSize(int stream) const
{
	return m_streamStride[stream];
}

void CVertexFormatD3DX10::GetFormatDesc(const VertexFormatDesc_t** desc, int& numAttribs) const
{
	*desc = m_vertexDesc.ptr();
	numAttribs = m_vertexDesc.numElem();
}
