//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IDynamicMesh.h"

struct VertexLayoutDesc;
class IVertexFormat;
class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IGPUCommandRecorder;
using IGPUCommandRecorderPtr = CRefPtr<IGPUCommandRecorder>;

class CDynamicMesh : public IDynamicMesh
{
public:
	~CDynamicMesh();

	void Ref_DeleteObject() override;

	bool					Init(int id, IVertexFormat* vertexFormat);
	void					Destroy();

	// sets the primitive type (chooses the way how to allocate geometry parts)
	void					SetPrimitiveType( EPrimTopology primType );
	EPrimTopology			GetPrimitiveType() const;

	// returns a pointer to vertex format description
	ArrayCRef<VertexLayoutDesc>	GetVertexLayoutDesc() const;

	// allocates geometry chunk. Returns the start index. Will return -1 if failed
	// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
	int						AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak = true );

	// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
	bool					FillDrawCmd(RenderDrawCmd& drawCmd, int firstIndex, int numIndices);

	// resets the dynamic mesh
	void					Reset();

	void					AddStripBreak();

	IGPUCommandBufferPtr	GetSubmitBuffer();

protected:
	void*					m_vertices{ nullptr };
	uint16*					m_indices{ nullptr };
	uint16					m_numVertices{ 0 };
	uint16					m_numIndices{ 0 };

	EPrimTopology			m_primType{ PRIM_TRIANGLES };
	int						m_vertexStride{ 0 };
	int						m_id{ -1 };

	IGPUCommandRecorderPtr	m_cmdRecorder;
	int						m_vtxBufferOffset{ 0 };
	int						m_idxBufferOffset{ 0 };

	IVertexFormat*			m_vertexFormat{ nullptr };
	IGPUBufferPtr			m_vertexBuffer;
	IGPUBufferPtr			m_indexBuffer;
};