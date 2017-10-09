//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#include "DynamicMesh.h"
#include "DebugInterface.h"

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
	m_vertexFormat(NULL),
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_numVertices(0),
	m_numIndices(0),
	m_vertices(NULL),
	m_indices(NULL),
	m_lockVertices(NULL),
	m_lockIndices(NULL),
	m_vboDirty(false),
	m_vboAqquired(false)
{

}

CDynamicMesh::~CDynamicMesh()
{
}

bool CDynamicMesh::Init( VertexFormatDesc_t* desc, int numAttribs )
{
	if(m_vertexBuffer != NULL && m_indexBuffer != NULL && m_vertexFormat != NULL)
		return true;

	ASSERTMSG(numAttribs > 0, "CDynamicMesh::Init - numAttribs is ZERO!\n");

	int vertexSize = 0;

	for(int i = 0; i < numAttribs; i++)
	{
		int stream = desc[i].streamId;
		int vecCount = desc[i].elemCount;

		ASSERTMSG(stream == 0, "Error - you should pass STREAM 0 only to CDynamicMesh::Init!\n");

		vertexSize += vecCount * s_attributeSize[desc[i].attribFormat];
	}

	m_vertexStride = vertexSize;

	m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_DYNAMIC_VERTICES, m_vertexStride, NULL);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_DYNAMIC_INDICES, sizeof(uint16), BUFFER_DYNAMIC, NULL);
	m_vertexFormat = g_pShaderAPI->CreateVertexFormat(desc, numAttribs);

	m_vertices = PPAlloc(MAX_DYNAMIC_VERTICES*m_vertexStride);
	m_indices = (uint16*)PPAlloc(MAX_DYNAMIC_INDICES*sizeof(uint16));

	return (m_vertexBuffer != NULL && m_indexBuffer != NULL && m_vertexFormat != NULL);
}

void CDynamicMesh::Destroy()
{
	if(m_vertexBuffer == NULL && m_indexBuffer == NULL && m_vertexFormat == NULL)
		return;

	Reset();
	Unlock();

	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);
	g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);

	PPFree(m_vertices);
	PPFree(m_indices);

	m_indexBuffer = NULL;
	m_vertexBuffer = NULL;
	m_vertexFormat = NULL;

	m_vertices = NULL;
	m_indices = NULL;
}

// returns a pointer to vertex format description
void CDynamicMesh::GetVertexFormatDesc(VertexFormatDesc_t** desc, int& numAttribs)
{
	if(m_vertexFormat)
		m_vertexFormat->GetFormatDesc(desc, numAttribs);
	else
		numAttribs = 0;
}

// sets the primitive type (chooses the way how to allocate geometry parts)
void CDynamicMesh::SetPrimitiveType( ER_PrimitiveType primType )
{
	m_primType = primType;
}

ER_PrimitiveType	CDynamicMesh::GetPrimitiveType() const
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

	m_vboDirty = true;
}

// allocates geometry chunk. Returns the start index. Will return -1 if failed
// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
int CDynamicMesh::AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak /*= true*/ )
{
	if(nVertices == 0 && nIndices == 0)
		return -1;

	if(addStripBreak)
		AddStripBreak();

	int startVertex = m_numVertices;
	int startIndex = m_numIndices;

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

	m_vboDirty = true;

	return startVertex;
}

bool CDynamicMesh::Lock()
{
	if(!m_vboAqquired)
	{
		if(!m_vertexBuffer->Lock(0, m_numVertices, &m_lockVertices, false))
			return false;

		if(!m_indexBuffer->Lock(0, m_numIndices, (void**)&m_lockIndices, false))
			return false;

		m_vboAqquired = true;
	}

	return true;
}

void CDynamicMesh::Unlock()
{
	if(m_vboAqquired)
	{
		m_vertexBuffer->Unlock();
		m_indexBuffer->Unlock();

		m_vboAqquired = false;
	}
}

// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
void CDynamicMesh::Render()
{
	if(m_numVertices == 0)
		return;

	bool drawIndexed = m_numIndices > 0;

	if(m_vboDirty)
	{
		m_vertexBuffer->Update(m_vertices, m_numVertices, 0, true);

		if(drawIndexed)
			m_indexBuffer->Update(m_indices, m_numIndices, 0, true);

		m_vboDirty = false;
	}

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, 0);

	if(drawIndexed)
		g_pShaderAPI->SetIndexBuffer(m_indexBuffer);

	g_pShaderAPI->ApplyBuffers();

	if(drawIndexed)
		g_pShaderAPI->DrawIndexedPrimitives(m_primType, 0, m_numIndices, 0, m_numVertices);
	else
		g_pShaderAPI->DrawNonIndexedPrimitives(m_primType, 0, m_numVertices);
}

// resets the dynamic mesh
void CDynamicMesh::Reset()
{
	m_numVertices = 0;
	m_numIndices = 0;
	m_vboDirty = false;
	Unlock();
}