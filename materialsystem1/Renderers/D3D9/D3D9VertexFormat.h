//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexFormat.h"

class CD3D9VertexFormat : public IVertexFormat
{
	friend class		ShaderAPID3D9;
public:
	CD3D9VertexFormat(const char* name, const VertexFormatDesc_t* desc, int numAttribs);
	~CD3D9VertexFormat();

	const char*						GetName() const {return m_name.ToCString();}
	int								GetVertexSize(int stream) const;
	ArrayCRef<VertexFormatDesc_t>	GetFormatDesc() const;

	//----------------------

	void							GenVertexElement(D3DVERTEXELEMENT9* elems);

protected:
	int								m_streamStride[MAX_VERTEXSTREAM];
	EqString						m_name;
	Array<VertexFormatDesc_t>		m_vertexDesc{ PP_SL };
	IDirect3DVertexDeclaration9*	m_pVertexDecl{ nullptr };
};