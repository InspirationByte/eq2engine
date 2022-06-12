//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexFormat.h"

class CVertexFormatD3DX9 : public IVertexFormat
{
	friend class		ShaderAPID3DX9;
public:
	CVertexFormatD3DX9(const char* name, const VertexFormatDesc_t* desc, int numAttribs);
	~CVertexFormatD3DX9();

	const char*						GetName() const {return m_name.ToCString();}
	int								GetVertexSize(int stream) const;
	void							GetFormatDesc(const VertexFormatDesc_t** desc, int& numAttribs) const;

	//----------------------

	void							GenVertexElement(D3DVERTEXELEMENT9* elems);

protected:
	int								m_streamStride[MAX_VERTEXSTREAM];
	EqString						m_name;
	Array<VertexFormatDesc_t>		m_vertexDesc{ PP_SL };
	IDirect3DVertexDeclaration9*	m_pVertexDecl{ nullptr };
};