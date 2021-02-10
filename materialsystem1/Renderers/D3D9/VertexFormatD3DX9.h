//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATD3DX9_H
#define VERTEXFORMATD3DX9_H

#include "renderers/IVertexFormat.h"

//**************************************
// Vertex format
//**************************************

class CVertexFormatD3DX9 : public IVertexFormat
{
	friend class		ShaderAPID3DX9;
public:
	CVertexFormatD3DX9(VertexFormatDesc_t* desc, int numAttribs);
	~CVertexFormatD3DX9();

	int					GetVertexSize(int stream);
	void				GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs);

	//----------------------

	void				GenVertexElement(D3DVERTEXELEMENT9* elems);

protected:
	int					m_streamStride[MAX_VERTEXSTREAM];
	VertexFormatDesc_t*	m_vertexDesc;
	int					m_numAttribs;

	IDirect3DVertexDeclaration9*	m_pVertexDecl;

};

#endif // VERTEXFORMATD3DX9_H