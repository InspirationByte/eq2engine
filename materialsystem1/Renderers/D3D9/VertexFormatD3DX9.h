//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format Direct3D9 declaration//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATD3DX9_H
#define VERTEXFORMATD3DX9_H

#include "renderers/IVertexFormat.h"
#include "renderers/ShaderAPI_defs.h"

//**************************************
// Vertex format
//**************************************

class CVertexFormatD3DX9 : public IVertexFormat
{
	friend class		ShaderAPID3DX9;
public:
	CVertexFormatD3DX9(const char* name, VertexFormatDesc_t* desc, int numAttribs);
	~CVertexFormatD3DX9();

	const char*						GetName() const {return m_name.ToCString();}
	int								GetVertexSize(int stream) const;
	void							GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) const;

	//----------------------

	void							GenVertexElement(D3DVERTEXELEMENT9* elems);

protected:
	int								m_streamStride[MAX_VERTEXSTREAM];
	EqString						m_name;
	VertexFormatDesc_t*				m_vertexDesc;
	int								m_numAttribs;

	IDirect3DVertexDeclaration9*	m_pVertexDecl;

};

#endif // VERTEXFORMATD3DX9_H