//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2010-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATD3DX9_H
#define VERTEXFORMATD3DX9_H

#include "IVertexFormat.h"

//**************************************
// Vertex format
//**************************************

class CVertexFormatD3DX10 : public IVertexFormat
{
public:
	
	friend class					ShaderAPID3DX10;

									CVertexFormatD3DX10();
	int8							GetVertexSizePerStream(int16 nStream);

protected:
	ID3D10InputLayout*				m_pVertexDecl;
	int8							m_nVertexSize[MAX_VERTEXSTREAM];
};

#endif // VERTEXFORMATD3DX9_H