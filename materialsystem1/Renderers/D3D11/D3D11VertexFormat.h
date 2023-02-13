//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 vertex layout impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IVertexFormat.h"
#include "renderers/ShaderAPI_defs.h"

class CVertexFormatD3DX10 : public IVertexFormat
{
	friend class		ShaderAPID3DX10;
public:
	~CVertexFormatD3DX10();

	const char*			GetName() const;

	int					GetVertexSize(int stream) const;;
	void				GetFormatDesc(const VertexFormatDesc_t** desc, int& numAttribs) const;

protected:
	int							m_streamStride[MAX_VERTEXSTREAM]{ 0 };
	Array<VertexFormatDesc_t>	m_vertexDesc{ PP_SL };
	EqString					m_name;
	ID3D10InputLayout*			m_vertexDecl{ nullptr };
};
