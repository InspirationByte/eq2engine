//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/RenderDefs.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "DynamicMesh.h"

#define MAX_DYNAMIC_VERTICES	32767
#define MAX_DYNAMIC_INDICES		32767

bool CDynamicMesh::Init(ArrayCRef<VertexLayoutDesc> vertexLayout)
{
	if(m_vertexFormat != nullptr)
		return true;

	ASSERT_MSG(vertexLayout.numElem(), "CDynamicMesh::Init - no vertex layout descs");
	ASSERT_MSG(vertexLayout.numElem() == 1, "CDynamicMesh::Init - only one vertex buffer layout supported\n");

	const VertexLayoutDesc& vertexDesc = vertexLayout[0];
	ASSERT_MSG(vertexDesc.attributes.numElem() > 0, "CDynamicMesh::Init - attributes count is ZERO\n");
	ASSERT_MSG(vertexDesc.stride > 0, "CDynamicMesh::Init - vertex layout stride hasn't set\n");

	m_vertexStride = vertexDesc.stride;
	m_vertexFormat = g_renderAPI->CreateVertexFormat("DynMeshVertex", vertexLayout);

	m_vertices = PPAlloc(MAX_DYNAMIC_VERTICES*m_vertexStride);
	m_indices = (uint16*)PPAlloc(MAX_DYNAMIC_INDICES*sizeof(uint16));

	return m_vertices && m_indices;
}

void CDynamicMesh::Destroy()
{
	Reset();

	g_renderAPI->DestroyVertexFormat(m_vertexFormat);
	m_vertexFormat = nullptr;

	PPFree(m_vertices);
	PPFree(m_indices);

	m_vertices = nullptr;
	m_indices = nullptr;
}

// returns a pointer to vertex format description
ArrayCRef<VertexLayoutDesc> CDynamicMesh::GetVertexLayoutDesc() const
{
	ArrayCRef<VertexLayoutDesc> desc(nullptr);
	if(m_vertexFormat)
		desc = m_vertexFormat->GetFormatDesc();

	return desc;
}

// sets the primitive type (chooses the way how to allocate geometry parts)
void CDynamicMesh::SetPrimitiveType( EPrimTopology primType )
{
	m_primType = primType;
}

EPrimTopology CDynamicMesh::GetPrimitiveType() const
{
	return m_primType;
}

void CDynamicMesh::AddStripBreak()
{
	// may be FAN also needs it, I forgot it =(
	if(m_primType != PRIM_TRIANGLE_STRIP)
		return;

	if(m_numIndices == 0)
		return; // no problemo 

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		uint16 lastIdx = m_indices[ num_ind-1 ];
		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		memcpy(&m_indices[m_numIndices], degenerate, sizeof(uint16) * 2);
		m_numIndices += 2;
	}
}

// allocates geometry chunk. Returns the start index. Will return -1 if failed
// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
int CDynamicMesh::AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak /*= true*/ )
{
	if(nVertices == 0 && nIndices == 0)
		return -1;

	if(addStripBreak)
		AddStripBreak();

	const int startVertex = m_numVertices;
	const int startIndex = m_numIndices;

	if (startVertex + nVertices > MAX_DYNAMIC_VERTICES || startIndex + nIndices > MAX_DYNAMIC_INDICES)
		return -1;

	// apply offsets first
	m_numVertices += nVertices;

	if(indices && nIndices)
		m_numIndices += nIndices;

	// give the pointers
	*verts = (ubyte*)m_vertices + startVertex * m_vertexStride;

	memset(*verts, 0, m_vertexStride*nVertices);

	// indices are optional
	if(indices && nIndices)
	{
		*indices = &m_indices[startIndex];
		memset(*indices, 0, sizeof(uint16)*nIndices);
	}

	memset(*verts, 0, m_vertexStride*nVertices);

	return startVertex;
}

bool CDynamicMesh::FillDrawCmd(RenderDrawCmd& drawCmd, int firstIndex, int numIndices)
{
	if (m_numVertices == 0)
		return false;

	drawCmd.vertexBuffers[0] = g_renderAPI->CreateBuffer(BufferInfo(m_vertices, m_vertexStride, m_numVertices), BUFFERUSAGE_VERTEX, "DynMeshVertexBuffer");
	if (m_numIndices > 0)
		drawCmd.indexBuffer = g_renderAPI->CreateBuffer(BufferInfo(m_indices, sizeof(uint16), m_numIndices), BUFFERUSAGE_INDEX, "DynMeshIndexBuffer");

	if (numIndices < 0)
		numIndices = m_numIndices;

	drawCmd.vertexLayout = m_vertexFormat;
	drawCmd.primitiveTopology = m_primType;

	if (m_numIndices > 0)
		drawCmd.SetDrawIndexed(numIndices, firstIndex, m_numVertices);
	else
		drawCmd.SetDrawNonIndexed(m_numVertices);

	return true;
}

// resets the dynamic mesh
void CDynamicMesh::Reset()
{
	m_numVertices = 0;
	m_numIndices = 0;
}