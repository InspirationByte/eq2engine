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
	CD3D9VertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> vertexLayout);
	~CD3D9VertexFormat();

	const char*					GetName() const {return m_name.ToCString();}
	int							GetNameHash() const { return m_nameHash; }
	int							GetVertexSize(int stream) const;
	ArrayCRef<VertexLayoutDesc>	GetFormatDesc() const;

	//----------------------

	void						GenVertexElement(D3DVERTEXELEMENT9* elems);

protected:
	EqString						m_name;
	int								m_nameHash;
	Array<VertexLayoutDesc>			m_vertexDesc{ PP_SL };
	IDirect3DVertexDeclaration9*	m_pVertexDecl{ nullptr };
};