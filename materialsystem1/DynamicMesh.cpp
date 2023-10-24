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

// pack vertex format description to uint16
struct vertexFormatId_t
{
	// position is default
	ushort types			: 5;		// HAS_TEXCOORD, HAS_COLOR, HAS_NORMAL, HAS_TANGENT, HAS_BINORMAL

	ushort cnt_vertex		: 1;		// 0 - 2D, 1 - 3D
	ushort cnt_color		: 1;		// 0 - RGB, 1 - RGBA
	ushort cnt_tbn			: 1;		// 0 - 2D, 1 - 3D

	ushort fmt_vertex		: 1;		// 0 - float, 1 - half
	ushort fmt_texcoord		: 1;		// 0 - float, 1 - half
	ushort fmt_color		: 1;		// 0 - half, 1 - ubyte
	ushort fmt_tbn			: 1;		// 0 - half, 1 - ubyte
};

assert_sizeof(vertexFormatId_t, 2);

/*
Vertex data order:
	VERTEXATTRIB_POSITION		// default, 2D or 3D (+ 1 comp padding)
	VERTEXATTRIB_TEXCOORD		// 2D only
	VERTEXATTRIB_COLOR		// 3D (alpha used or padding)

	VERTEXATTRIB_TANGENT		// 2D or 3D (+ 1 comp padding)
	VERTEXATTRIB_BINORMAL		// 2D or 3D (+ 1 comp padding)
	VERTEXATTRIB_NORMAL		// 2D or 3D (+ 1 comp padding)

Sort:
	Bigger to smaller
	Combine two small to 1 big if their size is equal

Sizes:
	All halfs (3D): 
		vertex		- 8 bytes
		texcoord	- 4 bytes
		color		- 8 bytes
		tangent		- 8 bytes
		binormal	- 8 bytes
		normal		- 8 bytes
*/

// FIXME: subdivide on streams???

#define MAX_DYNAMIC_VERTICES	32767
#define MAX_DYNAMIC_INDICES		32767

CDynamicMesh::CDynamicMesh() :
	m_primType( PRIM_TRIANGLES ),
	m_vertexFormat(nullptr),
	m_vertexBuffer(nullptr),
	m_indexBuffer(nullptr),
	m_numVertices(0),
	m_numIndices(0),
	m_vertices(nullptr),
	m_indices(nullptr),
	m_lockVertices(nullptr),
	m_lockIndices(nullptr),
	m_vboDirty(-1),
	m_vboAqquired(-1)
{

}

CDynamicMesh::~CDynamicMesh()
{
}

bool CDynamicMesh::Init(const VertexFormatDesc* desc, int numAttribs )
{
	if(m_vertexBuffer != nullptr && m_indexBuffer != nullptr && m_vertexFormat != nullptr)
		return true;

	ASSERT_MSG(numAttribs > 0, "CDynamicMesh::Init - numAttribs is ZERO!\n");

	int vertexSize = 0;

	for(int i = 0; i < numAttribs; i++)
	{
		int stream = desc[i].streamId;
		int vecCount = desc[i].elemCount;

		ASSERT_MSG(stream == 0, "Error - you should pass STREAM 0 only to CDynamicMesh::Init!\n");

		vertexSize += vecCount * s_attributeSize[desc[i].attribFormat];
	}

	m_vertexStride = vertexSize;

	m_vertexBuffer = g_renderAPI->CreateVertexBuffer(BufferInfo(m_vertexStride, MAX_DYNAMIC_VERTICES, BUFFER_DYNAMIC));
	m_indexBuffer = g_renderAPI->CreateIndexBuffer(BufferInfo(sizeof(uint16), MAX_DYNAMIC_INDICES, BUFFER_DYNAMIC));

	m_vertexFormat = g_renderAPI->CreateVertexFormat("DynMeshVertex", ArrayCRef(desc, numAttribs));

	m_vertices = PPAlloc(MAX_DYNAMIC_VERTICES*m_vertexStride);
	m_indices = (uint16*)PPAlloc(MAX_DYNAMIC_INDICES*sizeof(uint16));

	return (m_vertexBuffer != nullptr && m_indexBuffer != nullptr && m_vertexFormat != nullptr);
}

void CDynamicMesh::Destroy()
{
	if(m_vertexBuffer == nullptr && m_indexBuffer == nullptr && m_vertexFormat == nullptr)
		return;

	Reset();
	Unlock();

	g_renderAPI->DestroyIndexBuffer(m_indexBuffer);
	g_renderAPI->DestroyVertexBuffer(m_vertexBuffer);

	g_renderAPI->DestroyVertexFormat(m_vertexFormat);

	PPFree(m_vertices);
	PPFree(m_indices);

	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;

	m_vertexFormat = nullptr;

	m_vertices = nullptr;
	m_indices = nullptr;
}

// returns a pointer to vertex format description
ArrayCRef<VertexFormatDesc> CDynamicMesh::GetVertexFormatDesc() const
{
	ArrayCRef<VertexFormatDesc> desc(nullptr);
	if(m_vertexFormat)
		desc = m_vertexFormat->GetFormatDesc();

	return desc;
}

// sets the primitive type (chooses the way how to allocate geometry parts)
void CDynamicMesh::SetPrimitiveType( EPrimTopology primType )
{
	m_primType = primType;
}

EPrimTopology	CDynamicMesh::GetPrimitiveType() const
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

	m_vboDirty = 0;
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

	if (startVertex + nVertices > MAX_DYNAMIC_VERTICES ||
		startIndex + nIndices > MAX_DYNAMIC_INDICES)
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

	m_vboDirty = 0;

	return startVertex;
}

bool CDynamicMesh::Lock()
{
	if(m_vboAqquired == -1)
	{
		if(!m_vertexBuffer->Lock(0, m_numVertices, &m_lockVertices, BUFFER_FLAG_WRITE))
			return false;

		if (!m_indexBuffer->Lock(0, m_numIndices, (void**)&m_lockIndices, BUFFER_FLAG_WRITE))
		{
			m_vertexBuffer->Unlock();
			return false;
		}

		m_vboAqquired = 0;
	}

	return true;
}

void CDynamicMesh::Unlock()
{
	if (m_vboAqquired == -1)
		return;

	m_vertexBuffer->Unlock();
	m_indexBuffer->Unlock();

	m_vboAqquired = -1;
	m_vboDirty = -1;
}

bool CDynamicMesh::FillDrawCmd(RenderDrawCmd& drawCmd, int firstIndex, int numIndices)
{
	if (m_numVertices == 0)
		return false;

	ASSERT(m_vboAqquired != 0);

	const bool drawIndexed = m_numIndices > 0;
	if (m_vboDirty == 0)
	{
		// FIXME: CMeshBuilder::End should return new trainsient buffer really?
		// would be cool probably
		m_vertexBuffer->Update(m_vertices, m_numVertices);
		if (drawIndexed)
			m_indexBuffer->Update(m_indices, m_numIndices);
		m_vboDirty = -1;
	}

	if (numIndices < 0)
		numIndices = m_numIndices;

	drawCmd.vertexLayout = m_vertexFormat;
	drawCmd.vertexBuffers[0] = m_vertexBuffer;
	drawCmd.indexBuffer = drawIndexed ? m_indexBuffer : nullptr;
	drawCmd.primitiveTopology = m_primType;
	if (drawIndexed)
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

	m_vboDirty = -1;

	Unlock();
}