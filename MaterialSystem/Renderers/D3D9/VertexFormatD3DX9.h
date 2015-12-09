//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATD3DX9_H
#define VERTEXFORMATD3DX9_H

#include "IVertexFormat.h"

//**************************************
// Vertex format
//**************************************

class CVertexFormatD3DX9 : public IVertexFormat
{
public:
	
	friend class					ShaderAPID3DX9;

									CVertexFormatD3DX9();
	int8							GetVertexSizePerStream(int16 nStream);

protected:
	LPDIRECT3DVERTEXDECLARATION9	m_pVertexDecl;
	int8							m_nVertexSize[MAX_VERTEXSTREAM];
};

#endif // VERTEXFORMATD3DX9_H