//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IDynamicMesh.h"

struct VertexFormatDesc;
class IVertexFormat;
class IVertexBuffer;
class IIndexBuffer;

class CDynamicMesh : public IDynamicMesh
{
public:
	bool			Init(const VertexFormatDesc* desc, int numAttribs );
	void			Destroy();

	// sets the primitive type (chooses the way how to allocate geometry parts)
	void			SetPrimitiveType( EPrimTopology primType );
	EPrimTopology	GetPrimitiveType() const;

	// returns a pointer to vertex format description
	ArrayCRef<VertexFormatDesc>	GetVertexFormatDesc() const;

	// allocates geometry chunk. Returns the start index. Will return -1 if failed
	// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
	int				AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak = true );

	// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
	bool			FillDrawCmd(RenderDrawCmd& drawCmd, int firstIndex, int numIndices);

	// resets the dynamic mesh
	void			Reset();

	void			AddStripBreak();

protected:
	void*			m_vertices{ nullptr };
	uint16*			m_indices{ nullptr };

	IVertexFormat*	m_vertexFormat{ nullptr };
	IVertexBuffer*	m_vertexBuffer{ nullptr };
	IIndexBuffer*	m_indexBuffer{ nullptr };

	EPrimTopology	m_primType{ PRIM_TRIANGLES };
	int				m_vertexStride{ 0 };
	int				m_vboDirty{ -1 };

	uint16			m_numVertices{ 0 };
	uint16			m_numIndices{ 0 };
};