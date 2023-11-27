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

	m_vertexBuffer = g_renderAPI->CreateBuffer(BufferInfo(m_vertexStride, MAX_DYNAMIC_VERTICES), BUFFERUSAGE_VERTEX, "DynMeshVertexBuffer");
	m_indexBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(uint16), MAX_DYNAMIC_VERTICES), BUFFERUSAGE_INDEX, "DynMeshIndexBuffer");

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
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_cmdRecorder = nullptr;
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
	if(m_primType != PRIM_TRIANGLE_STRIP)
		return;

	if(m_numIndices == 0)
		return; // no problemo 

	const uint16 restart = 0xffff;
	m_indices[m_numIndices++] = restart;
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

	if(!m_cmdRecorder)
		m_cmdRecorder = g_renderAPI->CreateCommandRecorder("DynamicMeshSubmit");

	{
		const int writeSize = m_vertexStride * m_numVertices;
		const int writeOffset = NextBufferOffset(writeSize, m_vtxBufferOffset, m_vertexStride * MAX_DYNAMIC_VERTICES);

		m_cmdRecorder->WriteBuffer(m_vertexBuffer, m_vertices, writeSize, writeOffset);
		drawCmd.SetVertexBuffer(0, m_vertexBuffer, writeOffset, writeSize);
	}

	if (m_numIndices > 0)
	{
		const int writeSize = sizeof(uint16) * m_numIndices;
		const int writeOffset = NextBufferOffset(writeSize, m_idxBufferOffset, static_cast<int>(sizeof(uint16)) * MAX_DYNAMIC_VERTICES);

		m_cmdRecorder->WriteBuffer(m_indexBuffer, m_indices, writeSize, writeOffset);
		drawCmd.SetIndexBuffer(m_indexBuffer, INDEXFMT_UINT16, writeOffset, writeSize);
	}

	drawCmd.SetInstanceFormat(m_vertexFormat);

	if (numIndices < 0)
		numIndices = m_numIndices;

	if (numIndices > 0)
		drawCmd.SetDrawIndexed(m_primType, numIndices, firstIndex, m_numVertices);
	else
		drawCmd.SetDrawNonIndexed(m_primType, m_numVertices);

	return true;
}

IGPUCommandBufferPtr CDynamicMesh::GetSubmitBuffer()
{
	if (!m_cmdRecorder)
		return nullptr;

	IGPUCommandBufferPtr cmdBuffer = m_cmdRecorder->End();
	m_cmdRecorder = nullptr;
	m_vtxBufferOffset = 0;
	m_idxBufferOffset = 0;
	return cmdBuffer;
}

void CDynamicMesh::Reset()
{
	m_numVertices = 0;
	m_numIndices = 0;
}